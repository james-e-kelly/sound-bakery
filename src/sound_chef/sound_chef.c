#define MINIAUDIO_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION

// Disable built-in decoding in favour of the ones from the example
#define MA_NO_VORBIS
#define MA_NO_OPUS

// clang-format off
#include "sound_chef/sound_chef_internal.h" //< Must be included first. Includes all miniaudio structs for libopus and libvorbis
#include "extras/miniaudio_libopus.h"
#include "extras/miniaudio_libvorbis.h"
// clang-format on
#include "sound_chef_encoder.h"

#include <dirent.h>
#include <stdio.h>

#ifndef NDEBUG
    #define DEBUG_ASSERT(condition) MA_ASSERT(condition)
#else
    #define DEBUG_ASSERT(condition)
#endif  // NDEBUG

#pragma region Decoding

static ma_result ma_decoding_backend_init__libvorbis(void* pUserData,
                                                     ma_read_proc onRead,
                                                     ma_seek_proc onSeek,
                                                     ma_tell_proc onTell,
                                                     void* pReadSeekTellUserData,
                                                     const ma_decoding_backend_config* pConfig,
                                                     const ma_allocation_callbacks* pAllocationCallbacks,
                                                     ma_data_source** ppBackend)
{
    ma_result result;
    ma_libvorbis* pVorbis;

    (void)pUserData;

    pVorbis = (ma_libvorbis*)ma_malloc(sizeof(*pVorbis), pAllocationCallbacks);
    if (pVorbis == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libvorbis_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pVorbis);
    if (result != MA_SUCCESS)
    {
        ma_free(pVorbis, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pVorbis;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__libvorbis(void* pUserData,
                                                          const char* pFilePath,
                                                          const ma_decoding_backend_config* pConfig,
                                                          const ma_allocation_callbacks* pAllocationCallbacks,
                                                          ma_data_source** ppBackend)
{
    ma_result result;
    ma_libvorbis* pVorbis;

    (void)pUserData;

    pVorbis = (ma_libvorbis*)ma_malloc(sizeof(*pVorbis), pAllocationCallbacks);
    if (pVorbis == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libvorbis_init_file(pFilePath, pConfig, pAllocationCallbacks, pVorbis);
    if (result != MA_SUCCESS)
    {
        ma_free(pVorbis, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pVorbis;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__libvorbis(void* pUserData,
                                                  ma_data_source* pBackend,
                                                  const ma_allocation_callbacks* pAllocationCallbacks)
{
    ma_libvorbis* pVorbis = (ma_libvorbis*)pBackend;

    (void)pUserData;

    ma_libvorbis_uninit(pVorbis, pAllocationCallbacks);
    ma_free(pVorbis, pAllocationCallbacks);
}

static ma_result ma_decoding_backend_get_channel_map__libvorbis(void* pUserData,
                                                                ma_data_source* pBackend,
                                                                ma_channel* pChannelMap,
                                                                size_t channelMapCap)
{
    ma_libvorbis* pVorbis = (ma_libvorbis*)pBackend;

    (void)pUserData;

    return ma_libvorbis_get_data_format(pVorbis, NULL, NULL, NULL, pChannelMap, channelMapCap);
}

static ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libvorbis = {
    ma_decoding_backend_init__libvorbis, ma_decoding_backend_init_file__libvorbis, NULL, /* onInitFileW() */
    NULL,                                                                                /* onInitMemory() */
    ma_decoding_backend_uninit__libvorbis};

static ma_result ma_decoding_backend_init__libopus(void* pUserData,
                                                   ma_read_proc onRead,
                                                   ma_seek_proc onSeek,
                                                   ma_tell_proc onTell,
                                                   void* pReadSeekTellUserData,
                                                   const ma_decoding_backend_config* pConfig,
                                                   const ma_allocation_callbacks* pAllocationCallbacks,
                                                   ma_data_source** ppBackend)
{
    ma_result result;
    ma_libopus* pOpus;

    (void)pUserData;

    pOpus = (ma_libopus*)ma_malloc(sizeof(*pOpus), pAllocationCallbacks);
    if (pOpus == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libopus_init(onRead, onSeek, onTell, pReadSeekTellUserData, pConfig, pAllocationCallbacks, pOpus);
    if (result != MA_SUCCESS)
    {
        ma_free(pOpus, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pOpus;

    return MA_SUCCESS;
}

static ma_result ma_decoding_backend_init_file__libopus(void* pUserData,
                                                        const char* pFilePath,
                                                        const ma_decoding_backend_config* pConfig,
                                                        const ma_allocation_callbacks* pAllocationCallbacks,
                                                        ma_data_source** ppBackend)
{
    ma_result result;
    ma_libopus* pOpus;

    (void)pUserData;

    pOpus = (ma_libopus*)ma_malloc(sizeof(*pOpus), pAllocationCallbacks);
    if (pOpus == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    result = ma_libopus_init_file(pFilePath, pConfig, pAllocationCallbacks, pOpus);
    if (result != MA_SUCCESS)
    {
        ma_free(pOpus, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pOpus;

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__libopus(void* pUserData,
                                                ma_data_source* pBackend,
                                                const ma_allocation_callbacks* pAllocationCallbacks)
{
    ma_libopus* pOpus = (ma_libopus*)pBackend;

    (void)pUserData;

    ma_libopus_uninit(pOpus, pAllocationCallbacks);
    ma_free(pOpus, pAllocationCallbacks);
}

static ma_result ma_decoding_backend_get_channel_map__libopus(void* pUserData,
                                                              ma_data_source* pBackend,
                                                              ma_channel* pChannelMap,
                                                              size_t channelMapCap)
{
    ma_libopus* pOpus = (ma_libopus*)pBackend;

    (void)pUserData;

    return ma_libopus_get_data_format(pOpus, NULL, NULL, NULL, pChannelMap, channelMapCap);
}

static ma_decoding_backend_vtable g_ma_decoding_backend_vtable_libopus = {
    ma_decoding_backend_init__libopus, ma_decoding_backend_init_file__libopus, NULL, /* onInitFileW() */
    NULL,                                                                            /* onInitMemory() */
    ma_decoding_backend_uninit__libopus};

#pragma endregion

#pragma region System

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

    DEBUG_ASSERT(result == SBK_SUCCESS);

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

    DEBUG_ASSERT(result == SBK_SUCCESS);

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

            const sc_dsp_config meterConfig = sc_dsp_config_init(SC_DSP_TYPE_METER);
            sc_dsp* meterDSP                = NULL;
            result                          = sc_system_create_dsp(system, &meterConfig, &meterDSP);
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

            if (systemConfig != NULL)
            {
                if (systemConfig->pluginPath != NULL)
                {
                    DIR* const pluginDirectory = opendir(systemConfig->pluginPath);

                    if (pluginDirectory != NULL)
                    {
                        struct dirent* directoryEntry = readdir(pluginDirectory);

                        while (directoryEntry != NULL)
                        {
                            if (strlen(directoryEntry->d_name) > 5)
                            {
                                const char* const fileExt = sc_filename_get_ext(directoryEntry->d_name);

                                const sc_bool fileIsClap = strcmp(fileExt, "clap") == 0;
                                if (fileIsClap)
                                {
                                    char filePath[1024];
                                    snprintf(filePath, sizeof(filePath), "%s/%s", systemConfig->pluginPath,
                                             directoryEntry->d_name);

                                    sc_clap clapPlugin;
                                    if (sc_clap_load(filePath, &clapPlugin) == SBK_SUCCESS)
                                    {
                                        arrput(system->clapPlugins, clapPlugin);
                                    }
                                }
                            }

                            directoryEntry = readdir(pluginDirectory);
                        }

                        closedir(pluginDirectory);
                    }
                }
            }
        }
    }

    DEBUG_ASSERT(result == SBK_SUCCESS);
    DEBUG_ASSERT(maResult == MA_SUCCESS);

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

    DEBUG_ASSERT(result == SBK_SUCCESS);

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
        const ma_result attachResult = ma_node_attach_output_bus(*instance, 0, parent->tail->state->userData, 0);
        SC_CHECK_STATUS(SBK_FROM_MA(attachResult));
    }
    else if (system->masterNodeGroup != NULL)
    {
        const ma_result attachResult = ma_node_attach_output_bus(*instance, 0, system->masterNodeGroup->tail->state->userData, 0);
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
    sc_dsp_config faderConfig = sc_dsp_config_init(SC_DSP_TYPE_FADER);
    result                    = sc_system_create_dsp(system, &faderConfig, &(*nodeGroup)->fader);

    (*nodeGroup)->head = (*nodeGroup)->fader;
    (*nodeGroup)->tail = (*nodeGroup)->fader;

    if (master != NULL)
    {
        sc_node_group_set_parent(*nodeGroup, master);
    }

    DEBUG_ASSERT(result == SBK_SUCCESS);

    return result;
}

sbk_status sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->vtable != NULL);
    SC_CHECK_ARG(config->vtable->create != NULL);
    SC_CHECK_ARG(config->vtable->release != NULL);
    SC_CHECK_ARG(dsp != NULL);

    sbk_status result = SBK_ERR_CHEF;

    *dsp = (sc_dsp*)ma_malloc(sizeof(sc_dsp), &system->engine.allocationCallbacks);
    SC_CHECK_MEM(*dsp);
    MA_ZERO_OBJECT(*dsp);

    (*dsp)->state = ma_malloc(sizeof(sc_dsp_state), &system->engine.allocationCallbacks);
    SC_CHECK_MEM_FREE((*dsp)->state, *dsp);
    MA_ZERO_OBJECT((*dsp)->state);

    (*dsp)->state->instance = *dsp;
    (*dsp)->state->system   = system;

    (*dsp)->type        = config->type;
    (*dsp)->vtable      = config->vtable;
    (*dsp)->clapFactory = config->clapFactory;

    result = (*dsp)->vtable->create((*dsp)->state);

    if (result != SBK_SUCCESS)
    {
        sc_dsp_release(*dsp);
        *dsp = NULL;
    }

    DEBUG_ASSERT(result == SBK_SUCCESS);

    return result;
}

#pragma endregion

#pragma region Sound

sbk_status sc_sound_get_length(sc_sound* sound, float* lengthInSeconds)
{
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(lengthInSeconds != NULL);

    ma_sound_get_length_in_seconds(&sound->sound, lengthInSeconds);

    return SBK_SUCCESS;
}

sbk_status sc_sound_release(sc_sound* sound)
{
    SC_CHECK_ARG(sound != NULL);
    ma_sound_uninit(&sound->sound);

    if (sound->memoryDecoder != NULL)
    {
        ma_decoder_uninit(sound->memoryDecoder);
        SC_FREE(sound->memoryDecoder, sound->owningSystem);
    }

    SC_FREE(sound, sound->owningSystem);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_is_playing(sc_sound_instance* instance, sc_bool* isPlaying)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(isPlaying != NULL);

    *isPlaying = ma_sound_is_playing(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_start(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);

    ma_sound_start(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_pause(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);

    ma_sound_stop(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_get_cursor_in_seconds(sc_sound_instance* instance, float* seconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(seconds != NULL);

    ma_sound_get_cursor_in_seconds(&instance->sound, seconds);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_set_cursor_in_seconds(sc_sound_instance* instance, float seconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(seconds >= 0.0f);

    ma_uint64 lengthInPCMFrames = 0;
    float lengthInSeconds       = 0.0f;

    ma_sound_get_length_in_pcm_frames(&instance->sound, &lengthInPCMFrames);
    ma_sound_get_length_in_seconds(&instance->sound, &lengthInSeconds);

    const float percentage               = seconds / lengthInSeconds;
    const ma_uint32 frameIndexForSeconds = (ma_uint32)((float)lengthInPCMFrames * percentage);

    return SBK_FROM_MA(ma_sound_seek_to_pcm_frame(&instance->sound, frameIndexForSeconds));
}

sbk_status sc_sound_instance_get_loop_position_in_seconds(sc_sound_instance* instance, float* seconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(seconds != NULL);

    ma_data_source* const dataSource = ma_sound_get_data_source(&instance->sound);
    if (dataSource)
    {
        ma_uint64 loopPointInPCMFrames = 0;
        ma_uint32 sampleRate           = 0;

        SC_CHECK_STATUS(SBK_FROM_MA(ma_data_source_get_data_format(dataSource, NULL, NULL, &sampleRate, NULL, 0)));
        ma_data_source_get_loop_point_in_pcm_frames(dataSource, NULL, &loopPointInPCMFrames);

        *seconds = (float)loopPointInPCMFrames / (float)sampleRate;
    }

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_set_loop_position_in_seconds(sc_sound_instance* instance, float loopStartSeconds, float loopEndSeconds)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(loopStartSeconds >= 0.0f);
    SC_CHECK_ARG(loopEndSeconds >= 0.0f);

    ma_data_source* const dataSource =
        ma_sound_get_data_source(&instance->sound);
    if (dataSource)
    {
        ma_uint32 sampleRate = 0;

        SC_CHECK_STATUS(SBK_FROM_MA(ma_data_source_get_data_format(dataSource, NULL, NULL, &sampleRate, NULL, 0)));

        const ma_uint64 loopStartInPCMFrames = (ma_uint64)(loopStartSeconds * (float)sampleRate);
        const ma_uint64 loopEndInPCMFrames   = (ma_uint64)(loopEndSeconds * (float)sampleRate);

        SC_CHECK_STATUS(
            SBK_FROM_MA(ma_data_source_set_loop_point_in_pcm_frames(dataSource, loopStartInPCMFrames, loopEndInPCMFrames)));
    }

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_get_is_looping(sc_sound_instance* instance, sc_bool* looping)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(looping != NULL);

    *looping = ma_sound_is_looping(&instance->sound);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_set_looping(sc_sound_instance* instance, sc_bool looping)
{
    SC_CHECK_ARG(instance != NULL);

    ma_sound_set_looping(&instance->sound, looping);

    return SBK_SUCCESS;
}

sbk_status sc_sound_instance_release(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);
    ma_sound_uninit(&instance->sound);
    SC_FREE(instance, instance->owningSystem);
    return SBK_SUCCESS;
}

#pragma endregion

#pragma region Node Group

sbk_status sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index)
{
    SC_CHECK(index == SC_DSP_INDEX_HEAD, SBK_FROM_MA(MA_NOT_IMPLEMENTED));
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(dsp->prev == NULL,
             SBK_FROM_MA(MA_NOT_IMPLEMENTED));  // don't have detatch logic
    SC_CHECK(dsp->next == NULL,
             SBK_FROM_MA(MA_NOT_IMPLEMENTED));  // don't have detatch logic

    sbk_status result = SBK_ERR_CHEF;

    switch ((int)index)
    {
        case SC_DSP_INDEX_HEAD:
        {
            sc_dsp* currentHead = nodeGroup->head;
            DEBUG_ASSERT(currentHead->next == NULL);  // head nodes can't have
                                                      // something after them
            ma_node_base* currentParent = ((ma_node_base*)currentHead->state->userData)->pOutputBuses[0].pInputNode;
            DEBUG_ASSERT(currentParent != NULL);  // must be attached to
                                                  // something, even if it's the
                                                  // endpoint

            if (currentParent)
            {
                // Attach the dsp to the get_parent output
                result = SBK_FROM_MA(ma_node_attach_output_bus(dsp->state->userData, 0, currentParent, 0));
                SC_CHECK_STATUS(result);

                // Make the current head attach to the DSP (which is now the
                // head)
                result = SBK_FROM_MA(ma_node_attach_output_bus(currentHead->state->userData, 0, dsp->state->userData, 0));
                SC_CHECK_STATUS(result);

                nodeGroup->head->next = dsp;
                dsp->prev             = nodeGroup->head;

                nodeGroup->head = dsp;
            }

            break;
        }
        case 0:
        case SC_DSP_INDEX_TAIL:
        {
            sc_dsp* currentTail = nodeGroup->tail;

            result = SBK_FROM_MA(ma_node_attach_output_bus(dsp->state->userData, 0, currentTail->state->userData, 0));
            SC_CHECK_STATUS(result);

            break;
        }
        default:
            break;
    }

    DEBUG_ASSERT(result == SBK_SUCCESS);

    return result;
}

sbk_status sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(parent != NULL);

    return SBK_FROM_MA(ma_node_attach_output_bus(nodeGroup->head->state->userData, 0, parent->tail->state->userData, 0));
}

sbk_status sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    sc_system* const system = (sc_system*)nodeGroup->fader->state->system;
    SC_CHECK(system != NULL, SBK_ERR_NULL);

    ma_node* const endPoint = ma_node_graph_get_endpoint((ma_node_graph*)system);
    SC_CHECK(endPoint != NULL, SBK_FROM_MA(MA_BAD_ADDRESS));

    return SBK_FROM_MA(ma_node_attach_output_bus(nodeGroup->head->state->userData, 0, endPoint, 0));
}

sbk_status sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(nodeGroup->tail != NULL, SBK_ERR_NULL);

    *dsp = NULL;

    sc_dsp* currentDsp = nodeGroup->tail;

    do
    {
        if (currentDsp->type == type)
        {
            *dsp = currentDsp;
            break;
        }
        currentDsp = currentDsp->next;
    } while (currentDsp != NULL);

    return SBK_SUCCESS;
}

sbk_status sc_node_group_release(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    const sc_system* system = (sc_system*)nodeGroup->fader->state->system;

    sc_dsp* iDSP = nodeGroup->tail;

    while (iDSP != NULL)
    {
        sc_dsp* toFreeDSP = iDSP;
        iDSP              = toFreeDSP->next;
        sc_dsp_release(toFreeDSP);
    }
    SC_FREE(nodeGroup, system);

    return SBK_SUCCESS;
}

#pragma endregion

#pragma region DSP Low Level

extern sc_dsp_vtable g_dspFaderVTable;
extern sc_dsp_vtable g_dspLowpassVTable;
extern sc_dsp_vtable g_dspHighpassVTable;
extern sc_dsp_vtable g_dspDelayVTable;
extern sc_dsp_vtable g_dspMeterVTable;
extern sc_dsp_vtable g_dspClapVTable;

sc_dsp_config sc_dsp_config_init(sc_dsp_type type)
{
    sc_dsp_config result;
    MA_ZERO_OBJECT(&result);

    result.type = type;

    switch (type)
    {
        default:
        case SC_DSP_TYPE_UNKOWN:
            break;
        case SC_DSP_TYPE_FADER:
            result.vtable = &g_dspFaderVTable;
            break;
        case SC_DSP_TYPE_LOWPASS:
            result.vtable = &g_dspLowpassVTable;
            break;
        case SC_DSP_TYPE_HIGHPASS:
            result.vtable = &g_dspHighpassVTable;
            break;
        case SC_DSP_TYPE_DELAY:
            result.vtable = &g_dspDelayVTable;
            break;
        case SC_DSP_TYPE_METER:
            result.vtable = &g_dspMeterVTable;
            break;
        case SC_DSP_TYPE_CLAP:
            result.vtable = &g_dspClapVTable;
            break;
    }

    return result;
}

sc_dsp_config sc_dsp_config_init_clap(const clap_plugin_factory_t* pluginFactory)
{
    sc_dsp_config config = sc_dsp_config_init(SC_DSP_TYPE_CLAP);
    config.clapFactory   = pluginFactory;
    return config;
}

#pragma endregion

sbk_status sc_system_clap_get_count(sc_system* system, ma_uint32* count)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(count != NULL);

    *count = (ma_uint32)arrlen(system->clapPlugins);

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