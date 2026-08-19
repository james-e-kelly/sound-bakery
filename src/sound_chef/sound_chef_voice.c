#include "sound_chef/sound_chef.h"

_Static_assert(sizeof(sc_voice) == 64, "sc_voice must fit in a 64-byte cache line");

static sbk_status sc_voice_resolve(sc_system* system, sc_voice_handle handle, sc_voice** outVoice)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outVoice != NULL);

    const sc_voice_index slot = SC_VOICE_HANDLE_EXTRACT_INDEX(handle);
    SC_CHECK_ARG(slot < system->voiceSlotAllocator.capacity);

    sc_voice* const voice         = &system->voiceBuffer[slot];
    const sc_voice_handle current = (sc_voice_handle)c89atomic_load_64(&voice->handle);
    SC_CHECK(current == handle, SBK_ERR_NOT_FOUND);

    *outVoice = voice;
    return SBK_SUCCESS;
}

static void sc_voice_write_flag(volatile sc_atomic_uint32* flags, sc_voice_flags flag, sc_bool add)
{
    for (;;)
    {
        const sc_uint32 old     = c89atomic_load_32(flags);
        const sc_uint32 desired = add ? (old | (sc_uint32)flag) : (old & ~(sc_uint32)flag);

        if (old == desired)
        {
            return;
        }

        if (c89atomic_compare_and_swap_32(flags, old, desired) == old)
        {
            return;
        }
    }
}

sbk_status sc_voice_set_paused(sc_system* system, sc_voice_handle handle, sc_bool paused)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_PAUSED, paused);
    return SBK_SUCCESS;
}

sbk_status SC_API sc_voice_set_virtual(sc_voice* voice, sc_bool virtualised)
{
    SC_CHECK_ARG(voice != NULL);
    sc_voice_write_flag(&voice->flags, SC_VOICE_FLAG_VIRTUAL, virtualised);
    return SBK_SUCCESS;
}

sbk_status sc_voice_stop(sc_system* system, sc_voice_handle handle)
{
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    c89atomic_store_32(&voice->desiredState, (sc_uint32)sc_voice_desired_stopped);
    return SBK_SUCCESS;
}

