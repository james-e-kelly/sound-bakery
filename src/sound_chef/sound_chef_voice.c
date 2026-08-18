#include "sound_chef/sound_chef.h"

_Static_assert(sizeof(sc_voice) == 64, "sc_voice must fit in a 64-byte cache line");

static sbk_status sc_voice_resolve(sc_system* system, sc_voice_handle handle, sc_voice** outVoice)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outVoice != NULL);

    const sc_voice_index slot = SC_VOICE_HANDLE_EXTRACT_INDEX(handle);
    SC_CHECK_ARG(slot < system->voiceSlotAllocator.capacity);

    sc_voice* const voice         = &system->voiceBuffer[slot];
    const sc_voice_handle current = (sc_voice_handle)c89atomic_load_64(&voice->handle);
    SC_CHECK(current == handle, SBK_ERR_NOT_FOUND);

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

sbk_status sc_voice_pause(sc_system* system, sc_voice_handle handle)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_PAUSED, SC_TRUE);
    return SBK_SUCCESS;
}

sbk_status sc_voice_resume(sc_system* system, sc_voice_handle handle)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_PAUSED, SC_FALSE);
    return SBK_SUCCESS;
}

sbk_status sc_voice_stop(sc_system* system, sc_voice_handle handle)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_32(&voice->desiredState, (sc_uint32)sc_voice_desired_stopped);
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
