#ifndef SOUND_BAKERY_H
#define SOUND_BAKERY_H

#include "sound_bakery_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

sbk_system_config SB_API sbk_system_config_init_default();
sbk_system_config SB_API sbk_system_config_init(const char* pluginPath);

sbk_result SB_API sbk_system_create();
sbk_result SB_API sbk_system_init(sbk_system_config config);
sbk_result SB_API sbk_system_update();
sbk_result SB_API sbk_system_destroy();

sbk_result SB_API sbk_system_load_soundbank(const char* soundbankFilePath, sbk_soundbank** outSoundbank);

#ifdef __cplusplus
}
#endif

#endif