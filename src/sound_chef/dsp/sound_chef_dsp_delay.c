#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

sc_delay_config sc_delay_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames)
{
    sc_delay_config config;
    SC_ZERO_OBJECT(&config);

    config.channels         = channels;
    config.sampleRate       = sampleRate;
    config.delayInFrames    = 1U;
    config.maxDelayInFrames = maxDelayInFrames;
    config.dry              = 1.0F;
    config.wet              = 0.0F;
    config.feedback         = 0.0F;

    return config;
}

sbk_status sc_delay_init(const sc_delay_config* config, const ma_allocation_callbacks* allocationCallbacks, sc_delay* delay)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->feedback >= 0.0F);
    SC_CHECK_ARG(config->feedback <= 1.0F);
    SC_CHECK_ARG(config->maxDelayInFrames > 1U);
    SC_CHECK_ARG(delay != NULL);

    SC_ZERO_OBJECT(delay);

    delay->config               = *config;
    delay->bufferSizeInFrames   = config->maxDelayInFrames;
    delay->writeCursor          = 0;

    delay->buffer = (float*)ma_malloc((size_t)delay->bufferSizeInFrames * ma_get_bytes_per_frame(ma_format_f32, config->channels), allocationCallbacks);
    SC_CHECK_MEM(delay->buffer);

    ma_silence_pcm_frames(delay->buffer, delay->bufferSizeInFrames, ma_format_f32, config->channels);

    return MA_SUCCESS;
}

void sc_delay_uninit(sc_delay* delay, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_free(delay->buffer, allocationCallbacks);
    delay->buffer = NULL;
}

sbk_status sc_delay_process_pcm_frames(sc_delay* delay, void* framesOut, const void* framesIn, ma_uint32 frameCount)
{
    SC_CHECK_ARG(delay != NULL);
    SC_CHECK_ARG(delay->buffer != NULL);
    SC_CHECK_ARG(framesOut != NULL);
    SC_CHECK_ARG(framesIn != NULL);
    SC_CHECK_ARG(frameCount > 0);

    float* framesOutF32      = (float*)framesOut;
    const float* framesInF32 = (const float*)framesIn;

    ma_uint32 writePos = delay->writeCursor;
    ma_uint32 readPos  = (writePos + delay->bufferSizeInFrames - delay->config.delayInFrames) % delay->bufferSizeInFrames;

    float blockPeak = 0.0F;

    for (ma_uint32 frame = 0; frame < frameCount; ++frame)
    {
        for (ma_uint32 channel = 0; channel < delay->config.channels; ++channel)
        {
            const float wetSample = delay->buffer[(readPos * delay->config.channels) + channel];
            const float drySample = framesInF32[channel];

            framesOutF32[channel] = (drySample * delay->config.dry) + (wetSample * delay->config.wet);
            
            float writeSample = drySample + (wetSample * delay->config.feedback);
            const float absWriteSample = SC_ABS(writeSample);

            if (absWriteSample < SC_DELAY_SILENCE_THRESHOLD)
            {
                writeSample = 0.0F; /* also kills denormals */
            }
            else if (absWriteSample > blockPeak)
            {
                blockPeak = absWriteSample;
            }

            delay->buffer[(writePos * delay->config.channels) + channel] = writeSample;
        }

        if (++readPos >= delay->bufferSizeInFrames)
        {
            readPos = 0;
        }

        if (++writePos >= delay->bufferSizeInFrames)
        {
            writePos = 0;
        }

        framesOutF32 += delay->config.channels;
        framesInF32 += delay->config.channels;
    }

    delay->writeCursor = writePos;

    if (blockPeak > 0.0F)
    {
        delay->silentFrameCount = 0;

        atomic_store_explicit(&delay->isIdle, MA_FALSE, memory_order_relaxed);
    }
    else
    {
        delay->silentFrameCount += frameCount;

        if (delay->silentFrameCount >= delay->config.delayInFrames)
        {
            atomic_store_explicit(&delay->isIdle, MA_TRUE, memory_order_relaxed);
        }
    }

    return SBK_SUCCESS;
}

sbk_status sc_delay_set_delay_ms(sc_delay* delay, float value)
{
    SC_CHECK_ARG(delay != NULL);
    SC_CHECK_ARG(value >= 0.0F);

    const ma_uint32 potentialDelayInFrames = ma_calculate_buffer_size_in_frames_from_milliseconds((ma_uint32)value, delay->config.sampleRate);
    SC_CHECK(potentialDelayInFrames < delay->bufferSizeInFrames, SBK_ERR_TOO_LARGE);

    delay->config.delayInFrames = potentialDelayInFrames;
    return SBK_SUCCESS;
}

sbk_status sc_delay_get_delay_ms(const sc_delay* delay, float* outValue)
{
    SC_CHECK_ARG(delay != NULL);
    SC_CHECK_ARG(outValue != NULL);
    *outValue = (float)ma_calculate_buffer_size_in_milliseconds_from_frames(delay->config.delayInFrames, delay->config.sampleRate);
    return SBK_SUCCESS;
}

