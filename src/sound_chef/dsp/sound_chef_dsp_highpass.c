#include "sound_chef/sound_chef.h"

static sbk_status sc_dsp_highpass_create(sc_system* system, sc_dsp* dsp, const void* userData)
{
    (void)userData;
    SC_CREATE(dsp->node, ma_hpf_node, system);

    ma_hpf_node_config config = ma_hpf_node_config_init(ma_engine_get_channels((ma_engine*)system),
                                                        ma_engine_get_sample_rate((ma_engine*)system),
                                                        SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);
    return SC_STATUS_FROM_MA_RESULT(ma_hpf_node_init((ma_node_graph*)system, &config, &system->engine.allocationCallbacks, (ma_hpf_node*)dsp->node));
}

static sbk_status sc_dsp_highpass_release(sc_system* system, sc_dsp* dsp)
{
    ma_hpf_node_uninit((ma_hpf_node*)dsp->node, &system->engine.allocationCallbacks);
    SC_FREE(dsp->node, system);
    return SBK_SUCCESS;
}

static sbk_status sc_dsp_highpass_set_param_float(sc_dsp* dsp, sc_uint32 index, float value)
{
    sbk_status result = SBK_ERR_CHEF;

    const ma_format format     = ma_format_f32;
    const ma_uint32 channels   = ma_node_get_output_channels(dsp->node, 0);
    const ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)dsp->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_HIGHPASS_PARAM_CUTOFF:
        {
            const ma_hpf_config hpfConfig = ma_hpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = SC_STATUS_FROM_MA_RESULT(ma_hpf_node_reinit(&hpfConfig, dsp->node));
            break;
        }
    }

    return result;
}

static sbk_status sc_dsp_highpass_get_param_float(sc_dsp* dsp, sc_uint32 index, float* const value)
{
    (void)dsp;
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

static sc_dsp_parameter s_highpassCutoffParam = 
{
    sc_dsp_parameter_type_float, 
    "Cutoff", 
    SC_DSP_CUTOFF_MIN,
    SC_DSP_CUTOFF_MAX, 
    SC_DSP_CUTOFF_MIN
};

static sc_dsp_parameter* s_highpassParams[SC_DSP_HIGHPASS_PARAM_COUNT] = 
{
    &s_highpassCutoffParam
};

sc_dsp_description g_dspHighpassVTable =
{
    sc_dsp_highpass_create,
    sc_dsp_highpass_release,
    NULL, // Idle
    sc_dsp_highpass_set_param_float,
    sc_dsp_highpass_get_param_float,
    s_highpassParams,
    SC_DSP_HIGHPASS_PARAM_COUNT
};
