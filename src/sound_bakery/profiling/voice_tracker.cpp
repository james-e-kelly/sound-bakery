#include "sound_bakery/profiling/voice_tracker.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine::profiling;

void voice_tracker::update(system* system)
{
    m_playingNodeIDs.clear();
    m_nodePlayingCount.clear();

    if (const game_object* const listener = system->get_listener_game_object())
    {
        for (const std::shared_ptr<sbk::core::object>& object : listener->get_objects())
        {
            if (const sbk::engine::voice* const voice = object->try_convert_object<sbk::engine::voice>())
            {
                std::unordered_set<const node_instance*> trackedNodes;

                for (std::size_t j = 0; j < voice->num_voices(); ++j)
                {
                    if (const node_instance* const nodeInstance = voice->node_instance_at(j))
                    {
                        if (!trackedNodes.contains(nodeInstance))
                        {
                            trackedNodes.insert(nodeInstance);

                            if (const std::shared_ptr<node> node = nodeInstance->getReferencingNode())
                            {
                                m_playingNodeIDs.insert(node->get_database_id());
                                m_nodePlayingCount[node->get_database_id()]++;
                            }

                            const node_instance* parent = nodeInstance->get_parent();

                            if (!trackedNodes.contains(parent))
                            {
                                trackedNodes.insert(parent);

                                while (parent)
                                {
                                    if (const std::shared_ptr<node> node = parent->getReferencingNode())
                                    {
                                        m_playingNodeIDs.insert(node->get_database_id());
                                        m_nodePlayingCount[node->get_database_id()]++;
                                    }

                                    parent = parent->get_parent();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

unsigned int voice_tracker::getPlayingCountOfObject(sbk_id id) const
{
    unsigned int result = 0;

    if (std::unordered_map<sbk_id, unsigned int>::const_iterator find = m_nodePlayingCount.find(id);
        find != m_nodePlayingCount.cend())
    {
        result = find->second;
    }

    return result;
}
