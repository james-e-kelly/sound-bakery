#ifndef SOUND_BAKERY_COMMON_H
#define SOUND_BAKERY_COMMON_H

#ifdef sound_bakery_shared_EXPORTS
    #define SB_DLL
    #define SC_DLL
    #define MA_DLL
#endif

#include "sound_chef/sound_chef.h"

#define SB_API SC_API
#define SB_CLASS SC_CLASS

typedef SC_RESULT SB_RESULT;

typedef unsigned long long SB_ID;
#define SB_INVALID_ID 0

#endif