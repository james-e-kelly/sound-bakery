#define MINIAUDIO_IMPLEMENTATION

#include "sound_chef/sound_chef.h"

#include <dirent.h>
#include <stdio.h>

static void sc_system_clap_request_callback(const clap_host_t* host) { (void)host; }
static void sc_system_clap_request_process(const clap_host_t* host) { (void)host; }
static void sc_system_clap_request_restart(const clap_host_t* host) { (void)host; }

void sc_system_audio_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount)
{
    (void)input;

    sc_system* const system = (sc_system*)device->pUserData;

    for (sc_uint32 voiceIndex = 0; voiceIndex < system->voiceSlotAllocator.capacity; ++voiceIndex)
    {
        const sc_uint32 flags = c89atomic_load_32(&system->voiceBuffer[voiceIndex].flags);

        const sc_bool isPaused = SC_VOICE_HAS_FLAG(flags, SC_VOICE_FLAG_PAUSED);

        if (!isPaused)
        {
            c89atomic_fetch_add_64(&system->voiceBuffer[voiceIndex].playCursor, frameCount);
        }
    }

    (void)ma_engine_read_pcm_frames((ma_engine*)(device->pUserData), output, frameCount, NULL);
}

sbk_status sc_system_create(sc_system** outSystem)
{
    sbk_status result = SBK_ERR_CHEF;

    if (outSystem)
    {
        *outSystem = (sc_system*)ma_malloc(sizeof(sc_system), NULL);

        result = *outSystem ? SBK_SUCCESS : SBK_ERR_OUT_OF_MEMORY;

        if (*outSystem)
        {
            SC_ZERO_OBJECT(*outSystem);
        }
    }

    SC_ASSERT(result == SBK_SUCCESS);

    return result;
}

sbk_status sc_system_release(sc_system* system)
{
    sbk_status result = SBK_ERR_CHEF;

    if (system)
    {
        ma_free(system, NULL);
        result = SBK_SUCCESS;
    }

    SC_ASSERT(result == SBK_SUCCESS);

    return result;
}

sbk_status sc_system_log_init(sc_system* system, ma_log_callback_proc callbackProc)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(callbackProc != NULL);

    const sbk_status logInitResult = SC_STATUS_FROM_MA_RESULT(ma_log_init(NULL, &system->log));
    SC_CHECK_STATUS(logInitResult);

    const sbk_status registerResult = SC_STATUS_FROM_MA_RESULT(ma_log_register_callback(&system->log, ma_log_callback_init(callbackProc, NULL)));
    SC_CHECK_STATUS(registerResult);

    ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef Logging");

    return SBK_SUCCESS;
}

sc_system_config sc_system_config_init_default()
{
    sc_system_config config;
    SC_ZERO_OBJECT(&config);
    config.maxRealVoices = 128;
    config.maxVoices     = 2048;
    config.vol0Threshold = 0.001F;
    return config;
}

sc_system_config sc_system_config_init(const char* pluginPath)
{
    sc_system_config config = sc_system_config_init_default();
    config.pluginPath       = pluginPath;
    return config;
}

extern ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libvorbis;
extern ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libopus;

ma_decoding_backend_vtable* s_customBackendVTables[] = 
{
    &g_ma_decoding_backend_vtable_libvorbis,
    &g_ma_decoding_backend_vtable_libopus
};

