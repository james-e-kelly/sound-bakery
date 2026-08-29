#include "sound_chef/sound_chef.h"

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

sbk_status sc_voice_set_paused(sc_system* system, sc_voice_handle handle, sc_bool paused)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_PAUSED, paused);
    return SBK_SUCCESS;
}

sbk_status SC_API sc_voice_set_virtual(sc_voice* voice, sc_bool virtualised)
{
    SC_CHECK_ARG(voice != NULL);
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_VIRTUAL, virtualised);
    return SBK_SUCCESS;
}

sbk_status sc_voice_stop(sc_system* system, sc_voice_handle handle)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_32(&voice->desiredState, (sc_uint32)sc_voice_desired_stopped);
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
        float* const workingBuffer         = channelCountIn == channelCountOut ? runningFramesOut : temp;

        ma_uint64 resampleFrameCountIn  = inputFramesAvailableThisIteration;
        ma_uint64 resampleFrameCountOut = outputFramesAvailableThisIteration;

        if (ma_linear_resampler_process_pcm_frames(&realVoice->resampler, runningFramesIn, &resampleFrameCountIn, workingBuffer, &resampleFrameCountOut) == MA_SUCCESS)
        {
            const float volume = c89atomic_load_f32(&realVoice->voiceRef->gain);

            if (channelCountIn == channelCountOut)
            {
                ma_copy_and_apply_volume_factor_f32(runningFramesOut, workingBuffer, resampleFrameCountOut * channelCountOut, volume);
            }
            else
            {
                sc_channel_map_apply_f32(runningFramesOut, NULL, channelCountOut, workingBuffer, NULL, channelCountIn, resampleFrameCountOut, ma_channel_mix_mode_simple, ma_mono_expansion_mode_average);
                ma_apply_volume_factor_f32(runningFramesOut, resampleFrameCountOut * channelCountOut, volume);
            }

            totalFramesProcessedIn += resampleFrameCountIn;
            totalFramesProcessedOut += resampleFrameCountOut;

            if (resampleFrameCountOut == 0)
            {
                break;
            }

            if (ma_lpf_process_pcm_frames(&realVoice->lowpass, runningFramesOut, runningFramesOut, resampleFrameCountOut) == MA_SUCCESS)
            {
                if (ma_hpf_process_pcm_frames(&realVoice->highpass, runningFramesOut, runningFramesOut, resampleFrameCountOut) == MA_SUCCESS)
                {

                }
                else
                {
                    break;
                }
            }
            else
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

    if (SC_VOICE_HAS_FLAG(c89atomic_load_32(&realVoice->voiceRef->flags), SC_VOICE_FLAG_PAUSED))
    {
        *frameCountOut = 0;
        return;
    }

    ma_data_source* const dataSource = realVoice->dataSource;

    ma_format dataSourceFormat;
    ma_uint32 dataSourceChannels;
    ma_uint32 dataSourceSampleRate;

    const ma_uint32 outputChannelCount = ma_engine_get_channels(&realVoice->system->engine);

    if (ma_data_source_get_data_format(dataSource, &dataSourceFormat, &dataSourceChannels, &dataSourceSampleRate, NULL, 0) == MA_SUCCESS && dataSourceChannels > 0 && dataSourceFormat != ma_format_unknown)
    {
        sc_real_voice_update_pitch_if_required(realVoice, dataSourceSampleRate);
        sc_real_voice_sync_loop_if_needed(realVoice);
        sc_real_voice_update_highpass_if_required(realVoice);
        sc_real_voice_update_lowpass_if_required(realVoice);

        ma_uint8 temp[SC_TEMP_STACK_BUFFER_SIZE];
        const ma_uint64 tempCapInFrames = sizeof(temp) / ma_get_bytes_per_frame(dataSourceFormat, dataSourceChannels);

        ma_uint64 totalFramesRead = 0;

        while (totalFramesRead < *frameCountOut)
        {
            const ma_uint64 framesToReadThisIteration = *frameCountOut - totalFramesRead;

            ma_uint64 framesReadThisIteration;

            const ma_result readResult = ma_data_source_read_pcm_frames(dataSource, temp, sc_get_required_input_frames_from_resampler(&realVoice->resampler, framesToReadThisIteration, tempCapInFrames), &framesReadThisIteration);

            if (readResult == MA_AT_END)
            {
                // "We have no more data. Please stop us on the next update"
                c89atomic_store_32(&realVoice->voiceRef->desiredState, (sc_uint32)sc_voice_desired_stopped); 
            }

            float* const runningFramesOut = ma_offset_pcm_frames_ptr_f32(framesOut[0], totalFramesRead, outputChannelCount);

            const sc_bool canProcessInPlace = dataSourceFormat == ma_format_f32;

            if (canProcessInPlace)
            {
                sc_real_voice_process_pcm_frames__general(realVoice, temp, &framesReadThisIteration, runningFramesOut, &framesReadThisIteration, dataSourceChannels, outputChannelCount);
            }
            else
            {
                float tempf32[SC_TEMP_STACK_BUFFER_SIZE];  // Do not do `MA_DATA_CONVERTER_STACK_BUFFER_SIZE/sizeof(float)` here like we've done in other places
                ma_convert_pcm_frames_format(tempf32, ma_format_f32, temp, dataSourceFormat, framesReadThisIteration, dataSourceChannels, ma_dither_mode_none);

                sc_real_voice_process_pcm_frames__general(realVoice, tempf32, &framesReadThisIteration, runningFramesOut, &framesReadThisIteration, dataSourceChannels, outputChannelCount);
            }

            totalFramesRead += framesReadThisIteration;

            if (framesReadThisIteration == 0)
            {
                break;
            }

            if (readResult != MA_SUCCESS)
            {
                break;
            }
        }

        *frameCountOut = (ma_uint32)totalFramesRead;
    }
    else
    {
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
    size_t gainerOffset;
    size_t lowpassOffset;
    size_t highpassOffset;
} sc_real_voice_heap_layout;

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
    resamplerConfig          = ma_linear_resampler_config_init(ma_format_f32, channelsIn, 1, 1);
    resamplerConfig.lpfOrder = 0;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_linear_resampler_get_heap_size(&resamplerConfig, &tempHeapSize)));

    heapLayout->resamplerOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_gainer_config gainerConfig = ma_gainer_config_init(channelsIn, 1);

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_gainer_get_heap_size(&gainerConfig, &tempHeapSize)));
    
    heapLayout->gainerOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_lpf_config lowpassConfig = ma_lpf_config_init(ma_format_f32, channelsIn, 1, SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_lpf_get_heap_size(&lowpassConfig, &tempHeapSize)));

    heapLayout->lowpassOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_hpf_config highpassConfig = ma_hpf_config_init(ma_format_f32, channelsIn, 1, SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);

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
    SC_CHECK_ARG(config->voiceRef != NULL);
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

    /// @todo Handle copying streams
    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_resource_manager_data_source_init_copy(&config->system->resourceManager, config->voiceRef->sound->dataSource, realVoice->dataSource)), error1);

    realVoice->appliedLoopEpoch = 0u;
    sc_real_voice_sync_loop_if_needed(realVoice);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_node_init_preallocated(&config->system->engine.nodeGraph, &baseNodeConfig, SC_OFFSET_PTR(heap, heapLayout.baseNodeOffset), &realVoice->baseNode)), error2);

    const ma_uint32 engineSampleRate = ma_engine_get_sample_rate(&config->system->engine);

    ma_uint32 dataSourceSampleRate = 0;
    ma_uint32 dataSourceChannels   = 0;
    if (ma_data_source_get_data_format(realVoice->voiceRef->sound->dataSource, NULL, &dataSourceChannels, &dataSourceSampleRate, NULL, 0) != MA_SUCCESS)
    {
        dataSourceSampleRate = 0;
        dataSourceChannels   = 0;
    }

    if (dataSourceSampleRate == 0) 
    { 
        dataSourceSampleRate = engineSampleRate;
    }

    if (dataSourceChannels == 0)   
    { 
        dataSourceChannels = channelsIn;
    }

    ma_linear_resampler_config resamplerConfig = ma_linear_resampler_config_init(ma_format_f32, dataSourceChannels, dataSourceSampleRate, engineSampleRate);
    resamplerConfig.lpfOrder                   = 0;

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_linear_resampler_init_preallocated(&resamplerConfig, SC_OFFSET_PTR(heap, heapLayout.resamplerOffset), &realVoice->resampler)), error3);

    ma_fader_config faderConfig = ma_fader_config_init(ma_format_f32, channelsIn, engineSampleRate);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_fader_init(&faderConfig, &realVoice->fader)), error4);

    ma_gainer_config gainerConfig = ma_gainer_config_init(channelsIn, 1);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_gainer_init_preallocated(&gainerConfig, SC_OFFSET_PTR(heap, heapLayout.gainerOffset), &realVoice->gainer)), error5);

    ma_lpf_config lowpassConfig = ma_lpf_config_init(ma_format_f32, channelsIn, engineSampleRate, SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_lpf_init_preallocated(&lowpassConfig, SC_OFFSET_PTR(heap, heapLayout.lowpassOffset), &realVoice->lowpass)), error6);

    ma_hpf_config highpassConfig = ma_hpf_config_init(ma_format_f32, channelsIn, engineSampleRate, SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);

    SC_CHECK_STATUS_ELSE_GOTO(SC_STATUS_FROM_MA_RESULT(ma_hpf_init_preallocated(&highpassConfig, SC_OFFSET_PTR(heap, heapLayout.highpassOffset), &realVoice->highpass)), error7);
    
    SC_CREATE_ELSE_GOTO(realVoice->nodeGroup, sc_node_group, config->system, error8);
    SC_CHECK_STATUS_ELSE_GOTO(sc_node_group_init(config->system, realVoice->nodeGroup), error9);

    return SBK_SUCCESS;

