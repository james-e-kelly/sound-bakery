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

#ifdef __cplusplus
}
#endif

#endif