sbk_status sc_system_init(sc_system* system, const sc_system_config* systemConfig)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(systemConfig != NULL);
    SC_CHECK_ARG(systemConfig->maxVoices > 0);
    SC_CHECK_ARG(systemConfig->maxVoices >= systemConfig->maxRealVoices);
    SC_CHECK_ARG(systemConfig->vol0Threshold > 0.0F && systemConfig->vol0Threshold < 1.0F);

    system->tiebreakerPolicy = systemConfig->tiebreakPolicy;

    ma_engine* engine = (ma_engine*)system;

    ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Initializing resource manager");

    ma_resource_manager_config resourceManagerConfig     = ma_resource_manager_config_init();
    resourceManagerConfig.ppCustomDecodingBackendVTables = s_customBackendVTables;
    resourceManagerConfig.customDecodingBackendCount     = SC_COUNTOF(s_customBackendVTables);
    resourceManagerConfig.pCustomDecodingBackendUserData = NULL;
    resourceManagerConfig.pLog                           = &system->log;
    resourceManagerConfig.allocationCallbacks            = systemConfig->allocationCallbacks;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_resource_manager_init(&resourceManagerConfig, &system->resourceManager)));

    const ma_slot_allocator_config voiceAllocatorConfig = ma_slot_allocator_config_init(systemConfig->maxVoices);
    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_slot_allocator_init(&voiceAllocatorConfig, &systemConfig->allocationCallbacks, &system->voiceSlotAllocator)));

    const ma_slot_allocator_config realVoiceAllocatorConfig = ma_slot_allocator_config_init(systemConfig->maxRealVoices);
    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_slot_allocator_init(&realVoiceAllocatorConfig, &systemConfig->allocationCallbacks, &system->realVoiceSlotAllocator)));

    system->voiceBuffer = ma_calloc(sizeof(sc_voice) * systemConfig->maxVoices, &systemConfig->allocationCallbacks);
    SC_CHECK_MEM(system->voiceBuffer);

    system->realVoiceBuffer = ma_calloc(sizeof(sc_real_voice) * systemConfig->maxRealVoices, &systemConfig->allocationCallbacks);
    SC_CHECK_MEM(system->realVoiceBuffer);

    system->virtualizeCandidates = ma_calloc(sizeof(sc_virtual_voice_candidate) * systemConfig->maxVoices, &systemConfig->allocationCallbacks);
    SC_CHECK_MEM(system->virtualizeCandidates);

    system->virtualizeBoundary = ma_calloc(sizeof(sc_virtual_voice_candidate) * systemConfig->maxVoices, &systemConfig->allocationCallbacks);
    SC_CHECK_MEM(system->virtualizeBoundary);
    
    ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Initializing engine");

    ma_engine_config engineConfig    = ma_engine_config_init();
    engineConfig.pResourceManager    = &system->resourceManager;
    engineConfig.listenerCount       = 1;
    engineConfig.channels            = 2;   /// @todo Make this dynamic. It's currently set to two so there are no surprises
    engineConfig.sampleRate          = ma_standard_sample_rate_48000;
    engineConfig.pLog                = &system->log;
    engineConfig.allocationCallbacks = systemConfig->allocationCallbacks;
    engineConfig.dataCallback        = systemConfig->dataCallback;

    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_engine_init(&engineConfig, engine)));
    
    ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Creating master node group");

    SC_CHECK_STATUS(sc_system_create_node_group(system, &system->masterNodeGroup));
    SC_CHECK_STATUS(sc_node_group_set_parent_endpoint(system->masterNodeGroup));

    ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Adding meter");

    const sc_dsp_config meterDspConfig = sc_dsp_config_init_type(system, sc_dsp_type_meter);
    sc_dsp* meterDSP = NULL;
    SC_CHECK_STATUS(sc_system_create_dsp(system, &meterDspConfig, &meterDSP));
    SC_CHECK_STATUS(sc_node_group_add_dsp(system->masterNodeGroup, meterDSP, sc_dsp_index_head));

    ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Initialized Master Node Group");

    if (systemConfig->pluginPath != NULL)
    {
        ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Initializing CLAP");

        system->clapHost.clap_version     = CLAP_VERSION;
        system->clapHost.host_data        = system;
        system->clapHost.name             = SC_PRODUCT_NAME;
        system->clapHost.version          = SC_VERSION_STRING;
        system->clapHost.url              = "https://github.com/james-e-kelly/sound-bakery";
        system->clapHost.request_callback = sc_system_clap_request_callback;
        system->clapHost.request_process  = sc_system_clap_request_process;
        system->clapHost.request_restart  = sc_system_clap_request_restart;

        for (ma_uint32 channel = 0; channel < SC_MAX_CHANNELS; ++channel)
        {
            system->clapPluginChannels[channel] = system->clapPluginScratch[channel];
        }

        DIR* const pluginDirectory = opendir(systemConfig->pluginPath);

        if (pluginDirectory != NULL)
        {
            /* First pass: count .clap files so we can allocate exactly once. */
            ma_uint32 clapCandidateCount = 0;
            for (struct dirent* entry = readdir(pluginDirectory); entry != NULL; entry = readdir(pluginDirectory))
            {
                if (strlen(entry->d_name) > 5 && strcmp(sc_filename_get_ext(entry->d_name), "clap") == 0)
                {
                    ++clapCandidateCount;
                }
            }

            if (clapCandidateCount > 0)
            {
                system->clapPlugins = (sc_clap*)ma_malloc(sizeof(sc_clap) * clapCandidateCount, &system->engine.allocationCallbacks);

                if (system->clapPlugins != NULL)
                {
                    /* Second pass: load each plugin into its slot. Failed loads are skipped
                        * so clapPluginCount may end up smaller than clapCandidateCount. */
                    rewinddir(pluginDirectory);

                    for (struct dirent* entry = readdir(pluginDirectory); entry != NULL && system->clapPluginCount < clapCandidateCount; entry = readdir(pluginDirectory))
                    {
                        if (strlen(entry->d_name) > 5 && strcmp(sc_filename_get_ext(entry->d_name), "clap") == 0)
                        {
                            char filePath[1024];
                            snprintf(filePath, sizeof(filePath), "%s/%s", systemConfig->pluginPath, entry->d_name);

                            if (sc_clap_load(filePath, &system->clapPlugins[system->clapPluginCount]) == SBK_SUCCESS)
                            {
                                ++system->clapPluginCount;
                            }
                        }
                    }
                }
            }

            closedir(pluginDirectory);
        }

        ma_log_post(&system->log, MA_LOG_LEVEL_DEBUG, "Initialized CLAP");
    }

    ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef");

    return SBK_SUCCESS;
}

