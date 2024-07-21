#include "sound_bakery/profiling/voice_tracker.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine::Profiling;

static VoiceTracker* s_voiceTracker = nullptr;

VoiceTracker::VoiceTracker()
{
    assert(!s_voiceTracker);

    s_voiceTracker = this;
}

VoiceTracker* VoiceTracker::get() { return s_voiceTracker; }

void VoiceTracker::update(system* system)
{
    m_playingNodeIDs.clear();
    m_nodePlayingCount.clear();

    // @TODO Fix listener game object
    if (const game_object* const listener = nullptr /* system->get_listener_game_object() */)
    {
        for (std::size_t i = 0; i < listener->voice_count(); ++i)
        {
            if (const voice* const voice = listener->get_voice(i))
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

                            const node_instance* parent = nodeInstance->getParent();

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

                                    parent = parent->getParent();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

unsigned int VoiceTracker::getPlayingCountOfObject(sbk_id id) const
{
    unsigned int result = 0;

    if (std::unordered_map<sbk_id, unsigned int>::const_iterator find = m_nodePlayingCount.find(id);
        find != m_nodePlayingCount.cend())
    {
        result = find->second;
    }

    return result;
}
