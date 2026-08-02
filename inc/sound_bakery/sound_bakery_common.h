#ifndef SOUND_BAKERY_COMMON_H
#define SOUND_BAKERY_COMMON_H

#ifdef sound_bakery_shared_EXPORTS
    #define SB_DLL
    #define SC_DLL
    #define MA_DLL
#endif

#include "sound_chef/sound_chef.h"
#include "sound_bakery/sound_bakery_version.h"
#include <stdint.h>

#define SB_API SC_API
#define SB_CLASS SC_CLASS

typedef struct sbk_system_config sbk_system_config;

typedef uint64_t sbk_id;
#define SBK_INVALID_ID 0

static_assert(sizeof(void*) == sizeof(sbk_id));
static_assert(sizeof(sbk_id) == sizeof(uintptr_t));

struct sbk_system_config
{
    sc_system_config    soundChefConfig;
    bool                singleThreadedUpdate;   //< Update the database/studio thread when @r sbk::engine::system::update is called. Helpful for when consuming applications need to access Sound Bakery non-realtime data safely
    bool                enableProfiling;        //< Enable voice tracking, CPU usage, etc.
    bool                logToConsole;           //< Automatically set up the logger to print to the console. Normally left off so consuming applications can control Sound Bakery's logging
};

#endif