extern sbk_status sc_system_promote_voice_to_real(sc_system* system, sc_voice* voice);

sbk_status sc_system_update(sc_system* system)
{
    SC_CHECK_ARG(system != NULL);

    (void)sc_system_calculate_virtual_voices(system);

    for (sc_uint32 voiceIndex = 0; voiceIndex < system->voiceSlotAllocator.capacity; ++voiceIndex)
    {
        sc_voice* const voice                     = &system->voiceBuffer[voiceIndex];
        const sc_voice_desired_state desiredState = (sc_voice_desired_state)c89atomic_load_32(&voice->desiredState);
        const sc_voice_state currentState         = (sc_voice_state)c89atomic_load_32(&voice->currentState);
        const sc_bool isVirtual                   = SC_VOICE_HAS_FLAG(c89atomic_load_32(&voice->flags), SC_VOICE_FLAG_VIRTUAL);

        switch (desiredState)
        {
            case sc_voice_desired_playing:
                switch (currentState)
                {
                    case sc_voice_state_stopped:
                        SC_ASSERT(false && "Play request on STOPPED slot - use sc_system_play_sound_voice, which sets STARTING");
                        break;
                    case sc_voice_state_starting:
                        if (isVirtual)
                        {
                            // If virtual, there is nothing to do
                            c89atomic_store_32(&voice->currentState, sc_voice_state_playing);
                        }
                        else
                        {
                            (void)sc_system_promote_voice_to_real(system, voice);   
                        }
                        break;
                    case sc_voice_state_playing:
                        if (!isVirtual && voice->realVoiceHandle == 0)
                        {
                            (void)sc_system_promote_voice_to_real(system, voice);
                        }
                        break;
                    case sc_voice_state_stopping:
                        // Tail already running - it wins over a new play request.
                        break;
                }
                break;
            case sc_voice_desired_stopped:
                switch (currentState)
                {
                    case sc_voice_state_stopped:
                        // Nothing to do.
                        break;
                    case sc_voice_state_starting:
                        // Cancel before any audio plays: skip STOPPING, release slot to STOPPED.
                        break;
                    case sc_voice_state_playing:
                        // Begin tail: set current state to STOPPING (fade + drain).
                        break;
                    case sc_voice_state_stopping:
                        // Tail in progress: check if drained, then release slot to STOPPED.
                        break;
                }
                break;
        }
    }

    return SBK_SUCCESS;
}

static sbk_status sc_system_release_clap_plugins(sc_system* system)
{
    SC_CHECK_ARG(system != NULL);

    for (ma_uint32 index = 0; index < system->clapPluginCount; ++index)
    {
        const sbk_status unloadResult = sc_clap_unload(&system->clapPlugins[index]);
        assert(unloadResult == SBK_SUCCESS);
        (void)unloadResult;
    }

    ma_free(system->clapPlugins, &system->engine.allocationCallbacks);
    system->clapPlugins     = NULL;
    system->clapPluginCount = 0;

    return SBK_SUCCESS;
}

