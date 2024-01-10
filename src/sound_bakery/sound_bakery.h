#ifndef _SOUND_BAKERY_H
#define _SOUND_BAKERY_H

#include "sound_chef/sound_chef.h"

#define SB_INVALID_ID    0
typedef unsigned long long  SB_ID;

typedef enum SB_RESULT
{
    SB_SUCCESS = 1,
    SB_ERROR = 0
} SB_RESULT;

typedef enum ATLAS_EVENT_ACTION
{
    ATLAS_EVENT_ACTION_PLAY,
    ATLAS_EVENT_ACTION_STOP,
    ATLAS_EVENT_ACTION_BREAK,
    ATLAS_EVENT_ACTION_NUM
} ATLAS_EVENT_ACTION;

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

struct SC_NODE_GROUP_DELETER
{
    void operator()(SC_NODE_GROUP* bus)
    {
        SC_NodeGroup_Release(bus);
    }
};

#endif