#include "effect.h"

#include "sound_bakery/system.h"
#include "sound_bakery/runtime/runtime.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::effect_parameter_description)

DEFINE_REFLECTION(sbk::engine::effect_description)

sbk::engine::effect_parameter_description::effect_parameter_description(const sc_dsp_parameter* parameter)
{
    set_dsp_parameter(*parameter);
}

auto sbk::engine::effect_parameter_description::get_dsp_parameter() const -> sc_dsp_parameter
{
    sc_dsp_parameter parameter{};
    parameter.type = m_type;
    std::memcpy(parameter.name, m_name, SC_STRING_NAME_LENGTH);

    switch (m_type)
    {
        case sc_dsp_parameter_type_float:
        {
            const auto& floatProperty  = m_property.get_value<sbk::core::float_property>();
            parameter.floatParameter.min   = floatProperty.get_min();
            parameter.floatParameter.max   = floatProperty.get_max();
            parameter.floatParameter.value = floatProperty.get();
            break;
        }
        case sc_dsp_parameter_type_int:
        {
            const auto& intProperty  = m_property.get_value<sbk::core::int_property>();
            parameter.intParameter.min   = intProperty.get_min();
            parameter.intParameter.max   = intProperty.get_max();
            parameter.intParameter.value = intProperty.get();
            break;
        }
    }

    return parameter;
}

auto sbk::engine::effect_parameter_description::set_dsp_parameter(sc_dsp_parameter parameter) -> void
{
    m_type = parameter.type;
    std::memcpy(m_name, parameter.name, SC_STRING_NAME_LENGTH);

    switch (m_type)
    {
        case sc_dsp_parameter_type_float:
        {
            m_property = sbk::core::float_property(parameter.floatParameter.value, parameter.floatParameter.min, parameter.floatParameter.max);
            break;
        }
        case sc_dsp_parameter_type_int:
        {
            m_property = sbk::core::int_property(parameter.intParameter.value, parameter.intParameter.min, parameter.intParameter.max);
            break;
        }
    }
}

auto sbk::engine::effect_description::set_dsp_type(sc_dsp_type type) -> void
{
    m_parameterDescriptions.clear();

    const sc_uint32 handle = static_cast<sc_uint32>(type);

    const sc_dsp_description* description{};
    if (sc_system_get_dsp_desc(get_runtime(), handle, &description) == SBK_SUCCESS)
    {
        m_effectHandle = handle;

        for (sc_uint32 i = 0; i < description->numParams; ++i)
        {
            m_parameterDescriptions.emplace_back(description->params[i]);
        }
    }
}

auto sbk::engine::effect_description::set_dsp_clap(const clap_plugin_factory* pluginFactory) -> void
{
    m_parameterDescriptions.clear();

    const sc_uint32 handle = static_cast<sc_uint32>(sc_dsp_type_clap);

    const sc_dsp_description* description{};
    if (sc_system_get_dsp_desc(get_runtime(), handle, &description) == SBK_SUCCESS)
    {
        m_effectHandle = handle;

        /// @todo Add introspection for CLAP plugins
        /// We very likely need to create the CLAP plugin to get its parameters
    }
}
