#ifndef _SOUND_BAKERY_H
#define _SOUND_BAKERY_H

#include "sound_bakery/sound_bakery.h"

template <class T>
class pass_key
{
private:
    friend T;
    pass_key() {}
    pass_key(const pass_key&) {}
    pass_key& operator=(const pass_key&) = delete;
};

struct SC_SOUND_DELETER
{
    void operator()(sc_sound* sound) { sc_sound_release(sound); }
};

struct SC_SOUND_INSTANCE_DELETER
{
    void operator()(sc_sound_instance* instance) { sc_sound_instance_release(instance); }
};

struct SC_NODE_GROUP_DELETER
{
    void operator()(sc_node_group* bus) { sc_node_group_release(bus); }
};

#endif