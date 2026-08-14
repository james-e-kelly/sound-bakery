#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef.h"
#include "sound_chef/sound_chef_dsp.h"

#include <math.h>

static void sc_meter_node_process_pcm_frames(ma_node* node, const float** framesIn, ma_uint32* const frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{
    // We don't output anything
    // The node is set to passthrough
    (void)framesOut;
    (void)frameCountOut;

    sc_meter* const meter = &((sc_meter_node*)node)->meter;

    const ma_uint32 inputChannels = ma_node_get_input_channels(node, 0);

    if (inputChannels == 0)
    {
        return;
    }

    const ma_uint32 inputFrames = *frameCountIn;

    const ma_uint32 minChannels = SC_MIN(inputChannels, SC_DSP_METER_MAX_CHANNELS);

    float channelSums[SC_DSP_METER_MAX_CHANNELS];
    memset(channelSums, 0, SC_DSP_METER_MAX_CHANNELS * sizeof(float));

    for (ma_uint32 sampleIndex = 0; sampleIndex < inputFrames; ++sampleIndex)
    {
        for (ma_uint32 channelIndex = 0; channelIndex < minChannels; ++channelIndex)
        {
            float sample = framesIn[0][channelIndex + (sampleIndex * inputChannels)];
            channelSums[channelIndex] += sample * sample;
        }
    }

    for (ma_uint32 channelIndex = 0; channelIndex < minChannels; ++channelIndex)
    {
        const float channelSum = channelSums[channelIndex];
        const float rms        = sqrtf(channelSum / (float)inputFrames);

        c89atomic_store_explicit_f32(&meter->rmsLevels[channelIndex], rms, c89atomic_memory_order_relaxed);
    }
}

static ma_node_vtable sc_meter_node_vtable = {sc_meter_node_process_pcm_frames, NULL, 1, 1, MA_NODE_FLAG_PASSTHROUGH};

static ma_uint32 s_meterChannels = 2;

static sbk_status sc_meter_node_init(ma_node_graph* nodeGraph,
                                     const ma_allocation_callbacks* allocCallbacks,
                                     sc_meter_node* node)
{
    ma_node_config baseNodeConfig  = ma_node_config_init();
    baseNodeConfig.vtable          = &sc_meter_node_vtable;
    baseNodeConfig.pInputChannels  = &s_meterChannels;
    baseNodeConfig.pOutputChannels = &s_meterChannels;

    return SBK_FROM_MA(ma_node_init(nodeGraph, &baseNodeConfig, allocCallbacks, node));
}
static void sc_meter_node_uninit(sc_meter_node* node, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_node_uninit(node, allocationCallbacks);
}

static sbk_status sc_dsp_meter_create(sc_system* system, sc_dsp* dsp, void* userData)
{
    SC_CREATE(dsp->node, sc_meter_node, system);

    return sc_meter_node_init((ma_node_graph*)system, &system->engine.allocationCallbacks, (sc_meter_node*)dsp->node);
}

static sbk_status sc_dsp_meter_release(sc_system* system, sc_dsp* dsp)
{
    sc_meter_node_uninit((sc_meter_node*)dsp->node, &system->engine.allocationCallbacks);
    SC_FREE(dsp->node, system);
    return SBK_SUCCESS;
}

sc_dsp_description g_dspMeterVTable =
{
    sc_dsp_meter_create,
    sc_dsp_meter_release
};

sbk_status sc_dsp_get_metering_info(sc_dsp* dsp, ma_uint32 channelIndex, sc_dsp_meter_query meterType, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->handle == SC_DSP_TYPE_METER);
    SC_CHECK_ARG(channelIndex <= SC_DSP_METER_MAX_CHANNELS);
    SC_CHECK_ARG(meterType >= 0);
    SC_CHECK_ARG(meterType < SC_DSP_METER_QUERY_COUNT);
    SC_CHECK_ARG(value != NULL);

    sc_meter_node* meterNode = (sc_meter_node*)dsp->node;
    SC_CHECK(meterNode != NULL, SBK_FROM_MA(MA_INVALID_DATA));

    switch (meterType)
    {
        case SC_DSP_METER_QUERY_PEAK:
            *value = c89atomic_load_explicit_f32(&meterNode->meter.peakLevels[channelIndex], c89atomic_memory_order_relaxed);
            break;
        case SC_DSP_METER_QUERY_RMS:
            *value = c89atomic_load_explicit_f32(&meterNode->meter.rmsLevels[channelIndex], c89atomic_memory_order_relaxed);
            break;
        default:
            break;
    }

    return SBK_SUCCESS;
}
