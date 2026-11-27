#include "sound_chef/sound_chef.h"
#include <string.h>

#define SC_RESAMPLER_LPF_ORDER 4

// Pitch changes rebuild the filter for the new ratio. A cutoff at Nyquist puts its poles on the unit circle when the
// ratio nears 1:1, so keep it just under.
#define SC_RESAMPLER_LPF_NYQUIST_FACTOR 0.95

static sbk_status sc_voice_resolve(sc_system* system, sc_voice_handle handle, sc_voice** outVoice)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outVoice != NULL);

    const sc_voice_index slot = SC_VOICE_HANDLE_EXTRACT_INDEX(handle);
    SC_CHECK_ARG(slot < system->voiceSlotAllocator.capacity);

    sc_voice* const voice         = &system->voiceBuffer[slot];
    const sc_voice_handle current = (sc_voice_handle)c89atomic_load_64(&voice->handle);
    SC_CHECK(current == handle, SBK_ERR_NOT_FOUND);

    const sc_voice_state state = (sc_voice_state)c89atomic_load_32(&voice->currentState);
    SC_CHECK(state != sc_voice_state_stopped && state != sc_voice_state_free, SBK_ERR_NOT_FOUND);

    *outVoice = voice;
    return SBK_SUCCESS;
}

static void sc_voice_write_flag(volatile sc_atomic_uint32* flags, sc_voice_flags flag, sc_bool add)
{
    for (;;)
    {
        const sc_uint32 old     = c89atomic_load_32(flags);
        const sc_uint32 desired = add ? (old | (sc_uint32)flag) : (old & ~(sc_uint32)flag);

        if (old == desired)
        {
            return;
        }

        if (c89atomic_compare_and_swap_32(flags, old, desired) == old)
        {
            return;
        }
    }
}

void sc_ramp_reset(sc_ramp* ramp, float gain)
{
    ramp->current = gain;
    ramp->target  = gain;
    ramp->step    = 0.0F;
}

void sc_ramp_set_target(sc_ramp* ramp, float target, float frames)
{
    const float distance = target > ramp->current ? target - ramp->current : ramp->current - target;

    ramp->target = target;
    ramp->step   = frames >= 1.0F ? distance / frames : distance;

    if (ramp->step <= 0.0F)
    {
        ramp->current = target;
    }
}

sc_bool sc_ramp_is_moving(const sc_ramp* ramp)
{
    return ramp->current != ramp->target;
}

void sc_ramp_advance(sc_ramp* ramp, ma_uint64 frames)
{
    const float distance = ramp->step * (float)frames;

    if (ramp->current < ramp->target)
    {
        ramp->current = SC_MIN(ramp->current + distance, ramp->target);
    }
    else if (ramp->current > ramp->target)
    {
        ramp->current = SC_MAX(ramp->current - distance, ramp->target);
    }
}

float sc_ramp_next(sc_ramp* ramp)
{
    sc_ramp_advance(ramp, 1u);
    return ramp->current;
}

float sc_fade_gain_at(const sc_fade* fade, sc_uint64 clock)
{
    if (clock >= fade->endClock)
    {
        return fade->toGain;
    }
    if (clock <= fade->startClock)
    {
        return fade->fromGain;
    }

    const float progress = (float)(clock - fade->startClock) / (float)(fade->endClock - fade->startClock);
    return fade->fromGain + (fade->toGain - fade->fromGain) * progress;
}

/**
 * @brief The fade from @p clock onwards as a ramp that can be stepped a frame at a time.
 */
sc_ramp sc_fade_ramp_at(const sc_fade* fade, sc_uint64 clock)
{
    sc_ramp ramp;
    sc_ramp_reset(&ramp, sc_fade_gain_at(fade, clock));

    if (clock >= fade->startClock && clock < fade->endClock)
    {
        const float distance = fade->toGain > fade->fromGain ? fade->toGain - fade->fromGain : fade->fromGain - fade->toGain;
        ramp.target          = fade->toGain;
        ramp.step            = distance / (float)(fade->endClock - fade->startClock);
    }
    return ramp;
}

void sc_fade_start(sc_fade* fade, sc_uint64 clock, float fromGain, float toGain, sc_uint64 frames)
{
    fade->fromGain   = fromGain;
    fade->toGain     = toGain;
    fade->startClock = clock;
    fade->endClock   = clock + frames;
}

/**
 * @brief Picks up new fades and delayed stops, and works out the fade for this callback. Runs on the audio thread
 * for every live voice, real or virtual, before the graph is read.
 *
 * @param clock The voice's parent clock at the start of this callback.
 */
void sc_voice_update_schedule(sc_system* system, sc_voice* voice, sc_uint64 clock, ma_uint64 frameCount)
{
    // The stop is read first so a fade requested before it is never picked up a callback later than the stop.
    const sc_uint32 stopEpoch = c89atomic_load_32(&voice->stopEpoch);
    const sc_uint32 fadeEpoch = c89atomic_load_32(&voice->fadeEpoch);

    if (stopEpoch != voice->appliedStopEpoch)
    {
        ma_spinlock_lock(&voice->scheduleLock);
        const float seconds = voice->pendingStopSeconds;
        ma_spinlock_unlock(&voice->scheduleLock);

        voice->stopClock        = clock + sc_seconds_to_frames(system, seconds);
        voice->stopScheduled    = SC_TRUE;
        voice->appliedStopEpoch = stopEpoch;
    }

    if (fadeEpoch != voice->appliedFadeEpoch)
    {
        ma_spinlock_lock(&voice->scheduleLock);
        const sc_bool jump   = voice->pendingJump;
        const float jumpGain = voice->pendingJumpGain;
        const float toGain   = voice->pendingFadeTo;
        const float seconds  = voice->pendingFadeSeconds;
        voice->pendingJump   = SC_FALSE;
        ma_spinlock_unlock(&voice->scheduleLock);

        const float startGain = jump ? jumpGain : sc_fade_gain_at(&voice->fade, clock);
        sc_fade_start(&voice->fade, clock, startGain, toGain, sc_seconds_to_frames(system, seconds));
        voice->appliedFadeEpoch = fadeEpoch;
    }

    if (voice->stopScheduled && clock >= voice->stopClock)
    {
        voice->stopScheduled = SC_FALSE;
        c89atomic_store_32(&voice->desiredState, (sc_uint32)sc_voice_desired_stopped);
    }

    if (c89atomic_load_32(&voice->desiredState) == (sc_uint32)sc_voice_desired_stopped)
    {
        // However the voice was stopped, it reaches silence within the declick time, even part way through a long fade.
        const sc_uint64 declickFrames = sc_seconds_to_frames(system, SC_DECLICK_SECONDS);
        if (voice->fade.toGain != 0.0F || voice->fade.endClock > clock + declickFrames)
        {
            sc_fade_start(&voice->fade, clock, sc_fade_gain_at(&voice->fade, clock), 0.0F, declickFrames);
        }
    }

    voice->fadeAtCallbackStart = sc_fade_ramp_at(&voice->fade, clock);
    c89atomic_store_f32(&voice->fadeGain, sc_fade_gain_at(&voice->fade, clock + frameCount));
}

/**
 * @brief Queues a fade for the audio thread. The caller holds @ref sc_voice::scheduleLock and bumps @ref sc_voice::fadeEpoch.
 */
