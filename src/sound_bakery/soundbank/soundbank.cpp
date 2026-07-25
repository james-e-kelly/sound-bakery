#include "soundbank.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"

DEFINE_REFLECTION(sbk::engine::soundbank)

template <class T>
auto object_ptr_to_shared_ptr(sbk::core::object* ptr) -> std::shared_ptr<T>
{
    return ptr->casted_shared_from_this<T>();
}

auto sbk::engine::soundbank_database::fill_runtime_database() -> void
{
    if (sbk::engine::system* const system = sbk::engine::system::get())
    {
        for (const soundbank_database_entry& entry : database)
        {
            (void)system->assign_name_to_id(entry.assetID, entry.assetName);
        }
    }
}

sbk::engine::soundbank_dependencies sbk::engine::soundbank::gather_dependencies() const
{
    sbk::engine::soundbank_dependencies dependencies;

    if (m_lookupSoundbank)
    {
        for (const std::weak_ptr<sbk::core::database_object> databaseObject : sbk::engine::system::get()->get_all_database_objects())
        {
            if (const std::shared_ptr<sbk::core::database_object> sharedDatabaseObject = databaseObject.lock())
            {
                if (sharedDatabaseObject->get_is_export())
                {
                    dependencies.lookupDatabase.database.push_back(
                        soundbank_database_entry{.assetName = sharedDatabaseObject->get_database_name(),
                                                 .assetID   = sharedDatabaseObject->get_database_id()});
                }
            }
        }
    }

    if (m_initSoundbank)
    {
        auto busPointers            = sbk::engine::system::get()->get_objects_of_category(SB_CATEGORY_BUS);
        auto intParameterPointers   = sbk::engine::system::get()->get_objects_of_type(sbk::engine::int_parameter::type());
        auto floatParameterPointers = sbk::engine::system::get()->get_objects_of_type(sbk::engine::float_parameter::type());
        auto namedParameterPointers = sbk::engine::system::get()->get_objects_of_type(sbk::engine::named_parameter::type());

        std::transform(busPointers.begin(), busPointers.end(), std::back_inserter(dependencies.busses), object_ptr_to_shared_ptr<sbk::engine::bus>);
        std::transform(intParameterPointers.begin(), intParameterPointers.end(), std::back_inserter(dependencies.intParameters), object_ptr_to_shared_ptr<sbk::engine::int_parameter>);
        std::transform(floatParameterPointers.begin(), floatParameterPointers.end(), std::back_inserter(dependencies.floatParameters), object_ptr_to_shared_ptr<sbk::engine::float_parameter>);
        std::transform(namedParameterPointers.begin(), namedParameterPointers.end(), std::back_inserter(dependencies.namedParameters), object_ptr_to_shared_ptr<sbk::engine::named_parameter>);
    }

    std::vector<std::shared_ptr<sbk::engine::node_base>> nodesToSave;

    for (auto& event : get_events())
    {
        if (auto eventShared = event.shared())
        {
            dependencies.events.push_back(eventShared);

            for (auto& action : eventShared->m_actions)
            {
                if (action.m_type != sbk::engine::action_type::play)
                {
                    continue;
                }

                if (auto destination = action.m_destination.shared())
                {
                    auto nodeBase = std::static_pointer_cast<node_base>(destination);

                    nodesToSave.push_back(nodeBase);
                    nodeBase->gather_all_descendants(nodesToSave);
                    nodeBase->gather_all_parents(nodesToSave);
                }
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
                    if (auto sound = soundContainer->get_sound())
                    {
                        const sbk::engine::encoding_sound encodingSound = sound->get_encoding_sound_data();
                        BOOST_ASSERT(!encodingSound.encodedSoundPath.empty());

                        dependencies.sounds.push_back(sound);
                    }
                }
            }
        }
    }

    return dependencies;
}