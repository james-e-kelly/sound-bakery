#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

enum
{
    SC_DSP_CUTOFF_MIN           = 1,
    SC_DSP_CUTOFF_MAX           = 22000,
    SC_DSP_DEFAULT_FILTER_ORDER = 2,
};

static sbk_status sc_dsp_highpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_hpf_node), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return SBK_ERR_OUT_OF_MEMORY;
    }

    ma_hpf_node_config config = ma_hpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);
    return SBK_FROM_MA(ma_hpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_hpf_node*)state->userData));
}

static sbk_status sc_dsp_highpass_release(sc_dsp_state* state)
{
    ma_hpf_node_uninit((ma_hpf_node*)state->userData, NULL);
    SC_FREE(state->userData, (sc_system*)state->system);
    return SBK_SUCCESS;
}

static sbk_status sc_dsp_highpass_set_param_float(sc_dsp_state* state, int index, float value)
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
        case SC_DSP_HIGHPASS_PARAM_CUTOFF:
        {
            ma_hpf_config hpfConfig =
                ma_hpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = SBK_FROM_MA(ma_hpf_node_reinit(&hpfConfig, state->userData));
            break;
        }
    }

    return result;
}

static sbk_status sc_dsp_highpass_get_param_float(sc_dsp_state* state, int index, float* const value)
{
    (void)state;
    (void)value;

    sbk_status result = SBK_ERR_CHEF;

    switch (index)
    {
        default:
        case SC_DSP_HIGHPASS_PARAM_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_highpassCutoffParam = {sc_dsp_parameter_type_float, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                 SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MIN};

static sc_dsp_parameter* s_highpassParams[SC_DSP_HIGHPASS_PARAM_COUNT] = {&s_highpassCutoffParam};

sc_dsp_vtable g_dspHighpassVTable =
{
    sc_dsp_highpass_create,
    sc_dsp_highpass_release,
    NULL, // Idle
    sc_dsp_highpass_set_param_float,
    sc_dsp_highpass_get_param_float,
    s_highpassParams,
    SC_DSP_HIGHPASS_PARAM_COUNT
};
