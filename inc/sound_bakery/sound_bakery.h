#ifndef SOUND_BAKERY_H
#define SOUND_BAKERY_H

#include "sound_bakery_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Defines groups of objects that are rendered together/in the same tree
 */
typedef enum SB_OBJECT_CATEGORY
{
    /**
     * @brief Unkown category
     */
    SB_CATEGORY_UNKNOWN,
    /**
     * @brief Sound file
     */
    SB_CATEGORY_SOUND,
    /**
     * @brief Every sound, random, blend etc.
     */
    SB_CATEGORY_NODE,
    /**
     * @brief Bus or aux busses
     */
    SB_CATEGORY_BUS,
    /**
     * @brief Music nodes like music segments
     */
    SB_CATEGORY_MUSIC,
    /**
     * @brief Events
     */
    SB_CATEGORY_EVENT,
    /**
     * @brief Soundbanks
    */
    SB_CATEGORY_BANK,
    /**
     * @brief Parameter types
     */
    SB_CATEGORY_PARAMETER,
    /**
     * @brief Any identifiable object not categorised above
     */
    SB_CATEGORY_DATABASE_OBJECT,
    /**
     * @brief Any runtime object
     */
    SB_CATEGORY_RUNTIME_OBJECT,
    SB_CATEGORY_NUM
} SB_OBJECT_CATEGORY;

void SB_API SB_System_Test();

sb_system_config SB_API sb_system_config_init_default();
sb_system_config SB_API sb_system_config_init(const char* pluginPath);

#ifdef __cplusplus
}
#endif

#endif