sbk_status sc_system_close(sc_system* system)
{
    if (system)
    {
        ma_engine_uninit((ma_engine*)system);

        sc_system_release_clap_plugins(system);

        ma_slot_allocator_uninit(&system->voiceSlotAllocator, &system->engine.allocationCallbacks);
        ma_slot_allocator_uninit(&system->realVoiceSlotAllocator, &system->engine.allocationCallbacks);

        for (sc_uint32 realVoiceIndex = 0; realVoiceIndex < system->realVoiceSlotAllocator.capacity; ++realVoiceIndex)
        {
            SC_FREE(system->realVoiceBuffer[realVoiceIndex].nodeGroup, system);
        }

        SC_FREE(system->voiceBuffer, system);
        SC_FREE(system->realVoiceBuffer, system);
        SC_FREE(system->virtualizeCandidates, system);
        SC_FREE(system->virtualizeBoundary, system);

        ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Closed Sound Chef");
    }
    return SBK_SUCCESS;
}

sbk_status sc_system_read_pcm_frames(sc_system* system, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(framesOut != NULL);
    SC_CHECK_ARG(frameCount != NULL);

    return SC_STATUS_FROM_MA_RESULT(ma_engine_read_pcm_frames((ma_engine*)system, framesOut, frameCount, framesRead));
}

static ma_uint32 get_flags_from_mode(sc_sound_mode mode)
{
    ma_uint32 flags = 0;

    if (mode & SC_SOUND_MODE_DECODE)
    {
        flags |= MA_SOUND_FLAG_DECODE;
    }

    if (mode & SC_SOUND_MODE_ASYNC)
    {
        flags |= MA_SOUND_FLAG_ASYNC;
    }

    if (mode & SC_SOUND_MODE_STREAM)
    {
        flags |= MA_SOUND_FLAG_STREAM;
    }

    return flags;
}

sc_sound_config sc_sound_config_init_file(const char* filePath, sc_sound_mode mode)
{
    sc_sound_config config;
    SC_ZERO_OBJECT(&config);
    config.filePath = filePath;
    config.mode     = mode;
    return config;
}

sc_sound_config sc_sound_config_init_memory(const void* memory, size_t memorySize, sc_sound_mode mode)
{
    sc_sound_config config;
    SC_ZERO_OBJECT(&config);
    config.memory     = memory;
    config.memorySize = memorySize;
    config.mode       = mode;
    return config;
}

sbk_status sc_system_create_sound(sc_system* system, const sc_sound_config* config, sc_sound** sound)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG((config->filePath != NULL) != (config->memory != NULL));
    SC_CHECK_ARG(config->memory == NULL || config->memorySize > 0);

    SC_CREATE(*sound, sc_sound, system);

    (*sound)->mode         = config->mode;
    (*sound)->owningSystem = system;

    if (config->filePath != NULL)
    {
        return (sbk_status)ma_sound_init_from_file((ma_engine*)system, config->filePath, get_flags_from_mode(config->mode), NULL, NULL, &(*sound)->sound);
    }

    SC_CREATE((*sound)->memoryDecoder, ma_decoder, system);

    ma_decoder_config decoderConfig      = ma_decoder_config_init_default();
    decoderConfig.customBackendCount     = system->resourceManager.config.customDecodingBackendCount;
    decoderConfig.ppCustomBackendVTables = system->resourceManager.config.ppCustomDecodingBackendVTables;

    const ma_result decoderInitResult = ma_decoder_init_memory(config->memory, config->memorySize, &decoderConfig, (*sound)->memoryDecoder);

    if (decoderInitResult != MA_SUCCESS)
    {
        ma_decoder_uninit((*sound)->memoryDecoder);
        SC_FREE((*sound)->memoryDecoder, system);
        (*sound)->memoryDecoder = NULL;
        return SC_STATUS_FROM_MA_RESULT(decoderInitResult);
    }

    return (sbk_status)ma_sound_init_from_data_source((ma_engine*)system, (*sound)->memoryDecoder, get_flags_from_mode(config->mode), NULL, &(*sound)->sound);
}

