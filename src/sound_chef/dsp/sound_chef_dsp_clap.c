#include "sound_chef/sound_chef_internal.h"
#include "sound_chef/sound_chef_dsp.h"

#define SC_CLAP_INPUT_BUS  1
#define SC_CLAP_OUTPUT_BUS 1

static ma_uint32 sc_clap_input_events_size(const clap_input_events_t* list)
{
    (void)list;
    return 0;
}

static const clap_event_header_t* sc_clap_input_events_get(const clap_input_events_t* list, ma_uint32 index)
{
    (void)list;
    (void)index;
    return NULL;
}

static bool sc_clap_output_events_try_push(const clap_output_events_t* list, const clap_event_header_t* event)
{
    (void)list;
    (void)event;
    return true;
}

static void sc_clap_node_process_pcm_frames(ma_node* node, const float** framesIn, ma_uint32* const frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{
    sc_clap_node* const clapNode          = (sc_clap_node*)node;
    sc_system* const system               = (sc_system*)clapNode->baseNode.pNodeGraph;
    const clap_plugin_t* const clapPlugin = clapNode->clapPlugin;

    const ma_uint32 inputChannels  = ma_node_get_input_channels(node, 0);
    const ma_uint32 outputChannels = ma_node_get_output_channels(node, 0);

    assert(inputChannels == outputChannels);
    assert(*frameCountIn == *frameCountOut);

    if (*frameCountIn > SC_MAX_FRAME_COUNT || inputChannels > SC_MAX_CHANNELS)
    {
        ma_silence_pcm_frames(framesOut[0], *frameCountOut, ma_format_f32, outputChannels);
        return;
    }

    // CLAP expects start_processing() to bracket a run of process() calls; the matching stop_processing() is
    // issued when the node is released (sc_dsp_clap_release).
    if (!clapNode->isProcessing)
    {
        if (!clapPlugin->start_processing(clapPlugin))
        {
            ma_silence_pcm_frames(framesOut[0], *frameCountOut, ma_format_f32, outputChannels);
            return;
        }
        clapNode->isProcessing = MA_TRUE;
    }

    clap_process_t process;
    clap_audio_buffer_t inputBuffer;
    clap_audio_buffer_t outputBuffer;
    clap_input_events_t inputEvents;
    clap_output_events_t outputEvents;
    SC_ZERO_OBJECT(&process);
    SC_ZERO_OBJECT(&inputBuffer);
    SC_ZERO_OBJECT(&outputBuffer);
    SC_ZERO_OBJECT(&inputEvents);
    SC_ZERO_OBJECT(&outputEvents);

    ma_deinterleave_pcm_frames(ma_format_f32, inputChannels, *frameCountIn, framesIn[0], (void**)system->clapPluginChannels);

    inputBuffer.data32        = system->clapPluginChannels;
    inputBuffer.channel_count = inputChannels;

    outputBuffer.data32        = system->clapPluginChannels;
    outputBuffer.channel_count = outputChannels;

    inputEvents.size = sc_clap_input_events_size;
    inputEvents.get  = sc_clap_input_events_get;

    outputEvents.try_push = sc_clap_output_events_try_push;

    process.steady_time         = ma_node_graph_get_time(clapNode->baseNode.pNodeGraph);
    process.frames_count        = *frameCountIn;
    process.audio_inputs_count  = SC_CLAP_INPUT_BUS;
    process.audio_outputs_count = SC_CLAP_OUTPUT_BUS;
    process.audio_inputs        = &inputBuffer;
    process.audio_outputs       = &outputBuffer;
    process.in_events           = &inputEvents;
    process.out_events          = &outputEvents;

    const clap_process_status status = clapPlugin->process(clapPlugin, &process);

    if (status == CLAP_PROCESS_ERROR)
    {
        ma_silence_pcm_frames(framesOut[0], *frameCountOut, ma_format_f32, outputChannels);
    }
    else
    {
        ma_interleave_pcm_frames(ma_format_f32, outputChannels, *frameCountOut, (const void**)system->clapPluginChannels, framesOut[0]);
    }
}

static ma_node_vtable sc_clap_node_vtable = 
{
    sc_clap_node_process_pcm_frames, 
    NULL, 
    SC_CLAP_INPUT_BUS,
    SC_CLAP_OUTPUT_BUS, 
    0
};

static sbk_status sc_clap_node_init(ma_node_graph* nodeGraph,
                                    const ma_allocation_callbacks* allocCallbacks,
                                    sc_clap_node* node)
{
    SC_CHECK_ARG(nodeGraph != NULL);
    SC_CHECK_ARG(node != NULL);
    SC_CHECK_ARG(node->clapPlugin != NULL);

    ma_uint32 channels = 2;

    ma_node_config baseNodeConfig  = ma_node_config_init();
    baseNodeConfig.vtable          = &sc_clap_node_vtable;
    baseNodeConfig.pInputChannels  = &channels;
    baseNodeConfig.pOutputChannels = &channels;

    return SBK_FROM_MA(ma_node_init(nodeGraph, &baseNodeConfig, allocCallbacks, node));
}

static void sc_clap_node_uninit(sc_clap_node* node, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_node_uninit(node, allocationCallbacks);
}

static sbk_status sc_dsp_clap_create(sc_system* system, sc_dsp* dsp, void* userData)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(userData != NULL);

    SC_CREATE(dsp->node, sc_clap_node, system);

    sc_clap_node* const clapNode                   = (sc_clap_node*)dsp->node;
    const clap_plugin_factory_t* const clapFactory = (const clap_plugin_factory_t*)userData;

    const clap_plugin_descriptor_t* const clapDescriptor = clapFactory->get_plugin_descriptor(clapFactory, 0);
    SC_CHECK(clapDescriptor != NULL, SBK_FROM_MA(MA_ERROR));

    const clap_plugin_t* clapPlugin = clapFactory->create_plugin(clapFactory, &system->clapHost, clapDescriptor->id);
    SC_CHECK(clapPlugin != NULL, SBK_FROM_MA(MA_ERROR));

    if (!clapPlugin->init(clapPlugin))
    {
        clapPlugin->destroy(clapPlugin);
        return SBK_FROM_MA(MA_ERROR);
    }

    if (!clapPlugin->activate(clapPlugin, ma_engine_get_sample_rate((ma_engine*)system), 1, SC_MAX_FRAME_COUNT))
    {
        clapPlugin->destroy(clapPlugin);
        return SBK_FROM_MA(MA_ERROR);
    }

    clapNode->clapPlugin = clapPlugin;

    return sc_clap_node_init((ma_node_graph*)system, &system->engine.allocationCallbacks, clapNode);
}