sbk_status sc_delay_set_wet(sc_delay* delay, float value)
{
    SC_CHECK_ARG(delay != NULL);
    SC_CHECK_ARG(value >= 0.0F);
    SC_CHECK_ARG(value <= 1.0F);
    delay->config.wet = value;
    return SBK_SUCCESS;
}

sbk_status sc_delay_get_wet(const sc_delay* delay, float* outValue)
{
    SC_CHECK_ARG(delay != NULL);
    *outValue = delay->config.wet;
    return SBK_SUCCESS;
}

sbk_status sc_delay_set_dry(sc_delay* delay, float value)
{
    SC_CHECK_ARG(delay != NULL);
    SC_CHECK_ARG(value >= 0.0F);
    SC_CHECK_ARG(value <= 1.0F);
    delay->config.dry = value;
    return SBK_SUCCESS;
}

sbk_status sc_delay_get_dry(const sc_delay* delay, float* outValue)
{
    SC_CHECK_ARG(delay != NULL);
    *outValue = delay->config.dry;
    return SBK_SUCCESS;
}

sbk_status sc_delay_set_feedback(sc_delay* delay, float value)
{
    SC_CHECK_ARG(delay != NULL);
    SC_CHECK_ARG(value >= 0.0F);
    SC_CHECK_ARG(value <= 1.0F);
    delay->config.feedback = value;
    return SBK_SUCCESS;
}

sbk_status sc_delay_get_feedback(const sc_delay* delay, float* outValue)
{
    SC_CHECK_ARG(delay != NULL);
    *outValue = delay->config.feedback;
    return SBK_SUCCESS;
}

// NODE

sc_delay_node_config sc_delay_node_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames)
{
    sc_delay_node_config config;
    SC_ZERO_OBJECT(&config);

    config.nodeConfig  = ma_node_config_init();
    config.delayConfig = sc_delay_config_init(channels, sampleRate, maxDelayInFrames);

    return config;
}

