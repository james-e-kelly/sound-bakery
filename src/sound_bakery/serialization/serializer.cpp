#include "serializer.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"

//void yaml_serializer::packageSoundbank(sbk::engine::soundbank* soundbank, YAML::Emitter& emitter)
//{
//    if (soundbank != nullptr)
//    {
//        std::vector<sbk::engine::event*> eventsToSave;
//        std::vector<sbk::engine::node_base*> nodesToSave;
//        std::vector<sbk::engine::sound*> soundsToSave;
//
//        for (auto& event : soundbank->get_events())
//        {
//            if (event.lookup())
//            {
//                eventsToSave.push_back(event.raw());
//
//                for (auto& action : event->m_actions)
//                {
//                    if (action.m_type != sbk::engine::SB_ACTION_PLAY)
//                    {
//                        continue;
//                    }
//
//                    if (!action.m_destination.lookup())
//                    {
//                        continue;
//                    }
//
//                    sbk::engine::node_base* const nodeBase =
//                        action.m_destination->try_convert_object<sbk::engine::node_base>();
//
//                    assert(nodeBase);
//
//                    nodeBase->gatherAllDescendants(nodesToSave);
//                    nodeBase->gatherAllParents(nodesToSave);
//                }
//            }
//        }
//
//        for (auto& node : nodesToSave)
//        {
//            if (node->get_object_type() == sbk::engine::sound_container::type())
//            {
//                if (sbk::engine::sound_container* const soundContainer =
//                        node->try_convert_object<sbk::engine::sound_container>())
//                {
//                    if (sbk::engine::sound* const sound = soundContainer->get_sound())
//                    {
//                        soundsToSave.push_back(sound);
//                    }
//                }
//            }
//        }
//
//        for (sbk::engine::event* event : eventsToSave)
//        {
//            BOOST_ASSERT(event);
//
//            Doc eventDoc(emitter);
//
//            saveInstance(emitter, event);
//        }
//
//        for (sbk::engine::node_base* node : nodesToSave)
//        {
//            BOOST_ASSERT(node);
//
//            Doc soundDoc(emitter);
//
//            saveInstance(emitter, node);
//        }
//
//        for (sbk::engine::sound* sound : soundsToSave)
//        {
//            BOOST_ASSERT(sound);
//
//            Doc soundDoc(emitter);
//
//            saveInstance(emitter, sound);
//
//            ma_data_source* dataSource = ma_sound_get_data_source(&sound->getSound()->sound);
//        }
//
//        {
//            Doc doc(emitter);
//
//            saveInstance(emitter, soundbank);
//        }
//    }
//}

auto sbk::core::serialization::make_default_variant(const rttr::type& type) -> rttr::variant
{
    BOOST_ASSERT(type.is_valid());

    if (type.is_wrapper())
    {
        BOOST_ASSERT_MSG(type.get_wrapped_type().is_arithmetic(),
                         "We only convert simple types to ensure we're not creating large objects accidentally");

        rttr::variant variant = 0u;
        variant.convert(type.get_wrapped_type());
        BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the default value to the wrapped type");
        variant.convert(type);
        BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the variant to the wrapping type");
        return variant;
    }
    else if (type.is_arithmetic())  // Can't construct arithmetic types
    {
        rttr::variant variant = 0u;
        variant.convert(type);
        BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the default value to type");
        return variant;
    }
    else if (type.is_enumeration())
    {
        rttr::enumeration enumeration = type.get_enumeration();
        rttr::array_range<rttr::variant> values = enumeration.get_values();
        BOOST_ASSERT_MSG(!values.empty(), "Enums must have values so we can create a default one");
        return *values.begin();
    }
    else if (type == rttr::type::get<std::string>())
    {
        BOOST_ASSERT(false);
        return rttr::variant(std::string());
    }
    else if (type == rttr::type::get<std::string_view>())
    {
        BOOST_ASSERT(false);
        return rttr::variant(std::string_view());
    }

    BOOST_ASSERT_MSG(type.get_constructor({}).is_valid(), "Types must have constructors at this point");
    return type.create_default();
}
