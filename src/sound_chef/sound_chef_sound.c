#include "sound_chef/sound_chef.h"

sbk_status sc_sound_get_length(sc_sound* sound, float* lengthInSeconds)
{
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(sound->system != NULL);
    SC_CHECK_ARG(lengthInSeconds != NULL);

    return SC_STATUS_FROM_MA_RESULT(ma_data_source_get_length_in_seconds(sound->dataSource, lengthInSeconds));
}

sbk_status sc_sound_release(sc_sound* sound)
{
    SC_CHECK_ARG(sound != NULL);

    if (sound->dataSource != NULL)
    {
        ma_resource_manager_data_source_uninit(sound->dataSource);
        SC_FREE(sound->dataSource, sound->system);
    }

    SC_FREE(sound, sound->system);

    return SBK_SUCCESS;
}