#include "soundbank.h"

#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/system.h"

DEFINE_REFLECTION(sbk::engine::soundbank)

template <class T>
std::shared_ptr<T> object_ptr_to_shared_ptr(sbk::core::object* ptr)
{
    return ptr->casted_shared_from_this<T>();
}

sbk::engine::soundbank_dependencies sbk::engine::soundbank::gather_dependencies() const
{
    sbk::engine::soundbank_dependencies dependencies;

    if (m_masterSoundbank)
    {
        auto busPointers = sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_BUS);
        auto intParameterPointers = sbk::engine::system::get()->get_objects_of_type(sbk::engine::int_parameter::type());
        auto floatParameterPointers = sbk::engine::system::get()->get_objects_of_type(sbk::engine::float_parameter::type());
        auto namedParameterPointers = sbk::engine::system::get()->get_objects_of_type(sbk::engine::named_parameter::type());

        std::transform(busPointers.begin(), busPointers.end(), std::back_inserter(dependencies.busses), object_ptr_to_shared_ptr<sbk::engine::bus>);
        std::transform(intParameterPointers.begin(), intParameterPointers.end(), std::back_inserter(dependencies.intParameters), object_ptr_to_shared_ptr<sbk::engine::int_parameter>);
        std::transform(floatParameterPointers.begin(), floatParameterPointers.end(), std::back_inserter(dependencies.floatParameters), object_ptr_to_shared_ptr<sbk::engine::float_parameter>);
        std::transform(namedParameterPointers.begin(), namedParameterPointers.end(), std::back_inserter(dependencies.namedParameters), object_ptr_to_shared_ptr<sbk::engine::named_parameter>);
    }

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

                nodesToSave.push_back(nodeBase);
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

            if (node->get_object_type() == sbk::engine::sound_container::type())
            {
                if (sbk::engine::sound_container* const soundContainer =
                        node->try_convert_object<sbk::engine::sound_container>())
                {
                    if (sbk::engine::sound* const sound = soundContainer->get_sound())
                    {
                        const sbk::engine::encoding_sound encodingSound = sound->get_encoding_sound_data();
                        BOOST_ASSERT(!encodingSound.encodedSoundPath.empty());

                        dependencies.sounds.push_back(
                            std::static_pointer_cast<sbk::engine::sound>(sound->shared_from_this()));
                    }
                }
            }
        }
    }

    return dependencies;
}