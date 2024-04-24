#include "rttr/detail/misc/register_wrapper_mapper_conversion.h"
#include "rttr/detail/type/base_classes.h"
#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/event/event.h"
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

namespace SB::Reflection
{
    /**
     * @brief Creates wrapper_mapper conversions for the Dervived class and all its base classes.
     *
     * Implementation is taken from rttr's basic conversion tempates but adapts it to our use case.
     *
     * Once large difference is rttr assumes wrappers to wrap object types. However, our pointers wrap the SB_ID type.
     * This means it cannot automatically find the base_class_list.
     * In our custom version, we remove the auto deduction of wrapper types and just use our DatabasePtr and ChildPtr
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
                rttr::wrapper_mapper<SB::Core::ChildPtr<DerivedClass>>::template convert<SB::Core::DatabaseObject>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<SB::Core::ChildPtr<SB::Core::DatabaseObject>>::template convert<DerivedClass>);

            rttr::type::register_converter_func(
                rttr::wrapper_mapper<SB::Core::DatabasePtr<DerivedClass>>::template convert<SB::Core::DatabaseObject>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<SB::Core::DatabasePtr<SB::Core::DatabaseObject>>::template convert<DerivedClass>);
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
                rttr::wrapper_mapper<SB::Core::ChildPtr<DerivedClass>>::template convert<BaseClass>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<SB::Core::ChildPtr<BaseClass>>::template convert<DerivedClass>);

            rttr::type::register_converter_func(
                rttr::wrapper_mapper<SB::Core::DatabasePtr<DerivedClass>>::template convert<BaseClass>);
            rttr::type::register_converter_func(
                rttr::wrapper_mapper<SB::Core::DatabasePtr<BaseClass>>::template convert<DerivedClass>);

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
        using namespace SB::Core;
        using namespace SB::Engine;

        registration::enumeration<SC_DSP_TYPE>("SC_DSP_TYPE")(
            value("Fader", SC_DSP_TYPE_FADER), value("Lowpass", SC_DSP_TYPE_LOWPASS),
            value("Highpass", SC_DSP_TYPE_HIGHPASS), value("Delay", SC_DSP_TYPE_DELAY));

        registration::enumeration<SB_OBJECT_CATEGORY>("SB_OBJECT_CATEGORY")(
            value("Sound", SB_CATEGORY_SOUND), value("Bus", SB_CATEGORY_BUS), value("Node", SB_CATEGORY_NODE),
            value("Music", SB_CATEGORY_MUSIC), value("Event", SB_CATEGORY_EVENT), value("Soundbank", SB_CATEGORY_BANK),
            value("Parameter", SB_CATEGORY_PARAMETER));

        registration::enumeration<SB_ACTION_TYPE>("SB_ACTION_TYPE")(value("Play", SB_ACTION_PLAY),
                                                                    value("Stop", SB_ACTION_STOP));

        registration::class_<Action>("SB::Engine::Action")
            .constructor<>()(policy::ctor::as_object)
            .property("Type", &Action::m_type)
            .property("Destination",
                      &Action::m_destination)(metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadObject));

        registration::class_<EffectDescription>("SB::Engine::EffectDescription")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Type", &EffectDescription::getDSPType, &EffectDescription::setDSPType)
            .property("Parameters", &EffectDescription::m_parameterDescriptions);

        registration::class_<EffectParameterDescription>("SB::Engine::EffectParameterDescription")
            .constructor<>()(policy::ctor::as_object)
            .property("Parameter", &EffectParameterDescription::m_parameter);

        registration::class_<System>("SB::Engine::System").property("MasterBus", &System::m_masterBus);

        registration::class_<Object>("SB::Core::Object").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<DatabaseObject>("SB::Core::DatabaseObject")
            .constructor<>()(policy::ctor::as_raw_ptr)
            // Order is important here!
            // ID must be set first
            // When loading the object name, the ID must be valid for the name->ID
            // lookup
            .property("ObjectID", &DatabaseObject::getDatabaseID,
                      &DatabaseObject::setDatabaseID)(metadata(SB::Editor::METADATA_KEY::Readonly, true))
            .property("ObjectName", &DatabaseObject::getDatabaseName, &DatabaseObject::setDatabaseName);

        registration::class_<Sound>("SB::Engine::Sound")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Sound", &Sound::getSoundName,
                      &Sound::setSoundName)(metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadSound));

        registration::class_<NodeBase>("SB::Engine::NodeBase")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("ParentNode", &NodeBase::m_parentNode)(metadata(SB::Editor::METADATA_KEY::Readonly, true))
            .property("OutputBus",
                      &NodeBase::m_outputBus)(metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadBus))
            .property("ChildNodes", &NodeBase::m_childNodes)(metadata(SB::Editor::METADATA_KEY::Readonly, true));

        registration::class_<Node>("SB::Engine::Node")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Volume",
                      &Node::m_volume)(metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 1.0f)))
            .property("Pitch",
                      &Node::m_pitch)(metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 2.0f)))
            .property("Lowpass", &Node::m_lowpass)(
                metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 100.0f)))
            .property("Highass", &Node::m_highpass)(
                metadata(SB::Editor::METADATA_KEY::MinMax, std::pair<float, float>(0.0f, 100.0f)))
            .property("Effects", &Node::m_effectDescriptions)
            .method("Add Effect", &Node::addEffect)(parameter_names("Type"));

        registration::class_<Container>("SB::Engine::Container");

        registration::class_<SoundContainer>("SB::Engine::SoundContainer")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Sound",
                      &SoundContainer::m_sound)(metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadSound));

        registration::class_<BlendContainer>("SB::Engine::BlendContainer").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<SwitchContainer>("SB::Engine::SwitchContainer")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Switch", &SwitchContainer::getSwitchParameter, &SwitchContainer::setSwitchParameter)(
                metadata(SB::Editor::METADATA_KEY::Payload, SB::Editor::PayloadNamedParam))
            .property("Mappings", &SwitchContainer::getSwitchToChildMap, &SwitchContainer::setSwitchToChild);

        registration::class_<RandomContainer>("SB::Engine::RandomContainer").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<SequenceContainer>("SB::Engine::SequenceContainer")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Sequence", &SequenceContainer::m_sequence);

        registration::class_<Bus>("SB::Engine::Bus")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property_readonly("IsMasterBus", &Bus::m_masterBus);

        registration::class_<Event>("SB::Engine::Event")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Actions", &Event::m_actions);

        registration::class_<FloatParameter>("SB::Engine::FloatParameter").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<IntParameter>("SB::Engine::IntParameter").constructor<>()(policy::ctor::as_raw_ptr);

        registration::class_<NamedParameter>("SB::Engine::NamedParameter")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Values", &NamedParameter::m_values)(metadata(SB::Editor::METADATA_KEY::Readonly, true))
            .property("ParameterValue", &NamedParameter::getSelectedValue, &NamedParameter::setSelectedValue);

        registration::class_<NamedParameterValue>("SB::Engine::NamedParameterValue")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Parent",
                      &NamedParameterValue::parentParameter)(metadata(SB::Editor::METADATA_KEY::Readonly, true));

        registration::class_<Soundbank>("SB::Engine::Soundbank")
            .constructor<>()(policy::ctor::as_raw_ptr)
            .property("Events", &Soundbank::m_events);

        SB::Reflection::RegisterPointerConversionsForBaseClasses<AuxBus>();

        SB::Reflection::RegisterPointerConversionsForBaseClasses<BlendContainer>();
        SB::Reflection::RegisterPointerConversionsForBaseClasses<RandomContainer>();
        SB::Reflection::RegisterPointerConversionsForBaseClasses<SequenceContainer>();
        SB::Reflection::RegisterPointerConversionsForBaseClasses<SoundContainer>();
        SB::Reflection::RegisterPointerConversionsForBaseClasses<SwitchContainer>();

        SB::Reflection::RegisterPointerConversionsForBaseClasses<Container>();  // makes sure we have a direct
                                                                                // conversion between Container and
                                                                                // DatabaseObject

        SB::Reflection::RegisterPointerConversionsForBaseClasses<Sound>();

        SB::Reflection::RegisterPointerConversionsForBaseClasses<Soundbank>();
    }
}  // namespace SB::Reflection