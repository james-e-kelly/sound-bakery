#include "soundbank.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/node/container/sound_container.h"

DEFINE_REFLECTION(sbk::engine::soundbank)

sbk::engine::soundbank_dependencies sbk::engine::soundbank::gather_dependencies() const
{
    sbk::engine::soundbank_dependencies dependencies;

    std::vector<sbk::engine::node_base*> nodesToSave;

    for (auto& event : get_events())
    {
        if (event.lookup())
        {
            dependencies.events.push_back(event.shared());

            for (auto& action : event->m_actions)
            {
                if (action.m_type != sbk::engine::SB_ACTION_PLAY)
                {
                    continue;
                }

                if (!action.m_destination.lookup())
                {
                    continue;
                }

                sbk::engine::node_base* const nodeBase =
                    action.m_destination->try_convert_object<sbk::engine::node_base>();

                nodeBase->gatherAllDescendants(nodesToSave);
                nodeBase->gatherAllParents(nodesToSave);
            }
        }
    }

    for (auto& node : nodesToSave)
    {
        if (node != nullptr)
        {
            dependencies.nodes.push_back(std::static_pointer_cast<sbk::engine::node_base>(node->shared_from_this()));

            if (node->getType() == sbk::engine::SoundContainer::type())
            {
                if (sbk::engine::SoundContainer* const soundContainer =
                        node->try_convert_object<sbk::engine::SoundContainer>())
                {
                    if (sbk::engine::sound* const sound = soundContainer->getSound())
                    {
                        dependencies.sounds.push_back(
                            std::static_pointer_cast<sbk::engine::sound>(sound->shared_from_this()));
                    }
                }
            }
        }
    }

    return dependencies;
}