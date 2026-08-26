#include "sound_chef/sound_chef.h"

static float sc_voice_calculate_audibility(sc_voice* voice)
{
    // Placeholder: extend with 3D falloff, listener distance, group gain,
    // obstruction, occlusion once those layers exist.
    return c89atomic_load_f32((volatile sc_atomic_float*)&voice->gain);
}

static void sc_virtual_voice_sort_boundary(sc_virtual_voice_candidate* boundary, sc_uint32 count, sc_voice_tiebreak_policy policy)
{
    for (sc_uint32 i = 1; i < count; ++i)
    {
        const sc_virtual_voice_candidate pivot = boundary[i];
        sc_uint32 j                            = i;

        while (j > 0)
        {
            const sc_virtual_voice_candidate* const prev = &boundary[j - 1];

            if (prev->audibility > pivot.audibility)
            {
                break;
            }

            if (prev->audibility == pivot.audibility)
            {
                // kill_oldest -> newer wins -> smaller playCursor comes first
                // kill_newest -> older wins -> larger playCursor comes first
                const sc_bool prevWinsAge = (policy == sc_voice_tiebreak_kill_oldest) ? (prev->playCursor < pivot.playCursor) : (prev->playCursor > pivot.playCursor);
                if (prevWinsAge)
                {
                    break;
                }
            }

            boundary[j] = boundary[j - 1];
            --j;
        }
        boundary[j] = pivot;
    }
}

sbk_status sc_system_calculate_virtual_voices(sc_system* system)
{
    SC_CHECK_ARG(system != NULL);

    const sc_uint32 maxVoices                    = system->voiceSlotAllocator.capacity;
    const sc_uint32 maxRealVoices                = system->realVoiceSlotAllocator.capacity;
    sc_virtual_voice_candidate* const candidates = system->virtualizeCandidates;
    sc_virtual_voice_candidate* const boundary   = system->virtualizeBoundary;

    sc_uint32 histogram[256];
    SC_ZERO_OBJECT(&histogram);
    sc_uint32 count = 0;

    // Pass 1 - gather. vol0 candidates auto-virtualise
    for (sc_uint32 i = 0; i < maxVoices; ++i)
    {
        sc_voice* const voice                = &system->voiceBuffer[i];
        const sc_voice_desired_state desired = (sc_voice_desired_state)c89atomic_load_32(&voice->desiredState);
        const sc_voice_state current         = (sc_voice_state)c89atomic_load_32(&voice->currentState);

        if (desired != sc_voice_desired_playing || current == sc_voice_state_free || current == sc_voice_state_stopping || current == sc_voice_state_stopped)
        {
            continue;
        }

        const float audibility = sc_voice_calculate_audibility(voice);

        if (audibility < system->vol0Threshold)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_DEBUG, "[voice] virtualized (below vol0): slot=%u audibility=%.4f\n", i, audibility);
            (void)sc_voice_set_virtual(voice, SC_TRUE);
            continue;
        }

        sc_virtual_voice_candidate* const candidate = &candidates[count++];
        candidate->voiceIndex                       = i;
        candidate->priority                         = (sc_uint32)voice->priority;
        candidate->audibility                       = audibility;
        candidate->playCursor                       = c89atomic_load_64(&voice->playCursor);
        ++histogram[candidate->priority];
    }

    // Fast path - no contention, promote all candidates to real
    if (count <= maxRealVoices)
    {
        for (sc_uint32 i = 0; i < count; ++i)
        {
            (void)sc_voice_set_virtual(&system->voiceBuffer[candidates[i].voiceIndex], SC_FALSE);
        }
        return SBK_SUCCESS;
    }

    ma_log_postf(&system->log, MA_LOG_LEVEL_DEBUG, "[voice] virtualize: %u candidates for %u real slots\n", count, maxRealVoices);

    // Pass 2 - walk histogram top-down to find the priority cutoff and how many candidates at exactly `cutoff` still fit
    sc_uint32 remaining = maxRealVoices;
    sc_uint32 cutoff    = 0;
    sc_uint32 tieBudget = 0;
    for (sc_int32 s = 255; s >= 0; --s)
    {
        if (histogram[s] >= remaining)
        {
            cutoff    = (sc_uint32)s;
            tieBudget = remaining;
            break;
        }
        remaining -= histogram[s];
    }

    // Pass 3 - split candidates into winners (>cutoff), losers (<cutoff),
    // and the boundary set (==cutoff). Winners/losers get their flag now;
    // boundary is deferred to the sort.
    sc_uint32 boundaryCount = 0;
    for (sc_uint32 i = 0; i < count; ++i)
    {
        const sc_virtual_voice_candidate* const candidate = &candidates[i];
        if (candidate->priority > cutoff)
        {
            (void)sc_voice_set_virtual(&system->voiceBuffer[candidate->voiceIndex], SC_FALSE);
        }
        else if (candidate->priority < cutoff)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_DEBUG, "[voice] virtualized (low priority): slot=%u priority=%u cutoff=%u\n", candidate->voiceIndex, candidate->priority, cutoff);
            (void)sc_voice_set_virtual(&system->voiceBuffer[candidate->voiceIndex], SC_TRUE);
        }
        else
        {
            boundary[boundaryCount++] = *candidate;
        }
    }

    // Pass 4 - resolve the boundary. tieBudget candidates win, the rest lose.
    sc_virtual_voice_sort_boundary(boundary, boundaryCount, system->tiebreakerPolicy);
    for (sc_uint32 i = 0; i < boundaryCount; ++i)
    {
        const sc_bool wantsReal = (i < tieBudget);
        if (!wantsReal)
        {
            ma_log_postf(&system->log, MA_LOG_LEVEL_DEBUG, "[voice] virtualized (tiebreak): slot=%u priority=%u audibility=%.4f\n", boundary[i].voiceIndex, boundary[i].priority, boundary[i].audibility);
        }
        (void)sc_voice_set_virtual(&system->voiceBuffer[boundary[i].voiceIndex], !wantsReal);
    }

    return SBK_SUCCESS;
}