static sbk_status sc_dsp_clap_release(sc_system* system, sc_dsp* dsp)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->node != NULL);

    sc_clap_node* const clapNode          = (sc_clap_node*)dsp->node;
    const clap_plugin_t* const clapPlugin = clapNode->clapPlugin;

    clapPlugin->stop_processing(clapPlugin);
    clapPlugin->deactivate(clapPlugin);
    clapPlugin->destroy(clapPlugin);
    clapNode->clapPlugin = NULL;

    sc_clap_node_uninit(clapNode, &system->engine.allocationCallbacks);
    SC_FREE(dsp->node, system);

    return SBK_SUCCESS;
}

static sbk_status sc_dsp_clap_set_param_float(sc_dsp* dsp, sc_uint32 index, float value)
{
    (void)dsp;
    (void)index;
    (void)value;
    return SBK_FROM_MA(MA_NOT_IMPLEMENTED);
}

static sbk_status sc_dsp_clap_get_param_float(sc_dsp* dsp, sc_uint32 index, float* value)
{
    (void)dsp;
    (void)index;
    (void)value;
    return SBK_FROM_MA(MA_NOT_IMPLEMENTED);
}

sc_dsp_description g_dspClapVTable =
{
    sc_dsp_clap_create,
    sc_dsp_clap_release,
    NULL,   // Idle
    sc_dsp_clap_set_param_float,
    sc_dsp_clap_get_param_float,
    NULL,
    0
};
