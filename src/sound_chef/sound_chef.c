#define MINIAUDIO_IMPLEMENTATION

// clang-format off
#include "sound_chef/sound_chef_internal.h" //< Must be included first. Includes all miniaudio structs for libopus and libvorbis
// clang-format on

#include "sound_chef_encoder.h"

#include <dirent.h>
#include <stdio.h>

static void sc_system_clap_request_callback(const clap_host_t* host) { (void)host; }
static void sc_system_clap_request_process(const clap_host_t* host) { (void)host; }
static void sc_system_clap_request_restart(const clap_host_t* host) { (void)host; }

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

    const sbk_status logInitResult = SBK_FROM_MA(ma_log_init(NULL, &system->log));
    SC_CHECK_STATUS(logInitResult);

    const sbk_status registerResult = SBK_FROM_MA(ma_log_register_callback(&system->log, ma_log_callback_init(callbackProc, NULL)));
    SC_CHECK_STATUS(registerResult);

    ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef Logging");

    return SBK_SUCCESS;
}

sc_system_config sc_system_config_init_default()
{
    sc_system_config config;
    SC_ZERO_OBJECT(&config);
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

sbk_status sc_system_init(sc_system* system, const sc_system_config* systemConfig)
{
    ma_result maResult = MA_ERROR;
    sbk_status result  = SBK_ERR_CHEF;

    if (system)
    {
        ma_engine* engine = (ma_engine*)system;

        ma_decoding_backend_vtable* customBackendVTables[] = {&g_ma_decoding_backend_vtable_libvorbis,
                                                              &g_ma_decoding_backend_vtable_libopus};

        ma_resource_manager_config resourceManagerConfig     = ma_resource_manager_config_init();
        resourceManagerConfig.ppCustomDecodingBackendVTables = customBackendVTables;
        resourceManagerConfig.customDecodingBackendCount =
            sizeof(customBackendVTables) / sizeof(customBackendVTables[0]);
        resourceManagerConfig.pCustomDecodingBackendUserData =
            NULL; /* <-- This will be passed in to the pUserData parameter of each function in the decoding backend
                     vtables. */
        resourceManagerConfig.pLog = &system->log;

        maResult = ma_resource_manager_init(&resourceManagerConfig, &system->resourceManager);
        SC_CHECK_STATUS(SBK_FROM_MA(maResult));

        ma_engine_config engineConfig    = ma_engine_config_init();
        engineConfig.pResourceManager    = &system->resourceManager;
        engineConfig.listenerCount       = 1;
        engineConfig.channels            = 2;
        engineConfig.sampleRate          = ma_standard_sample_rate_48000;
        engineConfig.pLog                = &system->log;
        engineConfig.allocationCallbacks = systemConfig->allocationCallbacks;
        engineConfig.dataCallback        = systemConfig->dataCallback;

        maResult = ma_engine_init(&engineConfig, engine);

        if (maResult == MA_SUCCESS)
        {
            ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef");

            result = sc_system_create_node_group(system, &system->masterNodeGroup);
            result = sc_node_group_set_parent_endpoint(system->masterNodeGroup);

            sc_dsp* meterDSP                = NULL;
            result                          = sc_system_create_dsp_by_type(system, SC_DSP_TYPE_METER, &meterDSP);
            result                          = sc_node_group_add_dsp(system->masterNodeGroup, meterDSP, SC_DSP_INDEX_HEAD);

            if (result == SBK_SUCCESS)
            {
                ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Master Node Group");
            }

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

            if (systemConfig != NULL && systemConfig->pluginPath != NULL)
            {
                DIR* const pluginDirectory = opendir(systemConfig->pluginPath);

                if (pluginDirectory != NULL)
                {
                    /* First pass: count .clap files so we can allocate exactly once. */
                    ma_uint32 clapCandidateCount = 0;
                    for (struct dirent* entry = readdir(pluginDirectory); entry != NULL;
                         entry                = readdir(pluginDirectory))
                    {
                        if (strlen(entry->d_name) > 5 && strcmp(sc_filename_get_ext(entry->d_name), "clap") == 0)
                        {
                            ++clapCandidateCount;
                        }
                    }

                    if (clapCandidateCount > 0)
                    {
                        system->clapPlugins = (sc_clap*)ma_malloc(sizeof(sc_clap) * clapCandidateCount,
                                                                  &system->engine.allocationCallbacks);

                        if (system->clapPlugins != NULL)
                        {
                            /* Second pass: load each plugin into its slot. Failed loads are skipped
                             * so clapPluginCount may end up smaller than clapCandidateCount. */
                            rewinddir(pluginDirectory);

                            for (struct dirent* entry = readdir(pluginDirectory);
                                 entry != NULL && system->clapPluginCount < clapCandidateCount;
                                 entry = readdir(pluginDirectory))
                            {
                                if (strlen(entry->d_name) > 5 &&
                                    strcmp(sc_filename_get_ext(entry->d_name), "clap") == 0)
                                {
                                    char filePath[1024];
                                    snprintf(filePath, sizeof(filePath), "%s/%s", systemConfig->pluginPath,
                                             entry->d_name);

                                    if (sc_clap_load(filePath, &system->clapPlugins[system->clapPluginCount]) ==
                                        SBK_SUCCESS)
                                    {
                                        ++system->clapPluginCount;
                                    }
                                }
                            }
                        }
                    }

                    closedir(pluginDirectory);
                }
            }
        }
    }

    SC_ASSERT(result == SBK_SUCCESS);
    SC_ASSERT(maResult == MA_SUCCESS);

    return result;
}

sbk_status sc_system_close(sc_system* system)
{
    sbk_status result = SBK_SUCCESS;

    if (system)
    {
        ma_engine_uninit((ma_engine*)system);

        sc_system_release_clap_plugins(system);

        ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Closed Sound Chef");
    }

    SC_ASSERT(result == SBK_SUCCESS);

    return result;
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

sbk_status sc_system_create_sound(sc_system* system, const char* fileName, sc_sound_mode mode, sc_sound** sound)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(fileName != NULL);
    SC_CHECK_ARG(sound != NULL);

    SC_CREATE(*sound, sc_sound, system);

    (*sound)->mode         = mode;
    (*sound)->owningSystem = system;

    return (sbk_status)ma_sound_init_from_file((ma_engine*)system, fileName, get_flags_from_mode(mode), NULL, NULL,
                                               &(*sound)->sound);
}

sbk_status sc_system_create_sound_memory(sc_system* system, const void* data, size_t dataSize, sc_sound_mode mode, sc_sound** sound)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(data != NULL);
    SC_CHECK_ARG(dataSize > 0);
    SC_CHECK_ARG(sound != NULL);

    SC_CREATE(*sound, sc_sound, system);
    SC_CREATE((*sound)->memoryDecoder, ma_decoder, system);

    (*sound)->mode         = mode;
    (*sound)->owningSystem = system;

    ma_decoder_config decoderConfig      = ma_decoder_config_init_default();
    decoderConfig.customBackendCount     = system->resourceManager.config.customDecodingBackendCount;
    decoderConfig.ppCustomBackendVTables = system->resourceManager.config.ppCustomDecodingBackendVTables;

    const ma_result decoderInitResult = ma_decoder_init_memory(data, dataSize, &decoderConfig, (*sound)->memoryDecoder);

    if (decoderInitResult != MA_SUCCESS)
    {
        ma_decoder_uninit((*sound)->memoryDecoder);
        SC_FREE((*sound)->memoryDecoder, system);
        (*sound)->memoryDecoder = NULL;
        return SBK_FROM_MA(decoderInitResult);
    }

    return (sbk_status)ma_sound_init_from_data_source((ma_engine*)system, (*sound)->memoryDecoder, get_flags_from_mode(mode), NULL,
                                                      &(*sound)->sound);
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
            SC_CHECK_STATUS(SBK_FROM_MA(decoderInitResult));

            const ma_result initResult = ma_sound_init_from_data_source((ma_engine*)system, (*instance)->memoryDecoder,
                                                                        sound->mode, NULL, &(*instance)->sound);
            SC_CHECK_STATUS(SBK_FROM_MA(initResult));
        }
    }
    else
    {
        const ma_result copyResult =
            ma_sound_init_copy((ma_engine*)system, &sound->sound, sound->mode, NULL, &(*instance)->sound);
        SC_CHECK_STATUS(SBK_FROM_MA(copyResult));
    }

    if (parent != NULL)
    {
        const ma_result attachResult = ma_node_attach_output_bus(*instance, 0, parent->tail->node, 0);
        SC_CHECK_STATUS(SBK_FROM_MA(attachResult));
    }
    else if (system->masterNodeGroup != NULL)
    {
        const ma_result attachResult = ma_node_attach_output_bus(*instance, 0, system->masterNodeGroup->tail->node, 0);
        SC_CHECK_STATUS(SBK_FROM_MA(attachResult));
    }

    if (paused == MA_FALSE)
    {
        const ma_result startResult = ma_sound_start(&(*instance)->sound);
        SC_CHECK_STATUS(SBK_FROM_MA(startResult));
    }

    return SBK_SUCCESS;
}

