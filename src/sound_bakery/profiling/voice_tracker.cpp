#include "sound_bakery/profiling/voice_tracker.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/runtime/runtime.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine::profiling;

auto voice_tracker::update(system* system) -> void
{
    m_playingNodeIDs.clear();
    m_nodePlayingCount.clear();

    if (const sbk::engine::runtime* const runtime = system->get_runtime())
    {
        if (const auto listener = runtime->get_listener_game_object())
        {
            for (const std::shared_ptr<sbk::core::object>& object : listener->get_objects())
            {
                if (const sbk::engine::voice* const voice = object->try_convert_object<sbk::engine::voice>())
                {
                    std::unordered_set<const node_instance*> trackedNodes;
                }
            }
        }
    }
}

auto voice_tracker::get_playing_count_of_object(sbk_id id) const -> unsigned int
{
    unsigned int result = 0;

    if (std::unordered_map<sbk_id, unsigned int>::const_iterator find = m_nodePlayingCount.find(id);
        find != m_nodePlayingCount.cend())
    {
        result = find->second;
    }

    return result;
}
