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
        effect_description() : sbk::core::database_object(), m_config() { set_dsp_type(SC_DSP_TYPE_LOWPASS); }

        auto set_dsp_type(sc_dsp_type type) -> void
        {
            m_parameterDescriptions.clear();

            m_config = sc_dsp_config_init(type);
            BOOST_ASSERT(m_config.vtable != nullptr);

            for (int i = 0; i < m_config.vtable->numParams; ++i)
            {
                m_parameterDescriptions.emplace_back(m_config.vtable->params[i]);
            }
        }

        auto set_dsp_clap(const clap_plugin_factory* pluginFactory) -> void
        {
            m_parameterDescriptions.clear();

            m_config = sc_dsp_config_init_clap(pluginFactory);
            BOOST_ASSERT(m_config.vtable != nullptr);

            for (int i = 0; i < m_config.vtable->numParams; ++i)
            {
                m_parameterDescriptions.emplace_back(m_config.vtable->params[i]);
            }
        }

        [[nodiscard]] auto get_parameters() const -> eastl::vector<effect_parameter_description>
        {
            return m_parameterDescriptions;
        }
        [[nodiscard]] auto get_config() const -> const sc_dsp_config* { return &m_config; }
        [[nodiscard]] auto get_dsp_type() const -> sc_dsp_type { return m_config.type; }

    private:
        sc_dsp_config m_config;
        eastl::vector<effect_parameter_description> m_parameterDescriptions;
    };
}  // namespace sbk::engine