sbk_status sc_system_stop_all_voices(sc_system* system)
{
    SC_CHECK_ARG(system != NULL);
    for (sc_uint32 slot = 0; slot < system->voiceSlotAllocator.capacity; ++slot)
    {
        c89atomic_store_32(&system->voiceBuffer[slot].desiredState, (sc_uint32)sc_voice_desired_stopped);
    }
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_paused(sc_system* system, sc_voice_handle handle, sc_bool* outPaused)
{
    SC_CHECK_ARG(outPaused != NULL);
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    *outPaused = SC_VOICE_HAS_FLAG(c89atomic_load_32(&voice->flags), SC_VOICE_FLAG_PAUSED);
    return SBK_SUCCESS;
}

sbk_status sc_voice_get_virtual(sc_system* system, sc_voice_handle handle, sc_bool* outVirtual)
{
    SC_CHECK_ARG(outVirtual != NULL);
    sc_voice* voice = NULL;
    SC_CHECK_STATUS(sc_voice_resolve(system, handle, &voice));
    *outVirtual = SC_VOICE_HAS_FLAG(c89atomic_load_32(&voice->flags), SC_VOICE_FLAG_VIRTUAL);
    return SBK_SUCCESS;
}

static void sc_voice_real_process_pcm_frames(ma_node* node, const float** framesIn, ma_uint32* frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{

}

static ma_node_vtable s_realVoiceVtable =
{
    sc_voice_real_process_pcm_frames,
    NULL,
    1,  // One input                               
    1,  // One output. Maybe more in the future for aux busses?
    0   // Default
};

typedef struct
{
    size_t sizeInBytes;
    size_t baseNodeOffset;
    size_t resamplerOffset;
    size_t gainerOffset;
} sc_real_voice_heap_layout;

static sbk_status sc_real_voice_get_heap_layout(const sc_real_voice_config* config, sc_real_voice_heap_layout* heapLayout)
{
    SC_ASSERT(heapLayout);
    SC_ZERO_OBJECT(heapLayout);

    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->system != NULL);

    size_t tempHeapSize;
    ma_uint32 channelsIn;
    ma_uint32 channelsOut;
    ma_channel defaultStereoChannelMap[2] = {MA_CHANNEL_SIDE_LEFT, MA_CHANNEL_SIDE_RIGHT}; /* <-- Consistent with the default channel map of a stereo listener. Means channel conversion can run on a fast path. */

    heapLayout->sizeInBytes = 0;

    channelsIn  = (config->channelsIn != 0) ? config->channelsIn : ma_engine_get_channels(&config->system->engine);
    channelsOut = (config->channelsOut != 0) ? config->channelsOut : ma_engine_get_channels(&config->system->engine);

    ma_node_config nodeConfig  = ma_node_config_init();
    nodeConfig.vtable          = &s_realVoiceVtable;
    nodeConfig.pInputChannels  = &channelsIn;
    nodeConfig.pOutputChannels = &channelsOut;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_node_get_heap_size(&config->system->engine.nodeGraph, &nodeConfig, &tempHeapSize)));

    heapLayout->baseNodeOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_linear_resampler_config resamplerConfig;
    resamplerConfig          = ma_linear_resampler_config_init(ma_format_f32, channelsIn, 1, 1);
    resamplerConfig.lpfOrder = 0;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_linear_resampler_get_heap_size(&resamplerConfig, &tempHeapSize)));

    heapLayout->resamplerOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    ma_gainer_config gainerConfig = ma_gainer_config_init(channelsIn, 1);

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_gainer_get_heap_size(&gainerConfig, &tempHeapSize)));
    
    heapLayout->gainerOffset = heapLayout->sizeInBytes;
    heapLayout->sizeInBytes += SC_ALIGN_64(tempHeapSize);

    return MA_SUCCESS;
}

static sbk_status sc_real_voice_get_heap_size(const sc_real_voice_config* config, size_t* heapSizeInBytes)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(heapSizeInBytes != NULL);

    sc_real_voice_heap_layout heapLayout;

    *heapSizeInBytes = 0;

    SC_CHECK_STATUS(sc_real_voice_get_heap_layout(config, &heapLayout));

    *heapSizeInBytes = heapLayout.sizeInBytes;

    return MA_SUCCESS;
}

sbk_status sc_real_voice_init(const sc_real_voice_config* config, sc_real_voice* realVoice)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->system != NULL);
    SC_CHECK_ARG(realVoice != NULL);

    size_t heapSizeInBytes = 0;
    void* heap             = NULL;

    SC_CHECK_STATUS(sc_real_voice_get_heap_size(config, &heapSizeInBytes));

    if (heapSizeInBytes > 0)
    {
        SC_MALLOC(heap, heapSizeInBytes, config->system);
    }

    SC_CHECK_STATUS_ELSE_FREE(sc_real_voice_init_preallocated(config, heap, realVoice), heap, config->system);

    realVoice->ownsHeap = SC_TRUE;
    return SBK_SUCCESS;
}

