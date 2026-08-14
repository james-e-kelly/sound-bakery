#include "sound_chef/sound_chef_internal.h"

sbk_status sc_dsp_release(sc_dsp* dsp)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->node != NULL);
    SC_CHECK_ARG(dsp->system != NULL);

    const sc_dsp_description* description = NULL;
    SC_CHECK_STATUS(sc_system_get_dsp_desc(dsp->system, dsp->handle, &description));

    (void)description->release(dsp->system, dsp);

    SC_FREE(dsp, dsp->system);

    return SBK_SUCCESS;
}

sbk_status sc_dsp_get_parameter_float(sc_dsp* dsp, sc_uint32 index, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(value != NULL);

    const sc_system* const system = (const sc_system*)((ma_node_base*)dsp->node)->pNodeGraph;

    const sc_dsp_description* description = NULL;
    SC_CHECK_STATUS(sc_system_get_dsp_desc(system, dsp->handle, &description));

    return description->getFloat(dsp, index, value);
}

sbk_status sc_dsp_set_parameter_float(sc_dsp* dsp, sc_uint32 index, float value)
{
    SC_CHECK_ARG(dsp != NULL);

    const sc_system* const system = (const sc_system*)((ma_node_base*)dsp->node)->pNodeGraph;

    const sc_dsp_description* description = NULL;
    SC_CHECK_STATUS(sc_system_get_dsp_desc(system, dsp->handle, &description));

    return description->setFloat(dsp, index, value);
}