static void sc_voice_set_pending_fade(sc_voice* voice, float gain, float seconds)
{
    if (seconds <= 0.0F)
    {
        voice->pendingJump     = SC_TRUE;
        voice->pendingJumpGain = gain;
    }
    voice->pendingFadeTo      = gain;
    voice->pendingFadeSeconds = seconds;
}

sbk_status sc_voice_fade_to(sc_system* system, sc_voice_handle handle, float gain, float seconds)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(gain >= 0.0F);
    SC_CHECK_ARG(seconds >= 0.0F);
    sc_bool isStopping = SC_FALSE;
    SC_CHECK_STATUS(sc_voice_get_is_stopping(system, handle, &isStopping));
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));

    if (isStopping == SC_FALSE)
    {
        ma_spinlock_lock(&voice->scheduleLock);
        sc_voice_set_pending_fade(voice, gain, seconds);
        ma_spinlock_unlock(&voice->scheduleLock);

        c89atomic_fetch_add_32(&voice->fadeEpoch, 1u);
    }

    return SBK_SUCCESS;
}

sbk_status sc_voice_get_parent_clock(sc_system* system, sc_voice_handle handle, sc_uint64* outClock)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outClock != NULL);
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    return sc_node_group_get_clock(voice->parentNodeGroup, outClock);
}

sbk_status sc_voice_set_paused(sc_system* system, sc_voice_handle handle, sc_bool paused)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_PAUSED, paused);
    return SBK_SUCCESS;
}

sbk_status SC_API sc_voice_set_virtual(sc_system* system, sc_voice_handle handle, sc_bool virtualized)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_VIRTUAL, virtualized);
    return SBK_SUCCESS;
}

sbk_status sc_voice_stop(sc_system* system, sc_voice_handle handle, float delaySeconds, sc_voice_stop_mode mode)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(delaySeconds >= 0.0F);
    sc_bool isStopping = SC_FALSE;
    SC_CHECK_STATUS(sc_voice_get_is_stopping(system, handle, &isStopping));

    if (isStopping == SC_TRUE)
    {
        return SBK_SUCCESS;
    }

    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));

    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_INSTANT_STOP, mode == sc_voice_stop_mode_cut_tail);

    if (delaySeconds > 0.0F)
    {
        ma_spinlock_lock(&voice->scheduleLock);
        voice->pendingStopSeconds = delaySeconds;
        ma_spinlock_unlock(&voice->scheduleLock);

        c89atomic_fetch_add_32(&voice->stopEpoch, 1u);
    }
    else
    {
        c89atomic_store_32(&voice->desiredState, (sc_uint32)sc_voice_desired_stopped);
    }
    return SBK_SUCCESS;
}

sbk_status sc_voice_stop_with_fade(sc_system* system, sc_voice_handle handle, float fadeSeconds, sc_voice_stop_mode mode)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(fadeSeconds >= 0.0F);

    if (fadeSeconds <= 0.0F)
    {
        return sc_voice_stop(system, handle, 0.0F, mode);
    }

    sc_bool isStopping = SC_FALSE;
    SC_CHECK_STATUS(sc_voice_get_is_stopping(system, handle, &isStopping));

    if (isStopping == SC_TRUE)
    {
        return SBK_SUCCESS;
    }

    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));

    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_INSTANT_STOP, mode == sc_voice_stop_mode_cut_tail);

    ma_spinlock_lock(&voice->scheduleLock);
    sc_voice_set_pending_fade(voice, 0.0F, fadeSeconds);
    voice->pendingStopSeconds = fadeSeconds;
    ma_spinlock_unlock(&voice->scheduleLock);

    // The audio thread reads the stop's epoch before the fade's, so bumping the fade's first means it can't see one without the other.
    c89atomic_fetch_add_32(&voice->fadeEpoch, 1u);
    c89atomic_fetch_add_32(&voice->stopEpoch, 1u);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_stopped_callback(sc_system* system, sc_voice_handle handle, sc_voice_stopped_proc callback, void* pUserData)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    voice->stoppedCallback          = callback;
    voice->stoppedCallbackUserData = pUserData;
    return SBK_SUCCESS;
}

sbk_status sc_voice_add_dsp(sc_system* system, sc_voice_handle handle, sc_dsp* dsp, sc_dsp_index index)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(dsp != NULL);
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    SC_CHECK(voice->voiceNodeGroup != NULL, SBK_ERR_NULL);
    return sc_node_group_add_dsp(voice->voiceNodeGroup, dsp, index);
}

