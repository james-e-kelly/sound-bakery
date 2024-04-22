#ifndef _SOUND_BAKERY_H
#define _SOUND_BAKERY_H

#include "sound_bakery/sound_bakery.h"

struct SC_SOUND_DELETER
{
    void operator()(SC_SOUND* sound) { SC_Sound_Release(sound); }
};

struct SC_SOUND_INSTANCE_DELETER
{
    void operator()(SC_SOUND_INSTANCE* instance) { SC_SoundInstance_Release(instance); }
};

struct SC_NODE_GROUP_DELETER
{
    void operator()(SC_NODE_GROUP* bus) { SC_NodeGroup_Release(bus); }
};

#endif