sbk_status sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(nodeGroup != NULL);

    sc_node_group* const master = system->masterNodeGroup;

    sbk_status result = SBK_ERR_CHEF;

    SC_CREATE(*nodeGroup, sc_node_group, system);

    // Always create a fader/sound_group by default
    result = sc_system_create_dsp_by_type(system, SC_DSP_TYPE_FADER, &(*nodeGroup)->fader);

    (*nodeGroup)->head = (*nodeGroup)->fader;
    (*nodeGroup)->tail = (*nodeGroup)->fader;

    if (master != NULL)
    {
        sc_node_group_set_parent(*nodeGroup, master);
    }

    SC_ASSERT(result == SBK_SUCCESS);

    return result;
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

sbk_status sc_system_get_dsp_desc(sc_system* system, sc_uint32 handle, const sc_dsp_description** outDescription)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(outDescription != NULL);
    SC_CHECK_ARG(handle > 0);

    if (handle < (sc_int32)SC_DSP_TYPE_COUNT)
    {
        *outDescription = g_builtinDspDescriptions[handle];
    }
    else
    {
        const sc_uint32 userTypeIndex = handle - (sc_uint32)SC_DSP_TYPE_COUNT - 1;
        SC_CHECK_ARG(userTypeIndex < SC_MAX_USER_DSP_TYPES);
        *outDescription = system->userDspRegistry[userTypeIndex];
    }

    return SBK_SUCCESS;
}