sbk_status sc_system_stop_all_voices(sc_system* system)
{
    SC_CHECK_ARG(system != NULL);
    for (sc_uint32 slot = 0; slot < system->voiceSlotAllocator.capacity; ++slot)
    {
        c89atomic_store_32(&system->voiceBuffer[slot].desiredState, (sc_uint32)sc_voice_desired_stopped);
    }
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_is_playing(sc_system* system, sc_voice_handle handle, sc_bool* outPlaying)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outPlaying != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    const sc_voice_state state = (sc_voice_state)c89atomic_load_32(&voice->currentState);
    *outPlaying                = state != sc_voice_state_free && state != sc_voice_state_stopped;
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_is_stopping(sc_system* system, sc_voice_handle handle, sc_bool* outStopping)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outStopping != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    const sc_voice_state state = (sc_voice_state)c89atomic_load_32(&voice->currentState);
    *outStopping = c89atomic_load_32(&voice->desiredState) == (sc_uint32)sc_voice_desired_stopped;
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_cursor_position_in_seconds(sc_system* system, sc_voice_handle handle, float* outSeconds)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outSeconds!= NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    const sc_uint64 playCursorInFrames = c89atomic_load_64(&voice->playCursor);
    const ma_uint32 sampleRate         = ma_engine_get_sample_rate(system);
    /* VC6 does not support division of unsigned 64-bit integers with floating point numbers. Need to use a signed number. This shouldn't effect anything in practice. */
    *outSeconds = (ma_int64)playCursorInFrames / (float)sampleRate;
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_cursor_position_in_seconds(sc_system* system, sc_voice_handle handle, float seconds)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(seconds >= 0.0F);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    const ma_uint32 sampleRate = ma_engine_get_sample_rate(system);
    const sc_int64 targetFrames = (sc_int64)(seconds * (float)sampleRate);
    c89atomic_store_i64(&voice->pendingSeekFrames, targetFrames);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_loop_position_in_seconds(sc_system* system, sc_voice_handle handle, float loopStartSeconds, float loopEndSeconds)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(loopStartSeconds >= 0.0F);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_f32(&voice->loopStartSeconds, loopStartSeconds);
    c89atomic_store_f32(&voice->loopEndSeconds, loopEndSeconds);
    c89atomic_fetch_add_32(&voice->loopEpoch, 1u);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_mix_matrix(sc_system* system, sc_voice_handle handle, const float* matrix, sc_uint32 channelsIn, sc_uint32 channelsOut)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(matrix == NULL || (channelsIn > 0 && channelsIn <= SC_MAX_CHANNELS && channelsOut > 0 && channelsOut <= SC_MAX_CHANNELS));
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));

    // Allocated outside the lock so the audio thread never waits on an allocation.
    float* newMatrix = NULL;
    if (matrix != NULL && voice->mixMatrix == NULL)
    {
        newMatrix = (float*)ma_malloc(sizeof(float) * SC_MAX_CHANNELS * SC_MAX_CHANNELS, &system->engine.allocationCallbacks);
        SC_CHECK_MEM(newMatrix);
    }

    ma_spinlock_lock(&voice->mixLock);

    if (newMatrix != NULL && voice->mixMatrix == NULL)
    {
        voice->mixMatrix = newMatrix;
        newMatrix        = NULL;
    }

    if (matrix != NULL)
    {
        memcpy(voice->mixMatrix, matrix, sizeof(float) * channelsIn * channelsOut);
        voice->mixChannelsIn  = channelsIn;
        voice->mixChannelsOut = channelsOut;
    }
    else
    {
        voice->mixChannelsIn  = 0;
        voice->mixChannelsOut = 0;
    }

    ma_spinlock_unlock(&voice->mixLock);

    ma_free(newMatrix, &system->engine.allocationCallbacks);
    c89atomic_fetch_add_32(&voice->mixEpoch, 1u);
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_looping(sc_system* system, sc_voice_handle handle, sc_bool* outLooping)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outLooping != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    *outLooping = c89atomic_load_32(&voice->looping) != 0u;
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_looping(sc_system* system, sc_voice_handle handle, sc_bool looping)
{
    SC_CHECK_ARG(system != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_32(&voice->looping, looping ? 1u : 0u);
    c89atomic_fetch_add_32(&voice->loopEpoch, 1u);
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_paused(sc_system* system, sc_voice_handle handle, sc_bool* outPaused)
{
    SC_CHECK_ARG(outPaused != NULL);
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    *outPaused = SC_VOICE_HAS_FLAG(c89atomic_load_32(&voice->flags), SC_VOICE_FLAG_PAUSED);
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_virtual(sc_system* system, sc_voice_handle handle, sc_bool* outVirtual)
{
    SC_CHECK_ARG(outVirtual != NULL);
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    *outVirtual = SC_VOICE_HAS_FLAG(c89atomic_load_32(&voice->flags), SC_VOICE_FLAG_VIRTUAL);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_volume(sc_system* system, sc_voice_handle handle, float volume)
{
    SC_CHECK_ARG(system != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_f32(&voice->gain, volume);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_pitch(sc_system* system, sc_voice_handle handle, float pitch)
{
    SC_CHECK_ARG(system != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    if (pitch < 0.001f)
    {
        pitch = 0.001f;
    }
    c89atomic_store_f32(&voice->pitch, pitch);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_lowpass_cutoff(sc_system* system, sc_voice_handle handle, float cutoff)
{
    SC_CHECK_ARG(system != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_f32(&voice->lowpassCutoff, cutoff);
    return SBK_SUCCESS;
}

sbk_status sc_voice_set_highpass_cutoff(sc_system* system, sc_voice_handle handle, float cutoff)
{
    SC_CHECK_ARG(system != NULL);
    sc_voice* voice;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_f32(&voice->highpassCutoff, cutoff);
    return SBK_SUCCESS;
}

static void sc_real_voice_update_pitch_if_required(sc_real_voice* realVoice, ma_uint32 sampleRate)
{
    ma_bool32 isUpdateRequired = MA_FALSE;
    float newPitch             = c89atomic_load_f32(&realVoice->voiceRef->pitch);

    if (realVoice->voiceRef->oldPitch != newPitch)
    {
        realVoice->voiceRef->oldPitch = newPitch;
        isUpdateRequired              = MA_TRUE;
    }

    if (isUpdateRequired)
    {
        const float basePitch = (float)sampleRate / ma_engine_get_sample_rate(&realVoice->system->engine);
        ma_linear_resampler_set_rate_ratio(&realVoice->resampler, basePitch * realVoice->voiceRef->oldPitch);
    }
}

static void sc_real_voice_sync_mix_if_needed(sc_real_voice* realVoice)
{
    sc_voice* const voice      = realVoice->voiceRef;
    const sc_uint32 voiceEpoch = c89atomic_load_32(&voice->mixEpoch);
    if (voiceEpoch == realVoice->appliedMixEpoch)
    {
        return;
    }

    ma_spinlock_lock(&voice->mixLock);

    const sc_bool hasMatrix   = voice->mixMatrix != NULL && voice->mixChannelsIn == realVoice->sourceChannels && voice->mixChannelsOut == realVoice->outputChannels;
    realVoice->mixChannelsIn  = hasMatrix ? voice->mixChannelsIn : 0;
    realVoice->mixChannelsOut = hasMatrix ? voice->mixChannelsOut : 0;
    if (hasMatrix)
    {
        memcpy(realVoice->mixMatrix, voice->mixMatrix, sizeof(float) * voice->mixChannelsIn * voice->mixChannelsOut);
    }

    ma_spinlock_unlock(&voice->mixLock);

    realVoice->appliedMixEpoch = voiceEpoch;
}

static void sc_apply_mix_matrix_f32(float* framesOut, const float* framesIn, ma_uint64 frameCount, const float* matrix, ma_uint32 channelsIn, ma_uint32 channelsOut)
{
    for (ma_uint64 frame = 0; frame < frameCount; ++frame)
    {
        const float* const frameIn = framesIn + frame * channelsIn;
        float* const frameOut      = framesOut + frame * channelsOut;

        for (ma_uint32 channelOut = 0; channelOut < channelsOut; ++channelOut)
        {
            const float* const row = matrix + channelOut * channelsIn;
            float sum              = 0.0F;
            for (ma_uint32 channelIn = 0; channelIn < channelsIn; ++channelIn)
            {
                sum += row[channelIn] * frameIn[channelIn];
            }
            frameOut[channelOut] = sum;
        }
    }
}

static void sc_real_voice_sync_loop_if_needed(sc_real_voice* realVoice)
{
    const sc_uint32 voiceEpoch = c89atomic_load_32(&realVoice->voiceRef->loopEpoch);
    if (voiceEpoch == realVoice->appliedLoopEpoch)
    {
        return;
    }

    ma_uint32 dataSourceSampleRate = 0;
    if (ma_data_source_get_data_format(realVoice->dataSource, NULL, NULL, &dataSourceSampleRate, NULL, 0) != MA_SUCCESS || dataSourceSampleRate == 0)
    {
        // Data source not yet ready (async load in flight, etc). Leave the cache
        // unbumped so we retry on the next callback.
        return;
    }

    const float loopStartSeconds = c89atomic_load_f32(&realVoice->voiceRef->loopStartSeconds);
    const float loopEndSeconds   = c89atomic_load_f32(&realVoice->voiceRef->loopEndSeconds);
    const sc_bool looping        = c89atomic_load_32(&realVoice->voiceRef->looping) != 0u;

    const ma_uint64 loopStartFrames = (loopStartSeconds > 0.0F) ? (ma_uint64)(loopStartSeconds * (float)dataSourceSampleRate) : 0u;
    // <= 0 means "to end of source"; miniaudio expects (ma_uint64)-1 for that.
    const ma_uint64 loopEndFrames   = (loopEndSeconds > 0.0F) ? (ma_uint64)(loopEndSeconds * (float)dataSourceSampleRate) : (ma_uint64)-1;

    (void)ma_data_source_set_loop_point_in_pcm_frames(realVoice->dataSource, loopStartFrames, loopEndFrames);
    (void)ma_data_source_set_looping(realVoice->dataSource, looping ? MA_TRUE : MA_FALSE);

    realVoice->appliedLoopEpoch = voiceEpoch;
}

/**
 * @brief Points the real voice's gains at what the voice wants now. The fade is copied once per callback from
 * where the voice's fade is at the callback's start, then stepped a frame at a time.
 */
static void sc_real_voice_sync_gains(sc_real_voice* realVoice, sc_bool paused)
{
    sc_voice* const voice     = realVoice->voiceRef;
    const float declickFrames = sc_declick_frames(realVoice->system);

    const float volume = c89atomic_load_f32(&voice->gain);
    if (volume != realVoice->volumeRamp.target)
    {
        sc_ramp_set_target(&realVoice->volumeRamp, volume, declickFrames);
    }

    const float declickTarget = paused ? 0.0F : 1.0F;
    if (declickTarget != realVoice->declickRamp.target)
    {
        sc_ramp_set_target(&realVoice->declickRamp, declickTarget, declickFrames);
    }

    if (realVoice->fadeSyncedCallback != realVoice->system->callbackIndex)
    {
        realVoice->fadeRamp               = voice->fadeAtCallbackStart;
        realVoice->fadeSyncedCallback = realVoice->system->callbackIndex;
    }
}

static sc_bool sc_ramp_is_silent(const sc_ramp* ramp)
{
    return ramp->current <= 0.0F && ramp->target <= 0.0F;
}

/**
 * @brief Applies volume, fade and declick as one gain, stepping any that are moving a frame at a time.
 */
static void sc_real_voice_apply_gain(sc_real_voice* realVoice, float* frames, ma_uint64 frameCount, ma_uint32 channels)
{
    sc_ramp* const volume  = &realVoice->volumeRamp;
    sc_ramp* const fade    = &realVoice->fadeRamp;
    sc_ramp* const declick = &realVoice->declickRamp;

    if (!sc_ramp_is_moving(volume) && !sc_ramp_is_moving(fade) && !sc_ramp_is_moving(declick))
    {
        const float gain = volume->current * fade->current * declick->current;
        if (gain == 0.0F)
        {
            memset(frames, 0, sizeof(float) * frameCount * channels);
        }
        else if (gain != 1.0F)
        {
            ma_apply_volume_factor_f32(frames, frameCount * channels, gain);
        }
        return;
    }

    for (ma_uint64 frame = 0; frame < frameCount; ++frame)
    {
        const float gain = sc_ramp_next(volume) * sc_ramp_next(fade) * sc_ramp_next(declick);

        float* const frameSamples = frames + frame * channels;
        for (ma_uint32 channel = 0; channel < channels; ++channel)
        {
            frameSamples[channel] *= gain;
        }
    }
}

/**
 * @brief The voice's play cursor only counts forwards, so when the source loops back the cursor jumps back by the same
 * amount. Moving it by the jump rather than to the source's position keeps it at the callback's start, where the
 * cursor sits while the graph is read.
 */
static void sc_real_voice_sync_cursor_if_looped(sc_real_voice* realVoice, ma_uint64 cursorBeforeRead, ma_uint64 framesRead, ma_uint32 dataSourceSampleRate)
{
    ma_uint64 cursorAfterRead = 0;
    if (ma_data_source_get_cursor_in_pcm_frames(realVoice->dataSource, &cursorAfterRead) != MA_SUCCESS || cursorAfterRead >= cursorBeforeRead + framesRead)
    {
        return;
    }

    sc_voice* const voice            = realVoice->voiceRef;
    const ma_uint32 engineSampleRate = ma_engine_get_sample_rate(&realVoice->system->engine);
    const double jump                = (double)(ma_int64)(cursorBeforeRead + framesRead - cursorAfterRead) * (double)engineSampleRate / (double)dataSourceSampleRate;
    const double cursor              = (double)(ma_int64)c89atomic_load_64(&voice->playCursor) + voice->playCursorRemainder - jump;

    if (cursor <= 0.0)
    {
        c89atomic_store_64(&voice->playCursor, 0u);
        voice->playCursorRemainder = 0.0;
        return;
    }

    const sc_uint64 wholeFrames = (sc_uint64)cursor;
    c89atomic_store_64(&voice->playCursor, wholeFrames);
    voice->playCursorRemainder = cursor - (double)wholeFrames;
}

/**
 * @brief Moves the data source to the voice's play cursor. Returns false if the source isn't ready to seek yet.
 */
static sc_bool sc_real_voice_seek_to_cursor(sc_real_voice* realVoice)
{
    ma_uint32 dataSourceSampleRate = 0;
    if (ma_data_source_get_data_format(realVoice->dataSource, NULL, NULL, &dataSourceSampleRate, NULL, 0) != MA_SUCCESS || dataSourceSampleRate == 0)
    {
        return SC_FALSE;
    }

    // playCursor is in engine frames; the data source may run at a different rate.
    const ma_uint32 engineSampleRate = ma_engine_get_sample_rate(&realVoice->system->engine);
    const ma_uint64 playCursor       = c89atomic_load_64(&realVoice->voiceRef->playCursor);
    const ma_uint64 targetFrame      = (ma_uint64)((double)(ma_int64)playCursor * (double)dataSourceSampleRate / (double)engineSampleRate);

    // A stream drops what it has buffered when it seeks, so one already in place, like a new voice at the top, is left alone.
    ma_uint64 sourceFrame = 0;
    if (ma_data_source_get_cursor_in_pcm_frames(realVoice->dataSource, &sourceFrame) != MA_SUCCESS || sourceFrame != targetFrame)
    {
        (void)ma_data_source_seek_to_pcm_frame(realVoice->dataSource, targetFrame);
    }
    return SC_TRUE;
}

/**
 * @brief The inverse of @ref sc_real_voice_seek_to_cursor: moves the voice's cursor to wherever the source has read up
 * to. Returns false if the source can't say yet.
 */
static sc_bool sc_real_voice_move_cursor_to_source(sc_real_voice* realVoice)
{
    ma_uint32 dataSourceSampleRate = 0;
    ma_uint64 sourceFrame          = 0;
    if (ma_data_source_get_data_format(realVoice->dataSource, NULL, NULL, &dataSourceSampleRate, NULL, 0) != MA_SUCCESS || dataSourceSampleRate == 0 || ma_data_source_get_cursor_in_pcm_frames(realVoice->dataSource, &sourceFrame) != MA_SUCCESS)
    {
        return SC_FALSE;
    }

    const ma_uint32 engineSampleRate = ma_engine_get_sample_rate(&realVoice->system->engine);
    c89atomic_store_64(&realVoice->voiceRef->playCursor, (sc_uint64)((double)(ma_int64)sourceFrame * (double)engineSampleRate / (double)dataSourceSampleRate));
    realVoice->voiceRef->playCursorRemainder = 0.0;
    return SC_TRUE;
}

static void sc_real_voice_move_source_to_cursor(sc_real_voice* realVoice)
{
    const sc_uint32 voiceEpoch = c89atomic_load_32(&realVoice->voiceRef->seekEpoch);
    if (sc_real_voice_seek_to_cursor(realVoice))
    {
        ma_linear_resampler_reset(&realVoice->resampler);
        realVoice->appliedSeekEpoch = voiceEpoch;
        realVoice->sourceAtCursor   = SC_TRUE;
    }
}

static sc_bool sc_real_voice_is_source_at_cursor(const sc_real_voice* realVoice)
{
    return realVoice->sourceAtCursor && realVoice->appliedSeekEpoch == c89atomic_load_32(&realVoice->voiceRef->seekEpoch);
}

/**
 * @brief Brings the source and the voice's cursor back together before an audible read.
 *
 * A seek always moves the source. Otherwise they part when a pause ramp reads past the cursor, which stops the moment
 * the pause is asked for, or when a new real voice has yet to join its cursor. Once playing, the source goes back to
 * the cursor if the declick is still silent. Part way through a ramp that jump would click, so the cursor moves up to
 * the source instead.
 */
static void sc_real_voice_sync_source_before_read(sc_real_voice* realVoice, sc_bool paused)
{
    if (realVoice->appliedSeekEpoch != c89atomic_load_32(&realVoice->voiceRef->seekEpoch))
    {
        sc_real_voice_move_source_to_cursor(realVoice);
        return;
    }

    if (realVoice->sourceAtCursor || paused)
    {
        return;
    }

    if (realVoice->declickRamp.current <= 0.0F)
    {
        sc_real_voice_move_source_to_cursor(realVoice);
    }
    else if (sc_real_voice_move_cursor_to_source(realVoice))
    {
        realVoice->sourceAtCursor = SC_TRUE;
    }
}

static void sc_real_voice_update_lowpass_if_required(sc_real_voice* realVoice)
{
    ma_bool32 isUpdateRequired = MA_FALSE;
    float newCuttoff = c89atomic_load_f32(&realVoice->voiceRef->lowpassCutoff);

    if (realVoice->voiceRef->oldLowpassCutoff != newCuttoff)
    {
        realVoice->voiceRef->oldLowpassCutoff = newCuttoff;
        isUpdateRequired                      = MA_TRUE;
    }

    if (isUpdateRequired)
    {
        ma_lpf_config newConfig = ma_lpf_config_init(ma_format_f32, realVoice->lowpass.channels, realVoice->lowpass.sampleRate, newCuttoff, SC_DSP_DEFAULT_FILTER_ORDER);
        ma_lpf_reinit(&newConfig, &realVoice->lowpass);
    }
}

static void sc_real_voice_update_highpass_if_required(sc_real_voice* realVoice)
{
    ma_bool32 isUpdateRequired = MA_FALSE;
    float newCuttoff           = c89atomic_load_f32(&realVoice->voiceRef->highpassCutoff);

    if (realVoice->voiceRef->oldHighpassCutoff != newCuttoff)
    {
        realVoice->voiceRef->oldHighpassCutoff = newCuttoff;
        isUpdateRequired                       = MA_TRUE;
    }

    if (isUpdateRequired)
    {
        ma_hpf_config newConfig = ma_hpf_config_init(ma_format_f32, realVoice->lowpass.channels, realVoice->lowpass.sampleRate, newCuttoff, SC_DSP_DEFAULT_FILTER_ORDER);
        ma_hpf_reinit(&newConfig, &realVoice->highpass);
    }
}

static ma_uint64 sc_get_required_input_frames_from_resampler(ma_resampler* resampler, ma_uint64 outputFrameCount, ma_uint64 frameCap)
{
    ma_uint64 requiredInputFrames = 0;
    ma_linear_resampler_get_required_input_frame_count(resampler, outputFrameCount, &requiredInputFrames);
    requiredInputFrames = SC_MIN(requiredInputFrames, frameCap);
    return requiredInputFrames;
}

static void sc_real_voice_process_pcm_frames__general(sc_real_voice* realVoice, const float* framesIn, ma_uint64* frameCountIn, float* framesOut, ma_uint64* frameCountOut, ma_uint32 channelCountIn, ma_uint32 channelCountOut)
{
    if (channelCountIn == 0)
    {
        return;
    }

    ma_uint64 totalFramesProcessedIn  = 0;
    ma_uint64 totalFramesProcessedOut = 0;

    float temp[SC_TEMP_STACK_BUFFER_SIZE / sizeof(float)];
    ma_uint64 tempCapInFrames = SC_COUNTOF(temp) / channelCountIn;

    while (totalFramesProcessedOut < *frameCountOut)
    {
        ma_uint64 inputFramesAvailableThisIteration  = *frameCountIn - totalFramesProcessedIn;
        ma_uint64 outputFramesAvailableThisIteration = SC_MIN(*frameCountOut - totalFramesProcessedOut, tempCapInFrames);

        const float* const runningFramesIn = ma_offset_pcm_frames_const_ptr_f32(framesIn, totalFramesProcessedIn, channelCountIn);
        float* const runningFramesOut      = ma_offset_pcm_frames_ptr_f32(framesOut, totalFramesProcessedOut, channelCountOut);
        const sc_bool useMixMatrix         = realVoice->mixChannelsIn == channelCountIn && realVoice->mixChannelsOut == channelCountOut;
        float* const workingBuffer         = (channelCountIn == channelCountOut && !useMixMatrix) ? runningFramesOut : temp;

        ma_uint64 resampleFrameCountIn  = inputFramesAvailableThisIteration;
        ma_uint64 resampleFrameCountOut = outputFramesAvailableThisIteration;

        if (ma_linear_resampler_process_pcm_frames(&realVoice->resampler, runningFramesIn, &resampleFrameCountIn, workingBuffer, &resampleFrameCountOut) == MA_SUCCESS)
        {
            // Same channel count without a matrix resamples straight into the output.
            if (useMixMatrix)
            {
                sc_apply_mix_matrix_f32(runningFramesOut, workingBuffer, resampleFrameCountOut, realVoice->mixMatrix, channelCountIn, channelCountOut);
            }
            else if (channelCountIn != channelCountOut)
            {
                sc_channel_map_apply_f32(runningFramesOut, NULL, channelCountOut, workingBuffer, NULL, channelCountIn, resampleFrameCountOut, ma_channel_mix_mode_simple, ma_mono_expansion_mode_average);
            }

            totalFramesProcessedIn += resampleFrameCountIn;
            totalFramesProcessedOut += resampleFrameCountOut;

            if (resampleFrameCountOut == 0)
            {
                break;
            }

            // Bypassed at their neutral cutoffs: a 1Hz f32 biquad can't settle and leaves a DC offset.
            const float lowpassCutoff  = c89atomic_load_f32(&realVoice->voiceRef->lowpassCutoff);
            const float highpassCutoff = c89atomic_load_f32(&realVoice->voiceRef->highpassCutoff);

            if (lowpassCutoff < SC_DSP_CUTOFF_MAX && ma_lpf_process_pcm_frames(&realVoice->lowpass, runningFramesOut, runningFramesOut, resampleFrameCountOut) != MA_SUCCESS)
            {
                break;
            }

            if (highpassCutoff > SC_DSP_CUTOFF_MIN && ma_hpf_process_pcm_frames(&realVoice->highpass, runningFramesOut, runningFramesOut, resampleFrameCountOut) != MA_SUCCESS)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    *frameCountIn  = totalFramesProcessedIn;
    *frameCountOut = totalFramesProcessedOut;
}

static void sc_voice_real_process_pcm_frames(ma_node* node, const float** framesIn, ma_uint32* frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{
    (void)framesIn;
    (void)frameCountIn;

    sc_real_voice* const realVoice = (sc_real_voice*)node;

    const sc_voice_state voiceState = (sc_voice_state)c89atomic_load_32(&realVoice->voiceRef->currentState);
    if (voiceState == sc_voice_state_free || voiceState == sc_voice_state_stopped)
    {
        *frameCountOut = 0;
        return;
    }

    // A voice promoted part way through a callback waits for the next one, when its cursor starts moving with it.
    if (realVoice->voiceRef->renderCallback != realVoice->system->callbackIndex)
    {
        *frameCountOut = 0;
        return;
    }

    const sc_bool paused   = realVoice->voiceRef->pausedThisCallback;
    const sc_bool stopping = c89atomic_load_32(&realVoice->voiceRef->desiredState) == (sc_uint32)sc_voice_desired_stopped;

    // Playing from silence at the top of the source plays the first sample untouched; fading in is for the sound designer
    // to ask for. Anywhere else, like joining after playing virtually or resuming, it ramps in.
    if (!paused && sc_ramp_is_silent(&realVoice->declickRamp) && c89atomic_load_64(&realVoice->voiceRef->playCursor) == 0u)
    {
        sc_ramp_reset(&realVoice->declickRamp, 1.0F);
    }

    sc_real_voice_sync_gains(realVoice, paused);

    // A voice ramped to silence reads nothing. While paused it moves its source to its cursor, so resuming carries on
    // exactly where the pause was asked for and a stream has the rest of the pause to refill.
    const sc_bool declickedOut = sc_ramp_is_silent(&realVoice->declickRamp);
    if (declickedOut || (stopping && sc_ramp_is_silent(&realVoice->fadeRamp)))
    {
        if (stopping)
        {
            c89atomic_store_32(&realVoice->silent, 1u);
        }
        else if (paused && !sc_real_voice_is_source_at_cursor(realVoice))
        {
            sc_real_voice_move_source_to_cursor(realVoice);
        }
        *frameCountOut = 0;
        return;
    }

    ma_data_source* const dataSource = realVoice->dataSource;

    ma_format dataSourceFormat;
    ma_uint32 dataSourceChannels;
    ma_uint32 dataSourceSampleRate;

    const ma_uint32 outputChannelCount = ma_engine_get_channels(&realVoice->system->engine);

    // The resampler and mix matrix are sized for the channel count the real voice was created with.
    if (ma_data_source_get_data_format(dataSource, &dataSourceFormat, &dataSourceChannels, &dataSourceSampleRate, NULL, 0) == MA_SUCCESS && dataSourceChannels == realVoice->sourceChannels && dataSourceFormat != ma_format_unknown)
    {
        sc_real_voice_update_pitch_if_required(realVoice, dataSourceSampleRate);
        sc_real_voice_sync_loop_if_needed(realVoice);
        sc_real_voice_sync_source_before_read(realVoice, paused);
        sc_real_voice_sync_mix_if_needed(realVoice);
        sc_real_voice_update_highpass_if_required(realVoice);
        sc_real_voice_update_lowpass_if_required(realVoice);

        ma_uint8 temp[SC_TEMP_STACK_BUFFER_SIZE];
        const ma_uint64 tempCapInFrames = sizeof(temp) / ma_get_bytes_per_frame(dataSourceFormat, dataSourceChannels);

        ma_uint64 totalFramesRead = 0;

        while (totalFramesRead < *frameCountOut)
        {
            const ma_uint64 framesToReadThisIteration = *frameCountOut - totalFramesRead;

            ma_uint64 framesReadThisIteration;

            ma_uint64 cursorBeforeRead     = 0;
            const sc_bool knowsCursor      = ma_data_source_get_cursor_in_pcm_frames(dataSource, &cursorBeforeRead) == MA_SUCCESS;

            const ma_result readResult = ma_data_source_read_pcm_frames(dataSource, temp, sc_get_required_input_frames_from_resampler(&realVoice->resampler, framesToReadThisIteration, tempCapInFrames), &framesReadThisIteration);

            if (knowsCursor)
            {
                sc_real_voice_sync_cursor_if_looped(realVoice, cursorBeforeRead, framesReadThisIteration, dataSourceSampleRate);
            }

            if (readResult == MA_AT_END)
            {
                // "We have no more data. Please stop us on the next update"
                c89atomic_store_32(&realVoice->voiceRef->desiredState, (sc_uint32)sc_voice_desired_stopped);
            }

            float* const runningFramesOut = ma_offset_pcm_frames_ptr_f32(framesOut[0], totalFramesRead, outputChannelCount);

            const sc_bool canProcessInPlace = dataSourceFormat == ma_format_f32;

            ma_uint64 inputFrameCount  = framesReadThisIteration;
            ma_uint64 outputFrameCount = framesToReadThisIteration;

            if (canProcessInPlace)
            {
                sc_real_voice_process_pcm_frames__general(realVoice, temp, &inputFrameCount, runningFramesOut, &outputFrameCount, dataSourceChannels, outputChannelCount);
            }
            else
            {
                float tempf32[SC_TEMP_STACK_BUFFER_SIZE];
                ma_convert_pcm_frames_format(tempf32, ma_format_f32, temp, dataSourceFormat, framesReadThisIteration, dataSourceChannels, ma_dither_mode_none);

                sc_real_voice_process_pcm_frames__general(realVoice, tempf32, &inputFrameCount, runningFramesOut, &outputFrameCount, dataSourceChannels, outputChannelCount);
            }

            totalFramesRead += outputFrameCount;

            if (framesReadThisIteration == 0)
            {
                break;
            }

            if (readResult != MA_SUCCESS)
            {
                break;
            }
        }

        sc_real_voice_apply_gain(realVoice, framesOut[0], totalFramesRead, outputChannelCount);

        // The cursor holds still while paused, so anything the ramp out read puts the source ahead of it.
        if (paused && totalFramesRead > 0)
        {
            realVoice->sourceAtCursor = SC_FALSE;
        }

        // A source that ran out has nothing left to fade, so it's already silent.
        if (stopping && (sc_ramp_is_silent(&realVoice->fadeRamp) || sc_ramp_is_silent(&realVoice->declickRamp) || totalFramesRead < *frameCountOut))
        {
            c89atomic_store_32(&realVoice->silent, 1u);
        }

        *frameCountOut = (ma_uint32)totalFramesRead;
    }
    else
    {
        if (stopping)
        {
            c89atomic_store_32(&realVoice->silent, 1u);
        }
        *frameCountOut = 0;
    }
}

static ma_node_vtable s_realVoiceVtable =
{
    sc_voice_real_process_pcm_frames,
    NULL,
    0,  
    1,  // One output. Maybe more in the future for aux busses?
    0   // Default
};

typedef struct
{
    size_t sizeInBytes;
    size_t baseNodeOffset;
    size_t resamplerOffset;
    size_t mixMatrixOffset;
    size_t lowpassOffset;
    size_t highpassOffset;
} sc_real_voice_heap_layout;

/**
 * @brief Channels the resampler runs at. It resamples the source before its channels are mapped to the output, so
 * this is the source's count; sizing and initialising the resampler must agree on it.
 */
static ma_uint32 sc_real_voice_get_source_channels(const sc_real_voice_config* config, ma_uint32 fallbackChannels)
{
    ma_uint32 channels = 0;
    if (config->voiceRef != NULL && config->voiceRef->sound != NULL &&
        ma_data_source_get_data_format(config->voiceRef->sound->dataSource, NULL, &channels, NULL, NULL, 0) == MA_SUCCESS && channels > 0)
    {
        return channels;
    }
    return fallbackChannels;
}

static sbk_status sc_real_voice_get_heap_layout(const sc_real_voice_config* config, sc_real_voice_heap_layout* heapLayout)
{
    SC_ASSERT(heapLayout);
    SC_ZERO_OBJECT(heapLayout);

    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->system != NULL);

    size_t tempHeapSize;
    ma_uint32 channelsIn;
    ma_uint32 channelsOut;

    heapLayout->sizeInBytes = 0;

    channelsIn  = (config->channelsIn != 0) ? config->channelsIn : ma_engine_get_channels(&config->system->engine);
    channelsOut = (config->channelsOut != 0) ? config->channelsOut : ma_engine_get_channels(&config->system->engine);

    ma_node_config nodeConfig  = ma_node_config_init();
    nodeConfig.vtable          = &s_realVoiceVtable;
    nodeConfig.pInputChannels  = NULL;  /* No input buses. */
    nodeConfig.pOutputChannels = &channelsOut;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_node_get_heap_size(&config->system->engine.nodeGraph, &nodeConfig, &tempHeapSize)));

    heapLayout->baseNodeOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_linear_resampler_config resamplerConfig;
    resamplerConfig          = ma_linear_resampler_config_init(ma_format_f32, sc_real_voice_get_source_channels(config, channelsIn), 1, 1);
    resamplerConfig.lpfOrder = SC_RESAMPLER_LPF_ORDER;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_linear_resampler_get_heap_size(&resamplerConfig, &tempHeapSize)));

    heapLayout->resamplerOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    heapLayout->mixMatrixOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(sizeof(float) * sc_real_voice_get_source_channels(config, channelsIn) * channelsOut);

    // The filters run after the source is mapped to the output.
    ma_lpf_config lowpassConfig = ma_lpf_config_init(ma_format_f32, channelsOut, 1, SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_lpf_get_heap_size(&lowpassConfig, &tempHeapSize)));

    heapLayout->lowpassOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_hpf_config highpassConfig = ma_hpf_config_init(ma_format_f32, channelsOut, 1, SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_hpf_get_heap_size(&highpassConfig, &tempHeapSize)));

    heapLayout->highpassOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    return SBK_SUCCESS;
}

static sbk_status sc_real_voice_get_heap_size(const sc_real_voice_config* config, size_t* heapSizeInBytes)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(heapSizeInBytes != NULL);

    sc_real_voice_heap_layout heapLayout;

    *heapSizeInBytes = 0;

    SC_CHECK_STATUS(sc_real_voice_get_heap_layout(config, &heapLayout));

    *heapSizeInBytes = heapLayout.sizeInBytes;

    return SBK_SUCCESS;
}

sbk_status sc_real_voice_init(const sc_real_voice_config* config, sc_real_voice* realVoice)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->system != NULL);
    SC_CHECK_ARG(realVoice != NULL);

    size_t heapSizeInBytes = 0;
    void* heap             = NULL;

    SC_CHECK_STATUS(sc_real_voice_get_heap_size(config, &heapSizeInBytes));

    if (heapSizeInBytes > 0)
    {
        SC_CALLOC(heap, heapSizeInBytes, config->system);
    }    

    SC_CHECK_STATUS_ELSE_FREE(sc_real_voice_init_preallocated(config, heap, realVoice), heap, config->system);

    realVoice->ownsHeap = SC_TRUE;

    return SBK_SUCCESS;
}

sbk_status sc_real_voice_init_preallocated(const sc_real_voice_config* config, void* heap, sc_real_voice* realVoice)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->system != NULL);
    SC_CHECK_ARG(config->voiceRef != NULL);
    SC_CHECK_ARG(config->voiceRef->sound != NULL);
    SC_CHECK_ARG(heap != NULL);
    SC_CHECK_ARG(realVoice != NULL);

    SC_CHECK(realVoice->nodeGroup == NULL, SBK_ERR_ALREADY_INITIALIZED);
    SC_CHECK(realVoice->dataSource == NULL, SBK_ERR_ALREADY_INITIALIZED);

    MA_ZERO_OBJECT(realVoice);

    realVoice->system   = config->system;
    realVoice->voiceRef = config->voiceRef;

    sc_real_voice_heap_layout heapLayout;
    SC_CHECK_STATUS(sc_real_voice_get_heap_layout(config, &heapLayout));

    realVoice->heap = heap;
    SC_ZERO_MEMORY(heap, heapLayout.sizeInBytes);

    SC_CREATE(realVoice->dataSource, ma_resource_manager_data_source, config->system);

    const ma_uint32 channelsIn = (config->channelsIn != 0) ? config->channelsIn : ma_engine_get_channels(&config->system->engine);
    const ma_uint32 channelsOut = (config->channelsOut != 0) ? config->channelsOut : ma_engine_get_channels(&config->system->engine);

    ma_node_config baseNodeConfig  = ma_node_config_init();
    baseNodeConfig.vtable          = &s_realVoiceVtable;
    baseNodeConfig.pInputChannels  = NULL;  /* No input buses. */
    baseNodeConfig.pOutputChannels = &channelsOut;

    const sc_sound* const sound = config->voiceRef->sound;
    if (sound->mode & SC_SOUND_MODE_STREAM)
    {
        // miniaudio can only copy fully loaded buffers, so each streamed voice reads the source itself.
        SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_resource_manager_data_source_init(&config->system->resourceManager, sound->sourceName, MA_RESOURCE_MANAGER_DATA_SOURCE_FLAG_STREAM, NULL, realVoice->dataSource)), error1);
    }
    else
    {
        SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_resource_manager_data_source_init_copy(&config->system->resourceManager, sound->dataSource, realVoice->dataSource)), error1);
    }

    realVoice->appliedLoopEpoch = 0u;
    realVoice->appliedSeekEpoch = 0u;
    realVoice->sourceAtCursor   = SC_FALSE;
    sc_real_voice_sync_loop_if_needed(realVoice);

    // The declick holds at silence until the voice first plays unpaused, when it's set by where the voice's cursor is.
    sc_ramp_reset(&realVoice->declickRamp, 0.0F);
    sc_ramp_reset(&realVoice->volumeRamp, c89atomic_load_f32(&config->voiceRef->gain));
    sc_ramp_reset(&realVoice->fadeRamp, c89atomic_load_f32(&config->voiceRef->fadeGain));
    realVoice->fadeSyncedCallback = 0u;
    c89atomic_store_32(&realVoice->silent, 0u);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_node_init_preallocated(&config->system->engine.nodeGraph, &baseNodeConfig, SC_OFFSET_PTR(heap, heapLayout.baseNodeOffset), &realVoice->baseNode)), error2);

    const ma_uint32 engineSampleRate = ma_engine_get_sample_rate(&config->system->engine);

    ma_uint32 dataSourceSampleRate = 0;
    if (ma_data_source_get_data_format(realVoice->voiceRef->sound->dataSource, NULL, NULL, &dataSourceSampleRate, NULL, 0) != MA_SUCCESS || dataSourceSampleRate == 0)
    {
        dataSourceSampleRate = engineSampleRate;
    }

    const ma_uint32 dataSourceChannels = sc_real_voice_get_source_channels(config, channelsIn);

    realVoice->sourceChannels = dataSourceChannels;
    realVoice->outputChannels = channelsOut;
    realVoice->mixMatrix      = (float*)SC_OFFSET_PTR(heap, heapLayout.mixMatrixOffset);

    // Linear interpolation alone doesn't anti-alias, so ultrasonic content in high rate sources would fold into the audible
    // band. Matching rates skip the filter to stay bit-transparent.
    ma_linear_resampler_config resamplerConfig = ma_linear_resampler_config_init(ma_format_f32, dataSourceChannels, dataSourceSampleRate, engineSampleRate);
    resamplerConfig.lpfOrder                   = dataSourceSampleRate != engineSampleRate ? SC_RESAMPLER_LPF_ORDER : 0;
    resamplerConfig.lpfNyquistFactor           = SC_RESAMPLER_LPF_NYQUIST_FACTOR;

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_linear_resampler_init_preallocated(&resamplerConfig, SC_OFFSET_PTR(heap, heapLayout.resamplerOffset), &realVoice->resampler)), error3);

    ma_lpf_config lowpassConfig = ma_lpf_config_init(ma_format_f32, channelsOut, engineSampleRate, SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_lpf_init_preallocated(&lowpassConfig, SC_OFFSET_PTR(heap, heapLayout.lowpassOffset), &realVoice->lowpass)), error4);

    ma_hpf_config highpassConfig = ma_hpf_config_init(ma_format_f32, channelsOut, engineSampleRate, SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_hpf_init_preallocated(&highpassConfig, SC_OFFSET_PTR(heap, heapLayout.highpassOffset), &realVoice->highpass)), error5);

    realVoice->nodeGroup = config->voiceRef->voiceNodeGroup;

    return SBK_SUCCESS;

