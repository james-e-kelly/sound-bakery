#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/core/property.h"

namespace sbk::engine
{
    /**
     * @brief Reimplements a @ref sc_dsp_parameter in a C++ way that can be serialized and rendered easily.
     * 
     * sc_dsp_parameter uses a union to switch between the different parameter types.
     * effect_parameter_description uses a @ref rttr::variant to do the same thing.
     * The type and name is reimplemented.
     * 
     * Users can still use a @ref sc_dsp_parameter to set values on this class.
     * Users can still retrieve a @ref sc_dsp_parameter from this class.
     */
    class SB_CLASS effect_parameter_description final
    {
        REGISTER_REFLECTION(effect_parameter_description)

    public:
        effect_parameter_description() = default;
        effect_parameter_description(const sc_dsp_parameter* parameter);

        [[nodiscard]] auto get_name() const -> const char* { return m_name; }
        [[nodiscard]] auto get_parameter_type() const -> sc_dsp_parameter_type { return m_type; }
        [[nodiscard]] auto get_property() -> rttr::variant& { return m_property; }

        [[nodiscard]] auto get_dsp_parameter() const -> sc_dsp_parameter;
        auto set_dsp_parameter(sc_dsp_parameter parameter) -> void;

    private:
        sc_dsp_parameter_type m_type{};
        char m_name[SC_STRING_NAME_LENGTH]{};
        rttr::variant m_property;
    };

    /**
     * @brief Reimplements a @ref sc_dsp_description in a C++ way so it can be rendered and serialized.
     * 
     * The sc_dsp_description itself is not stored in the class.
     * Instead, the dsp handle is stored and the parameters copied into @ref effect_parameter_description objects.
     * 
     * Users wanting to create @ref sc_dsp objects from this can call @ref effect_description::get_dsp_handle and pass that to @ref sc_dsp_config_init_handle.
     * 
     * Example:
     * @code
     *      sc_dsp* dsp{};
     *      sc_dsp_config config = sc_dsp_config_init_handle(system, effectDescription->get_dsp_handle());
     *      sc_system_create_dsp(system, &config, &dsp);
     * @endcode
     */
    class SB_CLASS effect_description final : public sbk::core::database_object
    {
        REGISTER_REFLECTION(effect_description, database_object)

    public:
        effect_description() : sbk::core::database_object() {}

        auto set_dsp_type(sc_dsp_type type) -> void;
        auto set_dsp_clap(const clap_plugin_factory* pluginFactory) -> void;

        [[nodiscard]] auto get_parameters() -> eastl::vector<effect_parameter_description>& { return m_parameterDescriptions; }
        [[nodiscard]] auto get_parameters() const -> const eastl::vector<effect_parameter_description>& { return m_parameterDescriptions; }
        [[nodiscard]] auto get_dsp_type() const -> sc_dsp_type { return static_cast<sc_dsp_type>(m_effectHandle); }
        [[nodiscard]] auto get_dsp_handle() const -> sc_uint32 { return m_effectHandle; }

    private:
        sc_uint32 m_effectHandle{}; //< Handle is basically an index into an array of dsp descriptions
        eastl::vector<effect_parameter_description> m_parameterDescriptions;
    };
}  // namespace sbk::engine
