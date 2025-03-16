#ifndef SOUND_BAKERY_COMMON_H
#define SOUND_BAKERY_COMMON_H

#ifdef sound_bakery_shared_EXPORTS
    #define SB_DLL
    #define SC_DLL
    #define MA_DLL
#endif

#include "sound_chef/sound_chef.h"
#include "sound_bakery/sound_bakery_version.h"

#define SB_API SC_API
#define SB_CLASS SC_CLASS

typedef sc_result sb_result;

typedef struct sb_system_config sb_system_config;

typedef unsigned long long sbk_id;
#define SB_INVALID_ID 0

struct sb_system_config
{
    sc_system_config soundChefConfig;
};

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

#endif