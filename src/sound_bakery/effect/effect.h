#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Engine
{
    class EffectParameterDescription final
    {
    public:
        EffectParameterDescription() = default;

        EffectParameterDescription(const SC_DSP_PARAMETER* parameter)
        {
            m_parameter = *parameter;
        }

        SC_DSP_PARAMETER m_parameter;

    private:
        RTTR_ENABLE()
        RTTR_REGISTRATION_FRIEND
    };

    /**
     * @brief Wraps a SC_DSP_CONFIG
    */
    class EffectDescription final : public SB::Core::DatabaseObject
    {
    public:
        EffectDescription()
            : SB::Core::DatabaseObject(),
            m_config()
        {
            setDSPType(SC_DSP_TYPE_LOWPASS);
        }

        void setDSPType(SC_DSP_TYPE type)
        {
            m_parameterDescriptions.clear();

            m_config = SC_DSP_Config_Init(type);

            for (int i = 0; i < m_config.m_vtable->m_numParams; ++i)
            {
                m_parameterDescriptions.emplace_back(m_config.m_vtable->m_params[i]);
            }
        }

        const std::vector<EffectParameterDescription> getParameters() const { return m_parameterDescriptions; }
        const SC_DSP_CONFIG* getConfig() const                              { return &m_config; }
        SC_DSP_TYPE getDSPType() const                                      { return m_config.m_type; }

    private:
        SC_DSP_CONFIG m_config;
        std::vector<EffectParameterDescription> m_parameterDescriptions;

        RTTR_ENABLE(DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };
}