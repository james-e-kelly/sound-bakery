/**
 * @file
 * @brief DSP object definitions.
 *
 *
 */

#ifndef SOUND_CHEF_DSP
#define SOUND_CHEF_DSP

enum
{
    SC_STRING_NAME_LENGTH = 16
};

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum sc_dsp_parameter_type
    {
        SC_DSP_PARAMETER_TYPE_FLOAT
    } sc_dsp_parameter_type;

    typedef struct sc_dsp_parameter_float
    {
        float min;
        float max;
        float value;
    } sc_dsp_parameter_float;

    struct sc_dsp_parameter
    {
        sc_dsp_parameter_type type;
        char name[SC_STRING_NAME_LENGTH];

        union
        {
            sc_dsp_parameter_float floatParameter;
        };
    };

    typedef enum sc_dsp_lowpass
    {
        SC_DSP_LOWPASS_CUTOFF,
        SC_DSP_LOWPASS_NUM_PARAM
    } sc_dsp_lowpass;

    typedef enum sc_dsp_highpass
    {
        SC_DSP_HIGHPASS_CUTOFF,
        SC_DSP_HIGHPASS_NUM_PARAM
    } sc_dsp_highpass;

    typedef enum sc_dsp_delay
    {
        SC_DSP_DELAY_DELAY_SECONDS,
        SC_DSP_DELAY_DRY,
        SC_DSP_DELAY_WET,
        SC_DSP_DELAY_FEEDBACK,
        SC_DSP_DELAY_NUM_PARAM
    } sc_dsp_delay;

    typedef enum sc_dsp_meter
    {
        SC_DSP_METER_PEAK,
        SC_DSP_METER_RMS,
        SC_DSP_METER_NUM_PARAM
    } sc_dsp_meter;

    enum
    {
        SC_DSP_METER_MAX_CHANNELS = SC_MAX_CHANNELS
    };

    typedef struct sc_meter
    {
        ma_atomic_uint32 channels;
        ma_atomic_float peakLevels[SC_DSP_METER_MAX_CHANNELS];
        ma_atomic_float rmsLevels[SC_DSP_METER_MAX_CHANNELS];
    } sc_meter;

    typedef struct sc_meter_node
    {
        ma_node_base baseNode;
        sc_meter meter;
    } sc_meter_node;

    typedef struct sc_clap_node
    {
        ma_node_base baseNode;
        const clap_plugin_t* clapPlugin;
        sc_bool isProcessing;  //< Whether start_processing() has been called on the plugin.
    } sc_clap_node;

#ifdef __cplusplus
}
#endif

#endif