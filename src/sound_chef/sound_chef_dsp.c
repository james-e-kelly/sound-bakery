#include "sound_chef.h"
#include "sound_chef_dsp.h"

#include <string.h>  // for memset

#define SC_DSP_CUTOFF_MIN           1
#define SC_DSP_CUTOFF_MAX           22000
#define SC_DSP_DEFAULT_FILTER_ORDER 2

/**
 * FADER
 */

static SC_RESULT SC_DSP_FADER_CREATE(SC_DSP_STATE* state)
{
    state->m_userData = ma_malloc(sizeof(ma_sound_group), NULL);
    if (state->m_userData == NULL)
        return MA_OUT_OF_MEMORY;

    ma_sound_group_config config =
        ma_sound_group_config_init_2((ma_engine*)state->m_system);
    return ma_sound_group_init_ex((ma_engine*)state->m_system, &config,
                                  (ma_sound_group*)state->m_userData);
}

static SC_RESULT SC_DSP_FADER_RELEASE(SC_DSP_STATE* state)
{
    ma_sound_group_uninit((ma_sound_group*)state->m_userData);
    return MA_SUCCESS;
}

static SC_DSP_VTABLE s_faderVtable = {SC_DSP_FADER_CREATE,
                                      SC_DSP_FADER_RELEASE};

/**
 * LOWPASS
 */

static SC_RESULT SC_DSP_LOWPASS_CREATE(SC_DSP_STATE* state)
{
    state->m_userData = ma_malloc(sizeof(ma_lpf_node), NULL);
    if (state->m_userData == NULL)
        return MA_OUT_OF_MEMORY;

    ma_lpf_node_config config = ma_lpf_node_config_init(
        ma_engine_get_channels((ma_engine*)state->m_system),
        ma_engine_get_sample_rate((ma_engine*)state->m_system),
        SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_lpf_node_init((ma_node_graph*)state->m_system, &config, NULL,
                            (ma_lpf_node*)state->m_userData);
}

static SC_RESULT SC_DSP_LOWPASS_RELEASE(SC_DSP_STATE* state)
{
    ma_lpf_node_uninit((ma_lpf_node*)state->m_userData, NULL);
    return MA_SUCCESS;
}

static SC_RESULT SC_DSP_LOWPASS_SET_PARAM_FLOAT(SC_DSP_STATE* state,
                                                int index,
                                                float value)
{
    (void)value;

    SC_RESULT result = MA_ERROR;

    ma_format format   = ma_format_f32;
    ma_uint32 channels = ma_node_get_output_channels(state->m_userData, 0);
    ma_uint32 sampleRate =
        ma_engine_get_sample_rate((ma_engine*)state->m_system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_LOWPASS_CUTOFF:
        {
            ma_lpf_config lpfConfig =
                ma_lpf_config_init(format, channels, sampleRate, value,
                                   SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_lpf_node_reinit(&lpfConfig, state->m_userData);
            break;
        }
    }

    return result;
}

static SC_RESULT SC_DSP_LOWPASS_GET_PARAM_FLOAT(SC_DSP_STATE* const state,
                                                int index,
                                                float* const value)
{
    (void)state;
    (void)value;

    SC_RESULT result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_LOWPASS_CUTOFF:
            break;
    }

    return result;
}

static SC_DSP_PARAMETER s_lowpassCutoffParam = {
    SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN, SC_DSP_CUTOFF_MAX,
    SC_DSP_CUTOFF_MAX};

static SC_DSP_PARAMETER* s_lowpassParams[SC_DSP_LOWPASS_NUM_PARAM] = {
    &s_lowpassCutoffParam};

static SC_DSP_VTABLE s_lowpassVtable = {SC_DSP_LOWPASS_CREATE,
                                        SC_DSP_LOWPASS_RELEASE,
                                        SC_DSP_LOWPASS_SET_PARAM_FLOAT,
                                        SC_DSP_LOWPASS_GET_PARAM_FLOAT,
                                        s_lowpassParams,
                                        SC_DSP_LOWPASS_NUM_PARAM};

/**
 * HIGHPASS
 */

static SC_RESULT SC_DSP_HIGHPASS_CREATE(SC_DSP_STATE* state)
{
    state->m_userData = ma_malloc(sizeof(ma_hpf_node), NULL);
    if (state->m_userData == NULL)
        return MA_OUT_OF_MEMORY;

    ma_hpf_node_config config = ma_hpf_node_config_init(
        ma_engine_get_channels((ma_engine*)state->m_system),
        ma_engine_get_sample_rate((ma_engine*)state->m_system),
        SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_hpf_node_init((ma_node_graph*)state->m_system, &config, NULL,
                            (ma_hpf_node*)state->m_userData);
}

static SC_RESULT SC_DSP_HIGHPASS_RELEASE(SC_DSP_STATE* state)
{
    ma_hpf_node_uninit((ma_hpf_node*)state->m_userData, NULL);
    return MA_SUCCESS;
}

static SC_RESULT SC_DSP_HIGHPASS_SET_PARAM_FLOAT(SC_DSP_STATE* state,
                                                 int index,
                                                 float value)
{
    (void)value;

    SC_RESULT result = MA_ERROR;

    ma_format format   = ma_format_f32;
    ma_uint32 channels = ma_node_get_output_channels(state->m_userData, 0);
    ma_uint32 sampleRate =
        ma_engine_get_sample_rate((ma_engine*)state->m_system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_HIGHPASS_CUTOFF:
        {
            ma_hpf_config hpfConfig =
                ma_hpf_config_init(format, channels, sampleRate, value,
                                   SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_hpf_node_reinit(&hpfConfig, state->m_userData);
            break;
        }
    }

    return result;
}

static SC_RESULT SC_DSP_HIGHPASS_GET_PARAM_FLOAT(SC_DSP_STATE* state,
                                                 int index,
                                                 const float* const value)
{
    (void)state;
    (void)value;

    SC_RESULT result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_HIGHPASS_CUTOFF:
            break;
    }

    return result;
}

static SC_DSP_PARAMETER s_highpassCutoffParam = {
    SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN, SC_DSP_CUTOFF_MAX,
    SC_DSP_CUTOFF_MIN};

static SC_DSP_PARAMETER* s_highpassParams[SC_DSP_HIGHPASS_NUM_PARAM] = {
    &s_highpassCutoffParam};

static SC_DSP_VTABLE s_highpassVtable = {SC_DSP_HIGHPASS_CREATE,
                                         SC_DSP_HIGHPASS_RELEASE,
                                         SC_DSP_HIGHPASS_SET_PARAM_FLOAT,
                                         SC_DSP_HIGHPASS_GET_PARAM_FLOAT,
                                         s_highpassParams,
                                         SC_DSP_HIGHPASS_NUM_PARAM};

/**
 * FUNCTIONS
 */

SC_DSP_CONFIG SC_DSP_Config_Init(SC_DSP_TYPE type)
{
    SC_DSP_CONFIG result;
    memset(&result, 0, sizeof(SC_DSP_CONFIG));

    result.m_type = type;

    switch (type)
    {
        default:
        case SC_DSP_TYPE_UNKOWN:
            break;
        case SC_DSP_TYPE_FADER:
            result.m_vtable = &s_faderVtable;
            break;
        case SC_DSP_TYPE_LOWPASS:
            result.m_vtable = &s_lowpassVtable;
            break;
        case SC_DSP_TYPE_HIGHPASS:
            result.m_vtable = &s_highpassVtable;
            break;
        case SC_DSP_TYPE_DELAY:
            break;
    }

    return result;
}