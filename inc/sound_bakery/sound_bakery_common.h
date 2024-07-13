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

typedef sc_result sb_result;

typedef unsigned long long sbk_id;
#define SB_INVALID_ID 0

#define SBK_VERSION_MAJOR    0
#define SBK_VERSION_MINOR    2
#define SBK_VERSION_REVISION 0
#define SBK_VERSION_STRING \
    MA_XSTRINGIFY(SBK_VERSION_MAJOR) "." MA_XSTRINGIFY(SBK_VERSION_MINOR) "." MA_XSTRINGIFY(SBK_VERSION_REVISION)

#endif