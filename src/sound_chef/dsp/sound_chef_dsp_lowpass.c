#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

enum
{
    SC_DSP_CUTOFF_MIN           = 1,
    SC_DSP_CUTOFF_MAX           = 22000,
    SC_DSP_DEFAULT_FILTER_ORDER = 2,
};

static sbk_status sc_dsp_lowpass_create(sc_system* system, sc_dsp* dsp, void* userData)
{
    SC_CREATE(dsp->node, ma_lpf_node, system);

    ma_lpf_node_config config = ma_lpf_node_config_init(ma_engine_get_channels((ma_engine*)system),
                                                        ma_engine_get_sample_rate((ma_engine*)system),
                                                        SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);
    return SBK_FROM_MA(ma_lpf_node_init((ma_node_graph*)system, &config, &system->engine.allocationCallbacks, (ma_lpf_node*)dsp->node));
}

static sbk_status sc_dsp_lowpass_release(sc_system* system, sc_dsp* dsp)
{
    ma_lpf_node_uninit((ma_lpf_node*)dsp->node, &system->engine.allocationCallbacks);
    SC_FREE(dsp->node, (sc_system*)system);
    return SBK_SUCCESS;
}

static sbk_status sc_dsp_lowpass_set_param_float(sc_dsp* dsp, int index, float value)
{
    (void)value;

    sbk_status result = SBK_ERR_CHEF;

    const ma_format format     = ma_format_f32;
    const ma_uint32 channels   = ma_node_get_output_channels(dsp->node, 0);
    const ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)dsp->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_LOWPASS_PARAM_CUTOFF:
        {
            const ma_lpf_config lpfConfig = ma_lpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = SBK_FROM_MA(ma_lpf_node_reinit(&lpfConfig, dsp->node));
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

static sc_dsp_parameter s_lowpassCutoffParam = 
{
    sc_dsp_parameter_type_float, 
    "Cutoff", 
    SC_DSP_CUTOFF_MIN,
    SC_DSP_CUTOFF_MAX, 
    SC_DSP_CUTOFF_MAX
};

static sc_dsp_parameter* s_lowpassParams[SC_DSP_LOWPASS_PARAM_COUNT] = 
{
    &s_lowpassCutoffParam
};

sc_dsp_description g_dspLowpassVTable =
{
    sc_dsp_lowpass_create,
    sc_dsp_lowpass_release,
    NULL, // Idle
    sc_dsp_lowpass_set_param_float,
    sc_dsp_lowpass_get_param_float,
    s_lowpassParams,
    SC_DSP_LOWPASS_PARAM_COUNT
};
