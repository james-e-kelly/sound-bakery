#pragma once

#include "sound_bakery/core/database/database_object.h"

namespace sbk::engine
{
    class SB_CLASS effect_parameter_description final
    {
        REGISTER_REFLECTION(effect_parameter_description)

    public:
        effect_parameter_description() = default;

        effect_parameter_description(const sc_dsp_parameter* parameter) { m_parameter = *parameter; }

        sc_dsp_parameter m_parameter;
    };

    /**
     * @brief Wraps a sc_dsp_config
     */
    class SB_CLASS effect_description final : public sbk::core::database_object
    {
        REGISTER_REFLECTION(effect_description, database_object)

    public:
        effect_description() : sbk::core::database_object() {}

        auto set_dsp_type(sc_dsp_type type) -> void;
        auto set_dsp_clap(const clap_plugin_factory* pluginFactory) -> void;

        [[nodiscard]] auto get_parameters() const -> eastl::vector<effect_parameter_description>
        {
            return m_parameterDescriptions;
        }
        [[nodiscard]] auto get_dsp_type() const -> sc_dsp_type { return static_cast<sc_dsp_type>(m_effectHandle); }
        [[nodiscard]] auto get_dsp_handle() const -> sc_uint32 { return m_effectHandle; }

    private:
        sc_uint32 m_effectHandle{}; //< Handle is basically an index into an array of dsp descriptions
        eastl::vector<effect_parameter_description> m_parameterDescriptions;
    };
}  // namespace sbk::engine