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

#define SC_DELAY_SILENCE_THRESHOLD 0.0001F

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum sc_dsp_parameter_type
    {
        sc_dsp_parameter_type_float
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

    enum
    {
        SC_DSP_LOWPASS_PARAM_CUTOFF,
        SC_DSP_LOWPASS_PARAM_COUNT
    };

    enum
    {
        SC_DSP_HIGHPASS_PARAM_CUTOFF,
        SC_DSP_HIGHPASS_PARAM_COUNT
    };

    enum
    {
        SC_DSP_DELAY_PARAM_DELAY_SECONDS,
        SC_DSP_DELAY_PARAM_DRY,
        SC_DSP_DELAY_PARAM_WET,
        SC_DSP_DELAY_PARAM_FEEDBACK,
        SC_DSP_DELAY_PARAM_COUNT
    };

    typedef enum sc_dsp_meter_query
    {
        SC_DSP_METER_QUERY_PEAK,
        SC_DSP_METER_QUERY_RMS,
        SC_DSP_METER_QUERY_COUNT
    } sc_dsp_meter_query;

    enum
    {
        SC_DSP_METER_MAX_CHANNELS = SC_MAX_CHANNELS
    };

    typedef struct sc_meter
    {
        sc_atomic_uint32 channels;
        sc_atomic_float peakLevels[SC_DSP_METER_MAX_CHANNELS];
        sc_atomic_float rmsLevels[SC_DSP_METER_MAX_CHANNELS];
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

    typedef struct
    {
        ma_uint32 channels;
        ma_uint32 sampleRate;
        ma_uint32 delayInFrames;
        ma_uint32 maxDelayInFrames;
        float dry;      //< Dry signal gain (0 - 1). Defaults to 1
        float wet;      //< Wet signal gain (0 - 1). Defaults to 0 (no delay)
        float feedback; //< Feedback signal gain (0 - 1). Defaults to 0 (no feedback)
    } sc_delay_config;

    sc_delay_config SC_API sc_delay_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames);

    typedef struct
    {
        sc_delay_config config;
        ma_uint32 writeCursor;
        ma_uint32 bufferSizeInFrames;   //< Total buffer size. Not the delay time/size
        float* buffer;
        ma_uint32 silentFrameCount;     //< Audio thread. Counts number of silent frames so we know when are idle
        sc_atomic_uint32 isIdle;
    } sc_delay;

    sbk_status  SC_API sc_delay_init(const sc_delay_config* config, const ma_allocation_callbacks* allocationCallbacks, sc_delay* delay);
    void        SC_API sc_delay_uninit(sc_delay* delay, const ma_allocation_callbacks* allocationCallbacks);
    sbk_status  SC_API sc_delay_process_pcm_frames(sc_delay* delay, void* framesOut, const void* framesIn, ma_uint32 frameCount);
    sbk_status  SC_API sc_delay_set_delay_ms(sc_delay* delay, float value);
    sbk_status  SC_API sc_delay_get_delay_ms(const sc_delay* delay, float* outValue);
    sbk_status  SC_API sc_delay_set_wet(sc_delay* delay, float value);
    sbk_status  SC_API sc_delay_get_wet(const sc_delay* delay, float* outValue);
    sbk_status  SC_API sc_delay_set_dry(sc_delay* delay, float value);
    sbk_status  SC_API sc_delay_get_dry(const sc_delay* delay, float* outValue);
    sbk_status  SC_API sc_delay_set_feedback(sc_delay* delay, float value);
    sbk_status  SC_API sc_delay_get_feedback(const sc_delay* delay, float* outValue);

    typedef struct
    {
        ma_node_config nodeConfig;
        sc_delay_config delayConfig;
    } sc_delay_node_config;

    sc_delay_node_config SC_API sc_delay_node_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames);

    typedef struct
    {
        ma_node_base baseNode;
        sc_delay delay;
    } sc_delay_node;

    sbk_status  SC_API sc_delay_node_init(ma_node_graph* pNodeGraph, const sc_delay_node_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, sc_delay_node* pDelayNode);
    void        SC_API sc_delay_node_uninit(sc_delay_node* pDelayNode, const ma_allocation_callbacks* pAllocationCallbacks);

#ifdef __cplusplus
}
#endif

#endif