error5:
    ma_lpf_uninit(&realVoice->lowpass, NULL);
error4:
    ma_linear_resampler_uninit(&realVoice->resampler, NULL);
error3:
    ma_node_uninit(&realVoice->baseNode, NULL);
error2:
    ma_resource_manager_data_source_uninit(realVoice->dataSource);
error1:
    SC_FREE(realVoice->dataSource, config->system);
    return SBK_ERR_CHEF;
}

sbk_status sc_real_voice_uninit(sc_real_voice* realVoice)
{
    SC_CHECK_ARG(realVoice != NULL);
    SC_CHECK(realVoice->voiceRef != NULL, SBK_SUCCESS);

    ma_node_uninit(&realVoice->baseNode, &realVoice->system->engine.allocationCallbacks);

    if (realVoice->nodeGroup != NULL && realVoice->nodeGroup->head != NULL && realVoice->nodeGroup->head->node != NULL)
    {
        ma_node_detach_output_bus(realVoice->nodeGroup->head->node, 0);
    }
    realVoice->nodeGroup = NULL;

    ma_linear_resampler_uninit(&realVoice->resampler, &realVoice->system->engine.allocationCallbacks);

    if (realVoice->dataSource)
    {
        ma_resource_manager_data_source_uninit(realVoice->dataSource);
        SC_FREE(realVoice->dataSource, realVoice->system);
    }

    if (realVoice->ownsHeap)
    {
        SC_FREE(realVoice->heap, realVoice->system);
    }

    realVoice->voiceRef = NULL;

    return SBK_SUCCESS;
}

