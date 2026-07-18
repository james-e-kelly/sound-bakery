#ifndef SOUND_BAKERY_H
#define SOUND_BAKERY_H

#include "sound_bakery_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

sbk_status SB_API sbk_log(ma_log_level level, const char* message);

sbk_system_config SB_API sbk_system_config_init_default();
sbk_system_config SB_API sbk_system_config_init(const char* pluginPath);

SBK_NODISCARD sbk_status SB_API sbk_system_create();
SBK_NODISCARD sbk_status SB_API sbk_system_init(sbk_system_config config);
SBK_NODISCARD sbk_status SB_API sbk_system_update();
sbk_status SB_API sbk_system_destroy();

SBK_NODISCARD sbk_status SB_API sbk_system_get_object_count(uint64_t* count);
SBK_NODISCARD sbk_status SB_API sbk_system_get_object_info(uint64_t index, sbk_id* id, char* name, uint64_t nameSize, uint64_t* actualNameSize);

SBK_NODISCARD sbk_status SB_API sbk_system_load_soundbank(const char* soundbankFilePath, sbk_soundbank** outSoundbank);

SBK_NODISCARD sbk_status SB_API sbk_system_post_event(const char* eventName, sbk_id gameObjectID);
SBK_NODISCARD sbk_status SB_API sbk_system_stop_all(sbk_id gameObjectID);

#ifdef __cplusplus
}
#endif

#endif