#include "sound_chef/sound_chef.h"

#include <string.h>  // for memset

enum
{
    SC_DSP_CUTOFF_MIN           = 1,
    SC_DSP_CUTOFF_MAX           = 22000,
    SC_DSP_DEFAULT_FILTER_ORDER = 2,
};

/**
 * FADER
 */

static sc_result sc_dsp_fader_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_sound_group), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_sound_group_config config = ma_sound_group_config_init_2((ma_engine*)state->system);
    return ma_sound_group_init_ex((ma_engine*)state->system, &config, (ma_sound_group*)state->userData);
}

static sc_result sc_dsp_fader_release(sc_dsp_state* state)
{
    ma_sound_group_uninit((ma_sound_group*)state->userData);
    return MA_SUCCESS;
}

static sc_dsp_vtable s_faderVtable = {sc_dsp_fader_create, sc_dsp_fader_release};

/**
 * LOWPASS
 */

static sc_result sc_dsp_lowpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_lpf_node), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_lpf_node_config config = ma_lpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_lpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_lpf_node*)state->userData);
}

static sc_result sc_dsp_lowpass_release(sc_dsp_state* state)
{
    ma_lpf_node_uninit((ma_lpf_node*)state->userData, NULL);
    return MA_SUCCESS;
}

static sc_result sc_dsp_lowpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sc_result result = MA_ERROR;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_LOWPASS_CUTOFF:
        {
            ma_lpf_config lpfConfig =
                ma_lpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_lpf_node_reinit(&lpfConfig, state->userData);
            break;
        }
    }

    return result;
}

static sc_result sc_dsp_lowpass_get_param_float(sc_dsp_state* const state, int index, float* const value)
{
    (void)state;
    (void)value;

    sc_result result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_LOWPASS_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_lowpassCutoffParam = {SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MAX};

static sc_dsp_parameter* s_lowpassParams[SC_DSP_LOWPASS_NUM_PARAM] = {&s_lowpassCutoffParam};

static sc_dsp_vtable s_lowpassVtable = {
    sc_dsp_lowpass_create,          sc_dsp_lowpass_release, sc_dsp_lowpass_set_param_float,
    sc_dsp_lowpass_get_param_float, s_lowpassParams,        SC_DSP_LOWPASS_NUM_PARAM};

/**
 * HIGHPASS
 */

static sc_result sc_dsp_highpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_hpf_node), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_hpf_node_config config = ma_hpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_hpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_hpf_node*)state->userData);
}

static sc_result sc_dsp_highpass_release(sc_dsp_state* state)
{
    ma_hpf_node_uninit((ma_hpf_node*)state->userData, NULL);
    return MA_SUCCESS;
}

static sc_result sc_dsp_highpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sc_result result = MA_ERROR;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_HIGHPASS_CUTOFF:
        {
            ma_hpf_config hpfConfig =
                ma_hpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_hpf_node_reinit(&hpfConfig, state->userData);
            break;
        }
    }

    return result;
}

static sc_result sc_dsp_highpass_get_param_float(sc_dsp_state* state, int index, float* const value)
{
    (void)state;
    (void)value;

    sc_result result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_HIGHPASS_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_highpassCutoffParam = {SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                 SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MIN};

static sc_dsp_parameter* s_highpassParams[SC_DSP_HIGHPASS_NUM_PARAM] = {&s_highpassCutoffParam};

static sc_dsp_vtable s_highpassVtable = {
    sc_dsp_highpass_create,          sc_dsp_highpass_release, sc_dsp_highpass_set_param_float,
    sc_dsp_highpass_get_param_float, s_highpassParams,        SC_DSP_HIGHPASS_NUM_PARAM};

/**
 * FUNCTIONS
 */

sc_dsp_config sc_dsp_config_init(sc_dsp_type type)
{
    sc_dsp_config result;
    memset(&result, 0, sizeof(sc_dsp_config));

    result.type = type;

    switch (type)
    {
        default:
        case SC_DSP_TYPE_UNKOWN:
            break;
        case SC_DSP_TYPE_FADER:
            result.vtable = &s_faderVtable;
            break;
        case SC_DSP_TYPE_LOWPASS:
            result.vtable = &s_lowpassVtable;
            break;
        case SC_DSP_TYPE_HIGHPASS:
            result.vtable = &s_highpassVtable;
            break;
        case SC_DSP_TYPE_DELAY:
            break;
    }

    return result;
}