sbk_status sc_system_create_dsp(sc_system* system, const sc_dsp_description* description, sc_uint32 type, sc_dsp** dsp, void* userData)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(description != NULL);
    SC_CHECK_ARG(description->create != NULL);
    SC_CHECK_ARG(description->release != NULL);
    SC_CHECK_ARG(dsp != NULL);

    sbk_status result = SBK_ERR_CHEF;

    SC_CREATE(*dsp, sc_dsp, system);

    (*dsp)->handle   = type;
    (*dsp)->system = system;

    result = description->create(system, *dsp, userData);

    if ((*dsp)->node == NULL && result == SBK_SUCCESS)
    {
        result = SBK_ERR_NULL;
    }

    if (result != SBK_SUCCESS)
    {
        SC_FREE(*dsp, system);
    }

    return result;
}

sbk_status sc_system_create_dsp_by_desc(sc_system* system, const sc_dsp_description* description, sc_dsp** dsp)
{
    return sc_system_create_dsp(system, description, (sc_uint32)SC_DSP_TYPE_UNKNOWN, dsp, NULL);
}

sbk_status sc_system_create_dsp_by_type(sc_system* system, sc_dsp_type type, sc_dsp** dsp)
{
    const sc_dsp_description* description = NULL;
    SC_CHECK_STATUS(sc_system_get_dsp_desc(system, (sc_uint32)type, &description));
    return sc_system_create_dsp(system, description, (sc_uint32)type, dsp, NULL);
}

sbk_status sc_system_create_dsp_by_handle(sc_system* system, sc_uint32 handle, sc_dsp** dsp)
{
    const sc_dsp_description* description = NULL;
    SC_CHECK_STATUS(sc_system_get_dsp_desc(system, handle, &description));
    return sc_system_create_dsp(system, description, handle, dsp, NULL);
}

sbk_status sc_system_create_dsp_clap(sc_system* system, const clap_plugin_factory_t* pluginFactory, sc_dsp** dsp)
{
    const sc_dsp_description* description = NULL;
    SC_CHECK_STATUS(sc_system_get_dsp_desc(system, (sc_uint32)SC_DSP_TYPE_CLAP, &description));
    return sc_system_create_dsp(system, description, (sc_uint32)SC_DSP_TYPE_CLAP, dsp, pluginFactory);
}

sbk_status sc_system_clap_get_count(sc_system* system, ma_uint32* count)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(count != NULL);

    *count = system->clapPluginCount;

    return SBK_SUCCESS;
}

sbk_status sc_system_clap_get_at(sc_system* system, ma_uint32 index, sc_clap** plugin)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(plugin != NULL);

    ma_uint32 clapCount = 0;
    sc_system_clap_get_count(system, &clapCount);
    SC_CHECK_ARG(index < clapCount);
    SC_CHECK(clapCount > 0, SBK_FROM_MA(MA_DOES_NOT_EXIST));

    if (clapCount > 0)
    {
        *plugin = &system->clapPlugins[index];
    }

    return SBK_SUCCESS;
}