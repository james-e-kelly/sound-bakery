#include "effect.h"

#include "sound_bakery/system.h"
#include "sound_bakery/runtime/runtime.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::effect_parameter_description)

DEFINE_REFLECTION(sbk::engine::effect_description)

auto sbk::engine::effect_description::set_dsp_type(sc_dsp_type type) -> void
{
    m_parameterDescriptions.clear();

    const sc_uint32 handle = static_cast<sc_uint32>(type);

    const sc_dsp_description* description{};
    if (sc_system_get_dsp_desc(get_system()->get_runtime(), handle, &description) == SBK_SUCCESS)
    {
        m_effectHandle = handle;

        for (int i = 0; i < description->numParams; ++i)
        {
            m_parameterDescriptions.emplace_back(description->params[i]);
        }
    }
}

auto sbk::engine::effect_description::set_dsp_clap(const clap_plugin_factory* pluginFactory) -> void
{
    m_parameterDescriptions.clear();

    const sc_uint32 handle = static_cast<sc_uint32>(SC_DSP_TYPE_CLAP);

    const sc_dsp_description* description{};
    if (sc_system_get_dsp_desc(get_system()->get_runtime(), handle, &description) == SBK_SUCCESS)
    {
        m_effectHandle = handle;

        /// @todo Add introspection for CLAP plugins
        /// We very likely need to create the CLAP plugin to get its parameters
    }
}
