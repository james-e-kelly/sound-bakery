#include "sound_chef/sound_chef.h"

sbk_status sc_dsp_get_parameter_float(sc_dsp* dsp, int index, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->vtable != NULL);
    SC_CHECK_ARG(dsp->vtable->getFloat != NULL);
    SC_CHECK_ARG(dsp->state != NULL);
    SC_CHECK_ARG(value != NULL);
    SC_CHECK_ARG(index >= 0);

    return dsp->vtable->getFloat(dsp->state, index, value);
}

sbk_status sc_dsp_set_parameter_float(sc_dsp* dsp, int index, float value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->vtable != NULL);
    SC_CHECK_ARG(dsp->vtable->setFloat != NULL);
    SC_CHECK_ARG(dsp->state != NULL);
    SC_CHECK_ARG(index >= 0);

    return dsp->vtable->setFloat(dsp->state, index, value);
}

sbk_status sc_dsp_release(sc_dsp* dsp)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->vtable != NULL);
    SC_CHECK_ARG(dsp->vtable->release != NULL);
    SC_CHECK_ARG(dsp->state != NULL);

    const ma_allocation_callbacks* allocCallbacks = &((sc_system*)dsp->state->system)->engine.allocationCallbacks;

    sbk_status result = dsp->vtable->release(dsp->state);
    ma_free(dsp->state, allocCallbacks);
    ma_free(dsp, allocCallbacks);

    return result;
}