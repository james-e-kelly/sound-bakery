#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class SB_CLASS EffectParameterDescription final
    {
        REGISTER_REFLECTION(EffectParameterDescription)

    public:
        EffectParameterDescription() = default;

        EffectParameterDescription(const sc_dsp_parameter* parameter) { m_parameter = *parameter; }

        sc_dsp_parameter m_parameter;
    };

    /**
     * @brief Wraps a sc_dsp_config
     */
    class SB_CLASS EffectDescription final : public SB::Core::DatabaseObject
    {
        REGISTER_REFLECTION(EffectDescription, DatabaseObject)

    public:
        EffectDescription() : SB::Core::DatabaseObject(), m_config() { setDSPType(SC_DSP_TYPE_LOWPASS); }

        void setDSPType(sc_dsp_type type)
        {
            m_parameterDescriptions.clear();

            m_config = sc_dsp_config_init(type);

            for (int i = 0; i < m_config.vtable->numParams; ++i)
            {
                m_parameterDescriptions.emplace_back(m_config.vtable->params[i]);
            }
        }

        const std::vector<EffectParameterDescription> getParameters() const { return m_parameterDescriptions; }
        const sc_dsp_config* getConfig() const { return &m_config; }
        sc_dsp_type getDSPType() const { return m_config.type; }

    private:
        sc_dsp_config m_config;
        std::vector<EffectParameterDescription> m_parameterDescriptions;
    };
}  // namespace SB::Engine