error9:
    SC_FREE(realVoice->nodeGroup, config->system);
error8:
    ma_hpf_uninit(&realVoice->highpass, NULL);
        error7:
    ma_lpf_uninit(&realVoice->lowpass, NULL);
error6:
    ma_gainer_uninit(&realVoice->gainer, NULL);
error5:
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

    if (realVoice->nodeGroup != NULL)
    {
        (void)sc_node_group_release(realVoice->nodeGroup);
        realVoice->nodeGroup = NULL;
    }

    ma_gainer_uninit(&realVoice->gainer, &realVoice->system->engine.allocationCallbacks);
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

    sc_voice_handle realVoiceSlot       = 0;
    const ma_result realVoiceSlotResult = ma_slot_allocator_alloc(&system->realVoiceSlotAllocator, &realVoiceSlot);

    if (realVoiceSlotResult != MA_SUCCESS)
    {
        // If we couldn't get a real voice, we must be virtual
        ma_log_post(&system->log, MA_LOG_LEVEL_WARNING, "Real voice was requested but there are no open slots available");
        sc_voice_set_virtual(voice, SC_TRUE);
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
    if (voice->group != NULL && voice->group != system->masterNodeGroup)
    {
        if (voice->group->tail == NULL || voice->group->tail->node == NULL)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Caller-supplied parent group has no tail node (group=%p tail=%p tail_node=%p)", (void*)voice->group, (void*)voice->group->tail, (void*)(voice->group->tail ? voice->group->tail->node : NULL));
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

        const ma_result setParentResult = ma_node_attach_output_bus(realVoice->nodeGroup->head->node, 0, voice->group->tail->node, 0);
        if (setParentResult != MA_SUCCESS)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_ERROR, "Failed to route real voice to caller-supplied parent group: %s (internal head=%p caller tail=%p)", ma_result_description(setParentResult), (void*)realVoice->nodeGroup->head->node, (void*)voice->group->tail->node);
            (void)sc_real_voice_uninit(realVoice);
            ma_slot_allocator_free(&system->realVoiceSlotAllocator, realVoiceSlot);
            return SC_STATUS_FROM_MA_RESULT(setParentResult);
        }
    }

    voice->realVoiceHandle = realVoiceSlot;
    return SBK_SUCCESS;
}