static void sc_delay_node_process_pcm_frames(ma_node* node, const float** framesIn, ma_uint32* frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{
    (void)frameCountIn;

    sc_delay_node* delayNode = (sc_delay_node*)node;
    if (sc_delay_process_pcm_frames(&delayNode->delay, framesOut[0], framesIn[0], *frameCountOut) != SBK_SUCCESS)
    {
        ma_silence_pcm_frames(framesOut, *frameCountOut, ma_format_f32, delayNode->delay.config.channels);
    }
}

static ma_node_vtable s_delayNodeVTable =
{
    sc_delay_node_process_pcm_frames,
    NULL,
    1,                                 /* 1 input channels. */
    1,                                 /* 1 output channel. */
    MA_NODE_FLAG_CONTINUOUS_PROCESSING /* Delay requires continuous processing to ensure the tail get's processed. */
};

sbk_status sc_delay_node_init(ma_node_graph* nodeGraph, const sc_delay_node_config* config, const ma_allocation_callbacks* allocationCallbacks, sc_delay_node* delayNode)
{
    SC_CHECK_ARG(delayNode != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_ZERO_OBJECT(delayNode);

    sbk_status result;

    result = sc_delay_init(&config->delayConfig, allocationCallbacks, &delayNode->delay);
    SC_CHECK_STATUS(result);

    ma_node_config baseConfig  = config->nodeConfig;
    baseConfig.vtable          = &s_delayNodeVTable;
    baseConfig.pInputChannels  = &config->delayConfig.channels;
    baseConfig.pOutputChannels = &config->delayConfig.channels;

    result = SBK_FROM_MA(ma_node_init(nodeGraph, &baseConfig, allocationCallbacks, &delayNode->baseNode));
    if (result != SBK_SUCCESS)
    {
        sc_delay_uninit(&delayNode->delay, allocationCallbacks);
    }

    return result;
}

void sc_delay_node_uninit(sc_delay_node* delayNode, const ma_allocation_callbacks* allocationCallbacks)
{
    if (delayNode == NULL || delayNode->baseNode.pNodeGraph == NULL)
    {
        return;
    }

    /* The base node is always uninitialized first. */
    ma_node_uninit(&delayNode->baseNode, allocationCallbacks);
    sc_delay_uninit(&delayNode->delay, allocationCallbacks);
}

// DSP CALLBACKS

static sbk_status sc_dsp_delay_create(sc_dsp_state* state)
{
    SC_CHECK_ARG(state != NULL);

    ma_node_graph* const nodeGraph                           = (ma_node_graph*)state->system;
    const sc_system* const system                            = (sc_system*)state->system;
    const ma_engine* const engine                            = (const ma_engine*)system;
    const ma_allocation_callbacks* const allocationCallbacks = &engine->allocationCallbacks;

    SC_CREATE(state->userData, sc_delay_node, system);

    const ma_uint32 tenSecondDelay = ma_engine_get_sample_rate(engine) * 10U;

    sc_delay_node_config config = sc_delay_node_config_init(ma_engine_get_channels(engine), ma_engine_get_sample_rate(engine), tenSecondDelay);
    return sc_delay_node_init(nodeGraph, &config, allocationCallbacks, (sc_delay_node*)state->userData);
}

static sbk_status sc_dsp_delay_release(sc_dsp_state* state)
{
    SC_CHECK_ARG(state != NULL);
    SC_CHECK_ARG(state->system != NULL);

    const sc_system* const system                            = (sc_system*)state->system;
    const ma_engine* const engine                            = (const ma_engine*)system;
    const ma_allocation_callbacks* const allocationCallbacks = &engine->allocationCallbacks;

    sc_delay_node_uninit((sc_delay_node*)state->userData, allocationCallbacks);
    SC_FREE(state->userData, system);
    return SBK_SUCCESS;
}

static sbk_status sc_dsp_delay_is_idle(sc_dsp_state* state, sc_bool* outIsIdle)
{
    SC_CHECK_ARG(state != NULL);
    SC_CHECK_ARG(outIsIdle != NULL);
    SC_CHECK_ARG(state->userData != NULL);

    sc_delay_node* delayNode = (sc_delay_node*)state->userData;
    *outIsIdle               = (sc_bool)atomic_load_explicit(&delayNode->delay.isIdle, memory_order_relaxed);
    return SBK_SUCCESS;
}

static sbk_status sc_dsp_delay_set_param_float(sc_dsp_state* state, int index, float value)
{
    SC_CHECK_ARG(state != NULL);
    SC_CHECK_ARG(state->userData != NULL);
    SC_CHECK_ARG(index >= 0);

    sc_delay_node* const delayNode = (sc_delay_node*)state->userData;
    sc_delay* const delay          = &delayNode->delay;

    switch (index)
    {
        default:
            break;
        case SC_DSP_DELAY_PARAM_DELAY_SECONDS:
        {
            return sc_delay_set_delay_ms(delay, value);
        }
        break;
        case SC_DSP_DELAY_PARAM_DRY:
        {
            return sc_delay_set_dry(delay, value);
        }
        break;
        case SC_DSP_DELAY_PARAM_WET:
        {
            return sc_delay_set_wet(delay, value);
        }
        break;
        case SC_DSP_DELAY_PARAM_FEEDBACK:
        {
            return sc_delay_set_feedback(delay, value);
        }
        break;
    }

    return SBK_ERR_INVALID_OPERATION;
}

static sbk_status sc_dsp_delay_get_param_float(sc_dsp_state* state, int index, float* const value)
{
    SC_CHECK_ARG(state != NULL);
    SC_CHECK_ARG(state->userData != NULL);
    SC_CHECK_ARG(index >= 0);
    SC_CHECK_ARG(value != NULL);

    sc_delay_node* const delayNode = (sc_delay_node*)state->userData;
    sc_delay* const delay          = &delayNode->delay;

    switch (index)
    {
        default:
            break;
        case SC_DSP_DELAY_PARAM_DELAY_SECONDS:
        {
            return sc_delay_get_delay_ms(delay, value);
        }
        break;
        case SC_DSP_DELAY_PARAM_DRY:
        {
            return sc_delay_get_dry(delay, value);
        }
        break;
        case SC_DSP_DELAY_PARAM_WET:
        {
            return sc_delay_get_wet(delay, value);
        }
        break;
        case SC_DSP_DELAY_PARAM_FEEDBACK:
        {
            return sc_delay_get_feedback(delay, value);
        }
        break;
    }

    return SBK_ERR_INVALID_OPERATION;
}

static sc_dsp_parameter s_delayDelay    = {sc_dsp_parameter_type_float, "Delay", 0.0F, 10000.0F, 1.0F};
static sc_dsp_parameter s_delayDry      = {sc_dsp_parameter_type_float, "Dry", 0.0F, 1.0F, 1.0F};
static sc_dsp_parameter s_delayWet      = {sc_dsp_parameter_type_float, "Wet", 0.0F, 1.0F, 0.5F};
static sc_dsp_parameter s_delayFeedback = {sc_dsp_parameter_type_float, "Feedback", 0.0F, 1.0F, 0.0F};

static sc_dsp_parameter* s_delayParams[SC_DSP_DELAY_PARAM_COUNT] = {&s_delayDelay, &s_delayDry, &s_delayWet, &s_delayFeedback};

sc_dsp_vtable g_dspDelayVTable = 
{
    sc_dsp_delay_create, 
    sc_dsp_delay_release, 
    sc_dsp_delay_is_idle, 
    sc_dsp_delay_set_param_float, 
    sc_dsp_delay_get_param_float, 
    s_delayParams, 
    SC_DSP_DELAY_PARAM_COUNT
};