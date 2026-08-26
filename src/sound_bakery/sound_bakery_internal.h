#ifndef _SOUND_BAKERY_H
#define _SOUND_BAKERY_H

#include "sound_bakery/sound_bakery.h"

struct SC_SOUND_DELETER
{
    auto operator()(sc_sound* sound) -> void { sc_sound_release(sound); }
};

struct SC_NODE_GROUP_DELETER
{
    auto operator()(sc_node_group* bus) -> void { sc_node_group_release(bus); }
};

#endif