sbk_status sc_real_voice_init_preallocated(const sc_real_voice_config* config, void* heap, sc_real_voice* realVoice)
{
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->voiceRef != NULL);
    SC_CHECK_ARG(realVoice != NULL);

    MA_ZERO_OBJECT(realVoice);

    realVoice->system   = config->system;
    realVoice->voiceRef = config->voiceRef;

    sc_real_voice_heap_layout heapLayout;
    SC_CHECK_STATUS(sc_real_voice_get_heap_layout(config, &heapLayout));

    realVoice->heap = heap;
    SC_ZERO_MEMORY(heap, heapLayout.sizeInBytes);

    const ma_uint32 channelsIn = (config->channelsIn != 0) ? config->channelsIn : ma_engine_get_channels(&config->system->engine);
    const ma_uint32 channelsOut = (config->channelsOut != 0) ? config->channelsOut : ma_engine_get_channels(&config->system->engine);

    ma_node_config baseNodeConfig  = ma_node_config_init();
    baseNodeConfig.vtable          = &s_realVoiceVtable;
    baseNodeConfig.pInputChannels  = &channelsIn;
    baseNodeConfig.pOutputChannels = &channelsOut;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_node_init_preallocated(&config->system->engine.nodeGraph, &baseNodeConfig, SC_OFFSET_PTR(heap, heapLayout.baseNodeOffset), &realVoice->baseNode)));

    ma_linear_resampler_config resamplerConfig = ma_linear_resampler_config_init(ma_format_f32, channelsIn, 0 /* Set actual sample rate*/, ma_engine_get_sample_rate(&config->system->engine));
    resamplerConfig.lpfOrder                   = 0;

    SC_CHECK_STATUS_AND_GOTO(SC_STATUS_FROM_MA_RESULT(ma_linear_resampler_init_preallocated(&resamplerConfig, SC_OFFSET_PTR(heap, heapLayout.resamplerOffset), &realVoice->resampler)), error1);

    ma_fader_config faderConfig = ma_fader_config_init(ma_format_f32, channelsIn, 0);

    SC_CHECK_STATUS_AND_GOTO(SC_STATUS_FROM_MA_RESULT(ma_fader_init(&faderConfig, &realVoice->fader)), error2);

    ma_gainer_config gainerConfig = ma_gainer_config_init(channelsIn, 1);

    SC_CHECK_STATUS_AND_GOTO(SC_STATUS_FROM_MA_RESULT(ma_gainer_init_preallocated(&gainerConfig, SC_OFFSET_PTR(heap, heapLayout.gainerOffset), &realVoice->gainer)), error2);

    return MA_SUCCESS;

    /* No need for allocation callbacks here because we use a preallocated heap. */
error2: ma_linear_resampler_uninit(&realVoice->resampler, NULL);
error1: ma_node_uninit(&realVoice->baseNode, NULL);
    return SBK_ERR_CHEF;
}

sc_real_voice_config sc_real_voice_config_init(sc_system* system, sc_voice* voiceRef, sc_uint32 channelsIn, sc_uint32 channelsOut)
{
    sc_real_voice_config config;
    SC_ZERO_OBJECT(&config);
    config.system = system;
    config.voiceRef = voiceRef;
    config.channelsIn = channelsIn;
    config.channelsOut = channelsOut;
    return config;
}

sbk_status sc_system_promote_voice_to_real(sc_system* system, sc_voice* voice)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(voice != NULL);

    sc_voice_handle realVoiceSlot       = 0;
    const ma_result realVoiceSlotResult = ma_slot_allocator_alloc(&system->realVoiceSlotAllocator, &realVoiceSlot);

    if (realVoiceSlotResult == MA_SUCCESS)
    {
        voice->realVoiceHandle = realVoiceSlot;

        const sc_voice_index realVoiceIndex = SC_VOICE_HANDLE_EXTRACT_INDEX(realVoiceSlot);
        sc_real_voice* const realVoice      = &system->realVoiceBuffer[realVoiceIndex];

        realVoice->voiceRef = voice;

        // Node groups are created once and then kept around, being deallocated at system close

        if (realVoice->nodeGroup != NULL)
        {
            sc_node_group_uninit(realVoice->nodeGroup);
        }
        else
        {
            SC_CREATE(realVoice->nodeGroup, sc_node_group, system);
        }

        // Create any and all DSP needed

        SC_CHECK_STATUS(sc_node_group_init(system, realVoice->nodeGroup));

        const sc_real_voice_config voiceConfig = sc_real_voice_config_init(system, voice, 2, 2);

        SC_CHECK_STATUS(sc_real_voice_init(&voiceConfig, realVoice));

        SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_node_attach_output_bus(&realVoice->baseNode, 0, realVoice->nodeGroup->tail, 0)));
    }
    else
    {
        // If we couldn't get a real voice, we must be virtual
        sc_voice_set_virtual(voice, SC_TRUE);
        voice->realVoiceHandle = 0;
    }

    return SBK_SUCCESS;
}