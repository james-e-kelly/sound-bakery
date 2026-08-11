#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

enum
{
    SC_DSP_CUTOFF_MIN           = 1,
    SC_DSP_CUTOFF_MAX           = 22000,
    SC_DSP_DEFAULT_FILTER_ORDER = 2,
};

static sbk_status sc_dsp_lowpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_lpf_node), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return SBK_ERR_OUT_OF_MEMORY;
    }

    ma_lpf_node_config config = ma_lpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);
    return SBK_FROM_MA(ma_lpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_lpf_node*)state->userData));
}

static sbk_status sc_dsp_lowpass_release(sc_dsp_state* state)
{
    ma_lpf_node_uninit((ma_lpf_node*)state->userData, NULL);
    SC_FREE(state->userData, (sc_system*)state->system);
    return SBK_SUCCESS;
}

static sbk_status sc_dsp_lowpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sbk_status result = SBK_ERR_CHEF;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_LOWPASS_PARAM_CUTOFF:
        {
            ma_lpf_config lpfConfig =
                ma_lpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = SBK_FROM_MA(ma_lpf_node_reinit(&lpfConfig, state->userData));
            break;
        }
    }

    return result;
}

static sbk_status sc_dsp_lowpass_get_param_float(sc_dsp_state* const state, int index, float* const value)
{
    (void)state;
    (void)value;

    sbk_status result = SBK_ERR_CHEF;

    switch (index)
    {
        default:
        case SC_DSP_LOWPASS_PARAM_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_lowpassCutoffParam = {sc_dsp_parameter_type_float, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MAX};

static sc_dsp_parameter* s_lowpassParams[SC_DSP_LOWPASS_PARAM_COUNT] = {&s_lowpassCutoffParam};

sc_dsp_vtable g_dspLowpassVTable =
{
    sc_dsp_lowpass_create,
    sc_dsp_lowpass_release,
    NULL, // Idle
    sc_dsp_lowpass_set_param_float,
    sc_dsp_lowpass_get_param_float,
    s_lowpassParams,
    SC_DSP_LOWPASS_PARAM_COUNT
};
