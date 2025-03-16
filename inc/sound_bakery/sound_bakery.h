#ifndef SOUND_BAKERY_H
#define SOUND_BAKERY_H

#include "sound_bakery_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

sb_system_config SB_API sb_system_config_init_default();
sb_system_config SB_API sb_system_config_init(const char* pluginPath);

sb_result SB_API sb_system_create();
sb_result SB_API sb_system_init(sb_system_config config);
sb_result SB_API sb_system_update();
sb_result SB_API sb_system_destroy();

#ifdef __cplusplus
}
#endif

#endif