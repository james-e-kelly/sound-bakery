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
#include "sound_bakery/util/type_helper.h"
#include "sound_bakery/voice/voice.h"

#include "rttr/detail/misc/register_wrapper_mapper_conversion.h"
#include "rttr/detail/type/base_classes.h"

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
        static auto perform() -> void
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
        static auto perform() -> void
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

    template <class object_class>
    static auto create_sbk_object() -> object_class*
    {
        const sbk::memory::object_category category = sbk::util::type_helper::get_category_from_type(object_class::type());

        // Allocate first so we can check for out-of-memory before constructing. object::operator new
        // routes to sbk::memory::malloc, which returns null (never throws) on failure. Constructing at
        // a null address would be undefined behaviour, so bail out here; the OOM is logged at the
        // memory choke point and the caller sees a null object.
        void* const objectMemory = object_class::operator new(sizeof(object_class), alignof(object_class), category);

        if (objectMemory == nullptr)
        {
            return nullptr;
        }

        return ::new (objectMemory) object_class();
    }

    auto unregister_reflection_types() -> void { rttr::detail::get_registration_manager().unregister(); }

    auto register_reflection_types() -> void
    {
        using namespace rttr;
        using namespace sbk::core;
        using namespace sbk::engine;

        registration::enumeration<sc_dsp_type>("sc_dsp_type")(
            value("Fader", SC_DSP_TYPE_FADER),
            value("Lowpass", SC_DSP_TYPE_LOWPASS),
            value("Highpass", SC_DSP_TYPE_HIGHPASS),
            value("Delay", SC_DSP_TYPE_DELAY));

        registration::enumeration<sc_dsp_parameter_type>("sc_dsp_parameter_type")(
            value("Float", sc_dsp_parameter_type_float)
            );

        registration::enumeration<sbk::memory::object_category>("sb_object_category")(
            value("Unkown", sbk::memory::object_category::unknown),
            value("Sound", sbk::memory::object_category::sound),
            value("Node", sbk::memory::object_category::node),
            value("Bus", sbk::memory::object_category::bus),
            value("Music", sbk::memory::object_category::music),
            value("Event", sbk::memory::object_category::event),
            value("Soundbank", sbk::memory::object_category::bank),
            value("Parameter", sbk::memory::object_category::parameter),
            value("Database", sbk::memory::object_category::database_object),
            value("Runtime", sbk::memory::object_category::runtime_object),
            value("System", sbk::memory::object_category::system),
            value("Sound Chef", sbk::memory::object_category::sound_chef),
            value("Data", sbk::memory::object_category::data)
            );

        registration::enumeration<action_type>("action_type")(
            value("Play", action_type::play),
            value("Stop", action_type::stop));

        registration::enumeration<sc_encoding_format>("Encoding Format")(
            value("WAV", sc_encoding_format_wav),
            value("ADPCM", sc_encoding_format_adpcm),
            value("Vorbis", sc_encoding_format_vorbis),
            value("Opus", sc_encoding_format_opus));

        registration::class_<sc_dsp_parameter>("sc_dsp_parameter")
            .constructor<>()(policy::ctor::as_object)
            .property("Type", &sc_dsp_parameter::type)
            .property("Name", &sc_dsp_parameter::name);

        registration::class_<action>("action")
            .constructor<>()(policy::ctor::as_object)
            .property("Type", &action::m_type)
            .property("Destination", &action::m_destination)(metadata(sbk::editor::metadata_key::payload, sbk::editor::PayloadObject));

        registration::class_<effect_description>("effect_description")
            .constructor<>(create_sbk_object<effect_description>)(policy::ctor::as_raw_ptr)
            .property("Type", &effect_description::get_dsp_type, &effect_description::set_dsp_type)(metadata(sbk::editor::metadata_key::readonly, true))
            .property("Parameters", &effect_description::m_parameterDescriptions)(metadata(sbk::editor::metadata_key::no_grow, true), metadata(sbk::editor::metadata_key::no_shrink, true))
            (metadata(sbk::editor::metadata_key::draw_when_wrapped, true));

        registration::class_<effect_parameter_description>("effect_parameter_description")
            .constructor<>()(policy::ctor::as_object)
            .property("Parameter", &effect_parameter_description::m_parameter);

        registration::class_<system>("system");

        registration::class_<game_object>("game_object")
            .constructor<>(create_sbk_object<game_object>)(policy::ctor::as_raw_ptr);

        registration::class_<voice>("voice")
            .constructor<>(create_sbk_object<voice>)(policy::ctor::as_raw_ptr);

        registration::class_<int_property>("int_property")
            .constructor<>()
            .property("Value", &int_property::get, &int_property::set);

        registration::class_<float_property>("float_property")
            .constructor<>()
            .property("Value", &float_property::get, &float_property::set);

        registration::class_<id_property>("id_property")
            .constructor<>()
            .property("Value", &id_property::get, &id_property::set);

        registration::class_<object>("object")
            .constructor<>(create_sbk_object<object>)(policy::ctor::as_raw_ptr)
            .property("ObjectName", &object::get_object_name, &object::set_object_name)(metadata(sbk::editor::metadata_key::hidden_when_wrapped, true));

        registration::class_<database_object>("database_object")
            .constructor<>(create_sbk_object<database_object>)(policy::ctor::as_raw_ptr)
            .property("ObjectID", &database_object::get_database_id, &database_object::set_database_id)(metadata(sbk::editor::metadata_key::readonly, true), metadata(sbk::editor::metadata_key::hidden_when_wrapped, true))
            .property_readonly("DatabaseName", &database_object::get_database_name)(metadata(sbk::editor::metadata_key::readonly, true), metadata(sbk::editor::metadata_key::hidden_when_wrapped, true));

        registration::class_<sound>("sound")
            .constructor<>(create_sbk_object<sound>)(policy::ctor::as_raw_ptr)
            .property("Sound", &sound::get_sound_name, &sound::set_sound_name)(metadata(sbk::editor::metadata_key::payload, sbk::editor::PayloadSound))
            .property("Encoded Sound", &sound::get_encoded_sound_name, &sound::set_encoded_sound_name)(metadata(sbk::editor::metadata_key::payload, sbk::editor::PayloadSound))
            .property("Encoding Format", &sound::m_encodingFormat);

        registration::class_<node_base>("node_base")
            .constructor<>(create_sbk_object<node_base>)(policy::ctor::as_raw_ptr)
            .property("ParentNode", &node_base::m_parentNode)(metadata(sbk::editor::metadata_key::readonly, true))
            .property("OutputBus", &node_base::m_outputBus)(metadata(sbk::editor::metadata_key::payload, sbk::editor::PayloadBus))
            .property("ChildNodes", &node_base::m_childNodes)(metadata(sbk::editor::metadata_key::readonly, true));

        registration::class_<node>("node")
            .constructor<>(create_sbk_object<node>)(policy::ctor::as_raw_ptr)
            .property("Volume", &node::m_volume)(metadata(sbk::editor::metadata_key::min_max, std::pair<float, float>(0.0f, 1.0f)))
            .property("Pitch", &node::m_pitch)(metadata(sbk::editor::metadata_key::min_max, std::pair<float, float>(0.0f, 2.0f)))
            .property("Lowpass", &node::m_lowpass)(metadata(sbk::editor::metadata_key::min_max, std::pair<float, float>(0.0f, 100.0f)))
            .property("Highass", &node::m_highpass)(metadata(sbk::editor::metadata_key::min_max, std::pair<float, float>(0.0f, 100.0f)))
            .property("Effects", &node::m_effectDescriptions)(metadata(sbk::editor::metadata_key::no_grow, true)) // Not edited directly. Modified with the "Add Effect" method
            .method("Add Effect", &node::add_effect)(parameter_names("Type"));

        registration::class_<container>("container");

        registration::class_<sound_container>("sound_container")
            .constructor<>(create_sbk_object<sound_container>)(policy::ctor::as_raw_ptr)
            .property("Sound", &sound_container::m_sound)(metadata(sbk::editor::metadata_key::payload, sbk::editor::PayloadSound));

        registration::class_<blend_container>("blend_container")
            .constructor<>(create_sbk_object<blend_container>)(policy::ctor::as_raw_ptr);

        registration::class_<switch_container>("switch_container")
            .constructor<>(create_sbk_object<switch_container>)(policy::ctor::as_raw_ptr)
            .property("Switch", &switch_container::get_switch_parameters, &switch_container::set_switch_parameter)(metadata(sbk::editor::metadata_key::payload, sbk::editor::PayloadNamedParam))
            .property("Mappings", &switch_container::get_switch_to_child_map, &switch_container::set_switch_to_child);

        registration::class_<random_container>("random_container")
            .constructor<>(create_sbk_object<random_container>)(policy::ctor::as_raw_ptr);

        registration::class_<sequence_container>("sequence_container")
            .constructor<>(create_sbk_object<sequence_container>)(policy::ctor::as_raw_ptr)
            .property("Sequence", &sequence_container::m_sequence);

        registration::class_<bus>("bus")
            .constructor<>(create_sbk_object<bus>)(policy::ctor::as_raw_ptr)
            .property("IsMasterBus", &bus::is_master_bus, &bus::set_master_bus)(metadata(sbk::editor::metadata_key::readonly, true));

        registration::class_<aux_bus>("aux_bus")
            .constructor<>(create_sbk_object<aux_bus>)(policy::ctor::as_raw_ptr);

        registration::class_<event>("event")
            .constructor<>(create_sbk_object<event>)(policy::ctor::as_raw_ptr)
            .property("Actions", &event::m_actions);

        registration::class_<float_parameter>("float_parameter")
            .constructor<>(create_sbk_object<float_parameter>)(policy::ctor::as_raw_ptr);

        registration::class_<int_parameter>("int_parameter")
            .constructor<>(create_sbk_object<int_parameter>)(policy::ctor::as_raw_ptr);

        registration::class_<named_parameter>("named_parameter")
            .constructor<>(create_sbk_object<named_parameter>)(policy::ctor::as_raw_ptr)
            .property("Values", &named_parameter::m_values)(metadata(sbk::editor::metadata_key::readonly, true))
            .property("ParameterValue", &named_parameter::get_selected_value, &named_parameter::set_selected_value);

        registration::class_<named_parameter_value>("named_parameter_value")
            .constructor<>(create_sbk_object<named_parameter_value>)(policy::ctor::as_raw_ptr)
            .property("Parent", &named_parameter_value::parentParameter)(metadata(sbk::editor::metadata_key::readonly, true));

        registration::class_<soundbank>("soundbank")
            .constructor<>(create_sbk_object<soundbank>)(policy::ctor::as_raw_ptr)
            .property("Events", &soundbank::m_events)
            .property("Master", &soundbank::m_initSoundbank)
            .property("Lookup", &soundbank::m_lookupSoundbank);

        registration::class_<node_instance>("node_instance")
            .constructor<>(create_sbk_object<node_instance>)(policy::ctor::as_raw_ptr);

        sbk::reflection::RegisterPointerConversionsForBaseClasses<aux_bus>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<blend_container>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<random_container>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<sequence_container>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<sound_container>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<switch_container>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<container>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<sound>();
        sbk::reflection::RegisterPointerConversionsForBaseClasses<soundbank>();
    }
}  // namespace sbk::reflection