sbk_status sc_system_play_sound(sc_system* system, sc_sound* sound, sc_sound_instance** instance, sc_node_group* parent, sc_bool paused)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(instance != NULL);

    *instance = NULL;

    SC_CREATE(*instance, sc_sound_instance, system);
    (*instance)->mode         = sound->mode;
    (*instance)->owningSystem = sound->owningSystem;

    if (sound->memoryDecoder != NULL)
    {
        if ((*instance)->memoryDecoder == NULL)
        {
            SC_CREATE((*instance)->memoryDecoder, ma_decoder, system);

            ma_decoder_config decoderConfig      = ma_decoder_config_init_default();
            decoderConfig.customBackendCount     = system->resourceManager.config.customDecodingBackendCount;
            decoderConfig.ppCustomBackendVTables = system->resourceManager.config.ppCustomDecodingBackendVTables;

            const ma_result decoderInitResult = ma_decoder_init_memory(sound->memoryDecoder->data.memory.pData,
                                                                       sound->memoryDecoder->data.memory.dataSize,
                                                                       &decoderConfig, (*instance)->memoryDecoder);
            SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(decoderInitResult));

            const ma_result initResult = ma_sound_init_from_data_source((ma_engine*)system, (*instance)->memoryDecoder, sound->mode, NULL, &(*instance)->sound);
            SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(initResult));
        }
    }
    else
    {
        const ma_result copyResult = ma_sound_init_copy((ma_engine*)system, &sound->sound, sound->mode, NULL, &(*instance)->sound);
        SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(copyResult));
    }

    if (parent != NULL)
    {
        const ma_result attachResult = ma_node_attach_output_bus(*instance, 0, parent->tail->node, 0);
        SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(attachResult));
    }
    else if (system->masterNodeGroup != NULL)
    {
        const ma_result attachResult = ma_node_attach_output_bus(*instance, 0, system->masterNodeGroup->tail->node, 0);
        SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(attachResult));
    }

    if (paused == MA_FALSE)
    {
        const ma_result startResult = ma_sound_start(&(*instance)->sound);
        SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(startResult));
    }

    return SBK_SUCCESS;
}

sbk_status sc_system_play_sound_voice(sc_system* system, sc_sound* sound, sc_voice_handle* outVoiceHandle, sc_node_group* parent, sc_bool paused)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(outVoiceHandle != NULL);

    // Get a new virtual slot
    // If we can't find a slot, it's an instant exit because we cannot handle more sounds
    // We don't check for virtual because we're literally over capacity

    ma_uint64 slot = 0;
    SC_CHECK_STATUS(SC_STATUS_FROM_MA_RESULT(ma_slot_allocator_alloc(&system->voiceSlotAllocator, &slot)));
    const ma_uint32 index = SC_VOICE_HANDLE_EXTRACT_INDEX(slot);

    sc_voice* const voice = &system->voiceBuffer[index];

    // Slot must be idle before we take it over.
    const sc_voice_state currentState = (sc_voice_state)c89atomic_load_32(&voice->currentState);
    SC_ASSERT(currentState == sc_voice_state_stopped);

    voice->sound = sound;
    voice->group = parent;

    // Publish the handle before the state so any concurrent stale-handle
    // check that observes STARTING is guaranteed to see the new occupant.
    c89atomic_store_64(&voice->handle, (sc_uint64)slot);
    c89atomic_store_32(&voice->flags, paused ? (sc_uint32)SC_VOICE_FLAG_PAUSED : (sc_uint32)SC_VOICE_FLAG_NONE);
    c89atomic_store_32(&voice->currentState, (sc_uint32)sc_voice_state_starting);
    c89atomic_store_32(&voice->desiredState, (sc_uint32)sc_voice_desired_playing);

    *outVoiceHandle = slot;

    return SBK_SUCCESS;
}

sbk_status sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(nodeGroup != NULL);

    SC_CREATE(*nodeGroup, sc_node_group, system);

    SC_CHECK_STATUS_ELSE_FREE(sc_node_group_init(system, *nodeGroup), *nodeGroup, system);

    return SBK_SUCCESS;
}

