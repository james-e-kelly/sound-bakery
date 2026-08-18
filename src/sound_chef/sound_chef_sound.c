#include "sound_chef/sound_chef.h"

sbk_status sc_sound_get_length(sc_sound* sound, float* lengthInSeconds)
{
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(lengthInSeconds != NULL);

    ma_sound_get_length_in_seconds(&sound->sound, lengthInSeconds);

    return SBK_SUCCESS;
}

sbk_status sc_sound_release(sc_sound* sound)
{
    SC_CHECK_ARG(sound != NULL);
    ma_sound_uninit(&sound->sound);

    if (sound->memoryDecoder != NULL)
    {
        ma_decoder_uninit(sound->memoryDecoder);
        SC_FREE(sound->memoryDecoder, sound->owningSystem);
    }

    SC_FREE(sound, sound->owningSystem);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_is_playing(sc_sound_instance* instance, sc_bool* isPlaying)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(isPlaying != NULL);

    *isPlaying = ma_sound_is_playing(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_start(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);

    ma_sound_start(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_pause(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);

    ma_sound_stop(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_get_cursor_in_seconds(sc_sound_instance* instance, float* seconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(seconds != NULL);

    ma_sound_get_cursor_in_seconds(&instance->sound, seconds);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_set_cursor_in_seconds(sc_sound_instance* instance, float seconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(seconds >= 0.0f);

    ma_uint64 lengthInPCMFrames = 0;
    float lengthInSeconds       = 0.0f;

    ma_sound_get_length_in_pcm_frames(&instance->sound, &lengthInPCMFrames);
    ma_sound_get_length_in_seconds(&instance->sound, &lengthInSeconds);

    const float percentage               = seconds / lengthInSeconds;
    const ma_uint32 frameIndexForSeconds = (ma_uint32)((float)lengthInPCMFrames * percentage);

    return SBK_FROM_MA(ma_sound_seek_to_pcm_frame(&instance->sound, frameIndexForSeconds));
}

sbk_status sc_sound_instance_get_loop_position_in_seconds(sc_sound_instance* instance, float* seconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(seconds != NULL);

    ma_data_source* const dataSource = ma_sound_get_data_source(&instance->sound);
    if (dataSource)
    {
        ma_uint64 loopPointInPCMFrames = 0;
        ma_uint32 sampleRate           = 0;

        SC_CHECK_STATUS(SBK_FROM_MA(ma_data_source_get_data_format(dataSource, NULL, NULL, &sampleRate, NULL, 0)));
        ma_data_source_get_loop_point_in_pcm_frames(dataSource, NULL, &loopPointInPCMFrames);

        *seconds = (float)loopPointInPCMFrames / (float)sampleRate;
    }

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_set_loop_position_in_seconds(sc_sound_instance* instance, float loopStartSeconds, float loopEndSeconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(loopStartSeconds >= 0.0f);
    SC_CHECK_ARG(loopEndSeconds >= 0.0f);

    ma_data_source* const dataSource =
        ma_sound_get_data_source(&instance->sound);
    if (dataSource)
    {
        ma_uint32 sampleRate = 0;

        SC_CHECK_STATUS(SBK_FROM_MA(ma_data_source_get_data_format(dataSource, NULL, NULL, &sampleRate, NULL, 0)));

        const ma_uint64 loopStartInPCMFrames = (ma_uint64)(loopStartSeconds * (float)sampleRate);
        const ma_uint64 loopEndInPCMFrames   = (ma_uint64)(loopEndSeconds * (float)sampleRate);

        SC_CHECK_STATUS(
            SBK_FROM_MA(ma_data_source_set_loop_point_in_pcm_frames(dataSource, loopStartInPCMFrames, loopEndInPCMFrames)));
    }

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_is_looping(sc_sound_instance* instance, sc_bool* looping)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(looping != NULL);

    *looping = ma_sound_is_looping(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_set_looping(sc_sound_instance* instance, sc_bool looping)
{
    SC_CHECK_ARG(instance != NULL);

    ma_sound_set_looping(&instance->sound, looping);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_release(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);
    ma_sound_uninit(&instance->sound);
    SC_FREE(instance, instance->owningSystem);
    return SBK_SUCCESS;
}