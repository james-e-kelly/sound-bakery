#include "rttr/detail/misc/register_wrapper_mapper_conversion.h"
#include "rttr/detail/type/base_classes.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/aux_bus.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/blend_container.h"
#include "sound_bakery/node/container/random_container.h"
#include "sound_bakery/node/container/sequence_container.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/container/switch_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"

#include <rttr/registration>

namespace sbk::reflection
{
    /**
     * @brief Creates wrapper_mapper conversions for the Dervived class and all its base classes.
     *
     * Implementation is taken from rttr's basic conversion tempates but adapts it to our use case.
     *
     * Once large difference is rttr assumes wrappers to wrap object types. However, our pointers wrap the SB_ID type.
     * This means it cannot automatically find the base_class_list.
     * In our custom version, we remove the auto deduction of wrapper types and just use our DatabasePtr and child_ptr
     * wrappers.
     */
    template <typename DerivedClass, typename... T>
    struct CreatePointerConversion;

    /**
     * @brief Once the top-most base class is reached, make an explicit conversion between the base type and
     * DatabaseObject.
     *
     * UI code assume wrapped types can be converted to DatabaseObject wrappers.
     */
    template <typename DerivedClass>
    struct CreatePointerConversion<DerivedClass>
    {
        static void perform()
        {
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::child_ptr<DerivedClass>>::template convert<sbk::core::database_object>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::child_ptr<sbk::core::database_object>>::template convert<DerivedClass>);

            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::database_ptr<DerivedClass>>::template convert<
                    sbk::core::database_object>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::database_ptr<sbk::core::database_object>>::template convert<
                    DerivedClass>);
        }
    };

    /**
     * @brief Registers conversions between the derived type and base class, then does the same for the base class's
     * base class list.
     */
    template <typename DerivedClass, typename BaseClass, typename... U>
    struct CreatePointerConversion<DerivedClass, BaseClass, U...>
    {
        static void perform()
        {
            static_assert(rttr::detail::has_base_class_list<BaseClass>::value);

            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::child_ptr<DerivedClass>>::template convert<BaseClass>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::child_ptr<BaseClass>>::template convert<DerivedClass>);

            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::database_ptr<DerivedClass>>::template convert<BaseClass>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<sbk::core::database_ptr<BaseClass>>::template convert<DerivedClass>);

            CreatePointerConversion<DerivedClass, typename BaseClass::base_class_list>::perform();
            CreatePointerConversion<DerivedClass, U...>::perform();
        }
    };

    /**
     * @brief Specialisation for wrapping the rttr::type_list type and extracting its template arguments.
     */
    template <typename DerivedClass, class... BaseClassList>
    struct CreatePointerConversion<DerivedClass, rttr::type_list<BaseClassList...>>
        : CreatePointerConversion<DerivedClass, BaseClassList...>
    {
    };

    /**
     * @brief Auto-registers wrapper conversions for the type and its base classes.
     */
    template <typename T>
    struct RegisterPointerConversionsForBaseClasses
    {
        RegisterPointerConversionsForBaseClasses()
        {
            CreatePointerConversion<T, typename T::base_class_list>::perform();
        }
    };

    void unregisterReflectionTypes() { rttr::detail::get_registration_manager().unregister(); }

    void registerReflectionTypes()
    {
        using namespace rttr;
        using namespace sbk::core;
        using namespace sbk::engine;

        registration::enumeration<sc_dsp_type>("sc_dsp_type")(
            value("Fader", SC_DSP_TYPE_FADER), value("Lowpass", SC_DSP_TYPE_LOWPASS),
            value("Highpass", SC_DSP_TYPE_HIGHPASS), value("Delay", SC_DSP_TYPE_DELAY));

        registration::enumeration<SB_OBJECT_CATEGORY>("SB_OBJECT_CATEGORY")(
            value("Sound", SB_CATEGORY_SOUND), value("Bus", SB_CATEGORY_BUS), value("Node", SB_CATEGORY_NODE),
            value("Music", SB_CATEGORY_MUSIC), value("Event", SB_CATEGORY_EVENT), value("Soundbank", SB_CATEGORY_BANK),
            value("Parameter", SB_CATEGORY_PARAMETER));

        registration::enumeration<SB_ACTION_TYPE>("SB_ACTION_TYPE")(value("Play", SB_ACTION_PLAY),
                                                                    value("Stop", SB_ACTION_STOP));

        registration::class_<action>("SB::Engine::Action")
            .constructor<>()(policy::ctor::as_object)
            .property("Type", &action::m_type)
            .property("Destination",
                      &action::m_destination)(metadata(sbk::editor::METADATA_KEY::Payload, sbk::editor::PayloadObject));

        registration::class_<effect_description>("SB::Engine::EffectDescription")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Type", &effect_description::get_dsp_type, &effect_description::set_dsp_type)
            .property("Parameters", &effect_description::m_parameterDescriptions);

        registration::class_<effect_parameter_description>("SB::Engine::EffectParameterDescription")
            .constructor<>()(policy::ctor::as_object)
            .property("Parameter", &effect_parameter_description::m_parameter);

        registration::class_<system>("SB::Engine::system");

        registration::class_<game_object>("SB::Engine::GameObject").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<object>("SB::Core::object").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<database_object>("SB::Core::database_object")
            .constructor<>()(policy::ctor::as_raw_ptr)
            // Order is important here!
            // ID must be set first
            // When loading the object name, the ID must be valid for the name->ID
            // lookup
            .property("ObjectID", &database_object::get_database_id,
                      &database_object::set_database_id)(metadata(sbk::editor::METADATA_KEY::Readonly, true))
            .property("ObjectName", &database_object::get_database_name, &database_object::set_database_name);

        registration::class_<Sound>("SB::Engine::Sound")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Sound", &Sound::getSoundName,
                      &Sound::setSoundName)(metadata(sbk::editor::METADATA_KEY::Payload, sbk::editor::PayloadSound))
            .property("Encoded Sound", &Sound::getEncodedSoundName, &Sound::setEncodedSoundName)(
                metadata(sbk::editor::METADATA_KEY::Payload, sbk::editor::PayloadSound));

        registration::class_<node_base>("SB::Engine::NodeBase")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("ParentNode", &node_base::m_parentNode)(metadata(sbk::editor::METADATA_KEY::Readonly, true))
            .property("OutputBus",
                      &node_base::m_outputBus)(metadata(sbk::editor::METADATA_KEY::Payload, sbk::editor::PayloadBus))
            .property("ChildNodes", &node_base::m_childNodes)(metadata(sbk::editor::METADATA_KEY::Readonly, true));

        registration::class_<node>("SB::Engine::Node")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Volume",
                      &node::m_volume)(metadata(sbk::editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 1.0f)))
            .property("Pitch",
                      &node::m_pitch)(metadata(sbk::editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 2.0f)))
            .property("Lowpass", &node::m_lowpass)(
                metadata(sbk::editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 100.0f)))
            .property("Highass", &node::m_highpass)(
                metadata(sbk::editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 100.0f)))
            .property("Effects", &node::m_effectDescriptions)
            .method("Add Effect", &node::addEffect)(parameter_names("Type"));

        registration::class_<Container>("SB::Engine::Container");

        registration::class_<SoundContainer>("SB::Engine::SoundContainer")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Sound", &SoundContainer::m_sound)(
                metadata(sbk::editor::METADATA_KEY::Payload, sbk::editor::PayloadSound));

        registration::class_<BlendContainer>("SB::Engine::BlendContainer").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<SwitchContainer>("SB::Engine::SwitchContainer")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Switch", &SwitchContainer::getSwitchParameter, &SwitchContainer::setSwitchParameter)(
                metadata(sbk::editor::METADATA_KEY::Payload, sbk::editor::PayloadNamedParam))
            .property("Mappings", &SwitchContainer::getSwitchToChildMap, &SwitchContainer::setSwitchToChild);

        registration::class_<RandomContainer>("SB::Engine::RandomContainer").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<SequenceContainer>("SB::Engine::SequenceContainer")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Sequence", &SequenceContainer::m_sequence);

        registration::class_<bus>("SB::Engine::Bus")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("IsMasterBus", &bus::isMasterBus, &bus::setMasterBus)(metadata(sbk::editor::METADATA_KEY::Readonly, true));

        registration::class_<event>("SB::Engine::Event")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Actions", &event::m_actions);

        registration::class_<FloatParameter>("SB::Engine::FloatParameter").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<IntParameter>("SB::Engine::IntParameter").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<NamedParameter>("SB::Engine::NamedParameter")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Values", &NamedParameter::m_values)(metadata(sbk::editor::METADATA_KEY::Readonly, true))
            .property("ParameterValue", &NamedParameter::getSelectedValue, &NamedParameter::setSelectedValue);

        registration::class_<NamedParameterValue>("SB::Engine::NamedParameterValue")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Parent",
                      &NamedParameterValue::parentParameter)(metadata(sbk::editor::METADATA_KEY::Readonly, true));

        registration::class_<Soundbank>("SB::Engine::Soundbank")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Events", &Soundbank::m_events);

        sbk::reflection::RegisterPointerConversionsForBaseClasses<aux_bus>();

        sbk::reflection::RegisterPointerConversionsForBaseClasses<BlendContainer>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<RandomContainer>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<SequenceContainer>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<SoundContainer>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<SwitchContainer>();

        sbk::reflection::RegisterPointerConversionsForBaseClasses<Container>();  // makes sure we have a direct
                                                                                 // conversion between Container and
                                                                                 // DatabaseObject

        sbk::reflection::RegisterPointerConversionsForBaseClasses<Sound>();

        sbk::reflection::RegisterPointerConversionsForBaseClasses<Soundbank>();
    }
}  // namespace sbk::reflection