extern sc_dsp_description g_dspFaderVTable;
extern sc_dsp_description g_dspLowpassVTable;
extern sc_dsp_description g_dspHighpassVTable;
extern sc_dsp_description g_dspDelayVTable;
extern sc_dsp_description g_dspMeterVTable;
extern sc_dsp_description g_dspClapVTable;

static const sc_dsp_description* const g_builtinDspDescriptions[] =
    {
        NULL,
        &g_dspFaderVTable,
        &g_dspLowpassVTable,
        &g_dspHighpassVTable,
        &g_dspDelayVTable,
        &g_dspMeterVTable,
        &g_dspClapVTable,
};

sbk_status sc_system_get_dsp_desc(const sc_system* system, sc_uint32 handle, const sc_dsp_description** outDescription)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outDescription != NULL);
    SC_CHECK_ARG(handle > 0);

    if (handle < (sc_int32)sc_dsp_type_count)
    {
        *outDescription = g_builtinDspDescriptions[handle];
    }
    else
    {
        const sc_uint32 userTypeIndex = handle - (sc_uint32)sc_dsp_type_count - 1;
        SC_CHECK_ARG(userTypeIndex < SC_MAX_USER_DSP_TYPES);
        *outDescription = system->userDspRegistry[userTypeIndex];
    }

    return SBK_SUCCESS;
}

sbk_status sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->dspDescription != NULL);
    SC_CHECK_ARG(config->dspDescription->create != NULL);
    SC_CHECK_ARG(config->dspDescription->release != NULL);
    SC_CHECK_ARG(dsp != NULL);

    SC_CREATE(*dsp, sc_dsp, system);

    (*dsp)->handle = config->handle;
    (*dsp)->system = system;

    const sbk_status createResult = config->dspDescription->create(system, *dsp, config->clapFactory);

    if (createResult != SBK_SUCCESS)
    {
        SC_FREE(*dsp, system);
    }
    else if ((*dsp)->node == NULL)
    {
        SC_FREE(*dsp, system);
        return SBK_ERR_NULL;
    }

    return createResult;
}

sbk_status sc_system_clap_get_count(const sc_system* system, ma_uint32* count)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(count != NULL);

    *count = system->clapPluginCount;

    return SBK_SUCCESS;
}

sbk_status sc_system_clap_get_at(const sc_system* system, ma_uint32 index, sc_clap** plugin)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(plugin != NULL);

    ma_uint32 clapCount = 0;
    sc_system_clap_get_count(system, &clapCount);
    SC_CHECK_ARG(index < clapCount);
    SC_CHECK(clapCount > 0, SC_STATUS_FROM_MA_RESULT(MA_DOES_NOT_EXIST));

    if (clapCount > 0)
    {
        *plugin = &system->clapPlugins[index];
    }

    return SBK_SUCCESS;
}

#define CLAP_ENTRY "clap_entry"

#if defined(MA_WIN32)
    #include <windows.h>
#endif

#ifdef MA_POSIX
    /* No need for dlfcn.h if we're not using runtime linking. */
    #ifndef MA_NO_RUNTIME_LINKING
        #include <dlfcn.h>
    #endif
#endif

ma_handle sc_dlopen(ma_log* pLog, const char* filename)
{
#ifndef MA_NO_RUNTIME_LINKING
    ma_handle handle;

    ma_log_postf(pLog, MA_LOG_LEVEL_DEBUG, "Loading library: %s\n", filename);

    #ifdef MA_WIN32
        /* From MSDN: Desktop applications cannot use LoadPackagedLibrary; if a desktop application calls this function
         * it fails with APPMODEL_ERROR_NO_PACKAGE.*/
        #if !defined(MA_WIN32_UWP) ||   \
            !(defined(WINAPI_FAMILY) && \
              ((defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)))
    handle = (ma_handle)LoadLibraryA(filename);
        #else
    WCHAR filenameW[4096];
    if (MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenameW, sizeof(filenameW)) == 0)
    {
        handle = NULL;
    }
    else
    {
        handle = (ma_handle)LoadPackagedLibrary(filenameW, 0);
    }
        #endif
    #else
    handle = (ma_handle)dlopen(filename, RTLD_NOW);
    #endif

    /*
    I'm not considering failure to load a library an error nor a warning because seamlessly falling through to a
    lower-priority backend is a deliberate design choice. Instead I'm logging it as an informational message.
    */
    if (handle == NULL)
    {
        ma_log_postf(pLog, MA_LOG_LEVEL_INFO, "Failed to load library: %s\n", filename);
    }

    return handle;