sc_real_voice_config sc_real_voice_config_init(sc_system* system, sc_voice* voiceRef, sc_uint32 channelsIn, sc_uint32 channelsOut)
{
    sc_real_voice_config config;
    SC_ZERO_OBJECT(&config);
    config.system = system;
    config.voiceRef = voiceRef;
    config.channelsIn = channelsIn;
    config.channelsOut = channelsOut;
    return config;
}

sbk_status sc_system_promote_voice_to_real(sc_system* system, sc_voice* voice)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(voice != NULL);

    // The real voice is sized from the sound's format, so an async sound waits until its header has been read.
    ma_uint32 soundChannels = 0;
    if (ma_data_source_get_data_format(voice->sound->dataSource, NULL, &soundChannels, NULL, NULL, 0) != MA_SUCCESS || soundChannels == 0)
    {
        return SC_STATUS_FROM_MA_RESULT(MA_BUSY);
    }

    sc_voice_handle realVoiceSlot       = 0;
    const ma_result realVoiceSlotResult = ma_slot_allocator_alloc(&system->realVoiceSlotAllocator, &realVoiceSlot);

    if (realVoiceSlotResult != MA_SUCCESS)
    {
        // If we couldn't get a real voice, we must be virtual
        ma_log_post(&system->log, MA_LOG_LEVEL_WARNING, "Real voice was requested but there are no open slots available");
        (void)sc_voice_set_virtual(system, (sc_voice_handle)c89atomic_load_64(&voice->handle), SC_TRUE);
        voice->realVoiceHandle = 0;
        return SBK_SUCCESS;
    }

    const sc_voice_index realVoiceIndex = SC_VOICE_HANDLE_EXTRACT_INDEX(realVoiceSlot);
    sc_real_voice* const realVoice      = &system->realVoiceBuffer[realVoiceIndex];

    const sc_real_voice_config voiceConfig = sc_real_voice_config_init(system, voice, 0, 0);

    const sbk_status initResult = sc_real_voice_init(&voiceConfig, realVoice);
    if (initResult != SBK_SUCCESS)
    {
        ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Failed to initialise a real voice (status %d); releasing slot", (int)initResult);
        ma_slot_allocator_free(&system->realVoiceSlotAllocator, realVoiceSlot);
        c89atomic_store_32(&voice->desiredState, SC_FALSE);
        return initResult;
    }

    // Per-voice effects will slot into this group later.
    const ma_result attachOutputBusResult = ma_node_attach_output_bus(&realVoice->baseNode, 0, realVoice->nodeGroup->tail->node, 0);
    if (attachOutputBusResult != MA_SUCCESS)
    {
        ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Failed to attach real voice to its internal node group: %s", ma_result_description(attachOutputBusResult));
        (void)sc_real_voice_uninit(realVoice);
        ma_slot_allocator_free(&system->realVoiceSlotAllocator, realVoiceSlot);
        return SC_STATUS_FROM_MA_RESULT(attachOutputBusResult);
    }

    // sc_node_group_init already routed the internal group to master. Override
    // that when the caller supplied a specific parent group.
    if (voice->parentNodeGroup != NULL && voice->parentNodeGroup != system->masterNodeGroup)
    {
        if (voice->parentNodeGroup->tail == NULL || voice->parentNodeGroup->tail->node == NULL)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Caller-supplied parent group has no tail node (group=%p tail=%p tail_node=%p)", (void*)voice->parentNodeGroup, (void*)voice->parentNodeGroup->tail, (void*)(voice->parentNodeGroup->tail ? voice->parentNodeGroup->tail->node : NULL));
            (void)sc_real_voice_uninit(realVoice);
            ma_slot_allocator_free(&system->realVoiceSlotAllocator, realVoiceSlot);
            return SBK_ERR_NULL;
        }

        if (realVoice->nodeGroup->head == NULL || realVoice->nodeGroup->head->node == NULL)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Real voice internal group has no head node (group=%p head=%p head_node=%p)", (void*)realVoice->nodeGroup, (void*)realVoice->nodeGroup->head, (void*)(realVoice->nodeGroup->head ? realVoice->nodeGroup->head->node : NULL));
            (void)sc_real_voice_uninit(realVoice);
            ma_slot_allocator_free(&system->realVoiceSlotAllocator, realVoiceSlot);
            return SBK_ERR_NULL;
        }

        const ma_result setParentResult = ma_node_attach_output_bus(realVoice->nodeGroup->head->node, 0, voice->parentNodeGroup->tail->node, 0);
        if (setParentResult != MA_SUCCESS)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Failed to route real voice to caller-supplied parent group: %s (internal head=%p caller tail=%p)", ma_result_description(setParentResult), (void*)realVoice->nodeGroup->head->node, (void*)voice->parentNodeGroup->tail->node);
            (void)sc_real_voice_uninit(realVoice);
            ma_slot_allocator_free(&system->realVoiceSlotAllocator, realVoiceSlot);
            return SC_STATUS_FROM_MA_RESULT(setParentResult);
        }
    }

    voice->realVoiceHandle = realVoiceSlot;
    return SBK_SUCCESS;
}