#else
    /* Runtime linking is disabled. */
    (void)pLog;
    (void)filename;
    return NULL;
#endif
}

void sc_dlclose(ma_log* pLog, ma_handle handle)
{
#ifndef MA_NO_RUNTIME_LINKING
    #ifdef MA_WIN32
    FreeLibrary((HMODULE)handle);
    #else
    dlclose((void*)handle);
    #endif

    (void)pLog;
#else
    /* Runtime linking is disabled. */
    (void)pLog;
    (void)handle;
#endif
}

ma_proc sc_dlsym(ma_log* pLog, ma_handle handle, const char* symbol)
{
#ifndef MA_NO_RUNTIME_LINKING
    ma_proc proc;

    ma_log_postf(pLog, MA_LOG_LEVEL_DEBUG, "Loading symbol: %s\n", symbol);

    #ifdef _WIN32
    proc = (ma_proc)GetProcAddress((HMODULE)handle, symbol);
    #else
        #if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpedantic"
        #endif
    proc = (ma_proc)dlsym((void*)handle, symbol);
        #if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
            #pragma GCC diagnostic pop
        #endif
    #endif

    if (proc == NULL)
    {
        ma_log_postf(pLog, MA_LOG_LEVEL_WARNING, "Failed to load symbol: %s\n", symbol);
    }

    (void)pLog; /* It's possible for pContext to be unused. */
    return proc;
#else
    /* Runtime linking is disabled. */
    (void)pLog;
    (void)handle;
    (void)symbol;
    return NULL;
#endif
}

const char* sc_filename_get_ext(const char* filename)
{
    if (filename != NULL)
    {
        const char* dot = strrchr(filename, '.');
        if (!dot || dot == filename)
        {
            return "";
        }
        return dot + 1;
    }
    return "";
}

sbk_status sc_clap_load(const char* clapFilePath, sc_clap* clapPlugin)
{
    SC_CHECK_ARG(clapFilePath != NULL);
    SC_CHECK_ARG(clapPlugin != NULL);

    SC_ZERO_OBJECT(clapPlugin);

    ma_handle pluginHandle = sc_dlopen(NULL, clapFilePath);
    SC_CHECK(pluginHandle != NULL, SC_STATUS_FROM_MA_RESULT(MA_ERROR));

    clap_plugin_entry_t* const clapEntry = (clap_plugin_entry_t*)sc_dlsym(NULL, pluginHandle, CLAP_ENTRY);
    SC_CHECK_AND_GOTO(clapEntry != NULL, error_dll);

    if (clapEntry->init(clapFilePath))
    {
        const clap_plugin_factory_t* pluginFactory =
            (const clap_plugin_factory_t*)clapEntry->get_factory(CLAP_PLUGIN_FACTORY_ID);
        SC_CHECK_AND_GOTO(pluginFactory != NULL, error_clap);

        const ma_uint32 pluginCount = pluginFactory->get_plugin_count(pluginFactory);
        SC_CHECK_AND_GOTO(pluginCount > 0, error_clap);

        clapPlugin->dynamicLibraryHandle = pluginHandle;
        clapPlugin->clapEntry            = clapEntry;
        clapPlugin->pluginFactory        = pluginFactory;

        return SBK_SUCCESS;
    }
    else
    {
        goto error_dll;
    }

error_clap:
    clapEntry->deinit();
error_dll:
    sc_dlclose(NULL, pluginHandle);

    return SC_STATUS_FROM_MA_RESULT(MA_ERROR);
}

sbk_status sc_clap_unload(sc_clap* clapPlugin)
{
    SC_CHECK_ARG(clapPlugin != NULL);
    SC_CHECK_ARG(clapPlugin->dynamicLibraryHandle != NULL);
    SC_CHECK_ARG(clapPlugin->clapEntry != NULL);
    SC_CHECK_ARG(clapPlugin->pluginFactory != NULL);

    clapPlugin->clapEntry->deinit();
    sc_dlclose(NULL, clapPlugin->dynamicLibraryHandle);

    SC_ZERO_OBJECT(clapPlugin);

    return SBK_SUCCESS;
}