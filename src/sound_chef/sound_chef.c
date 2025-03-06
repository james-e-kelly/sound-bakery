#define MINIAUDIO_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION

// Disable built-in decoding in favour of the ones from the example
#define MA_NO_VORBIS
#define MA_NO_OPUS

#include "sound_chef/sound_chef_internal.h"
#include "extras/miniaudio_libopus.h"
#include "extras/miniaudio_libvorbis.h"
#include "sound_chef_encoder.h"

#include <dirent.h>

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

static void sc_system_clap_request_callback(const clap_host_t* host) {}
static void sc_system_clap_request_process(const clap_host_t* host) {}
static void sc_system_clap_request_restart(const clap_host_t* host) {}

sc_result sc_system_create(sc_system** outSystem)
{
    sc_result result = MA_ERROR;

    if (outSystem)
    {
        *outSystem = (sc_system*)ma_malloc(sizeof(sc_system), NULL);

        result = *outSystem ? MA_SUCCESS : MA_ERROR;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_system_release(sc_system* system)
{
    sc_result result = MA_ERROR;

    if (system)
    {
        ma_free(system, NULL);
        result = MA_SUCCESS;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_system_log_init(sc_system* system, ma_log_callback_proc callbackProc)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(callbackProc != NULL);

    ma_result logInitResult = ma_log_init(NULL, &system->log);
    SC_CHECK_RESULT(logInitResult);

    ma_result registerResult = ma_log_register_callback(&system->log, ma_log_callback_init(callbackProc, NULL));
    SC_CHECK_RESULT(registerResult);

    ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef Logging");

    return MA_SUCCESS;
}

sc_system_config SC_API sc_system_config_init_default()
{
    sc_system_config config;
    SC_ZERO_OBJECT(&config);
    return config;
}

sc_system_config SC_API sc_system_config_init(const char* pluginPath)
{
    sc_system_config config = sc_system_config_init_default();
    config.pluginPath       = pluginPath;
    return config;
}

sc_result sc_system_init(sc_system* system, const sc_system_config* systemConfig)
{
    sc_result result = MA_ERROR;

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

        result = ma_resource_manager_init(&resourceManagerConfig, &system->resourceManager);
        SC_CHECK_RESULT(result);

        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.pResourceManager = &system->resourceManager;
        engineConfig.listenerCount    = 1;
        engineConfig.channels         = 2;
        engineConfig.sampleRate       = ma_standard_sample_rate_48000;
        engineConfig.pLog             = &system->log;
        engineConfig.allocationCallbacks = systemConfig->allocationCallbacks;

        result = ma_engine_init(&engineConfig, engine);

        if (result == MA_SUCCESS)
        {
            ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef");

            result = sc_system_create_node_group(system, &system->masterNodeGroup);
            result = sc_node_group_set_parent_endpoint(system->masterNodeGroup);

            const sc_dsp_config meterConfig = sc_dsp_config_init(SC_DSP_TYPE_METER);
            sc_dsp* meterDSP                = NULL;
            result                          = sc_system_create_dsp(system, &meterConfig, &meterDSP);
            result = sc_node_group_add_dsp(system->masterNodeGroup, meterDSP, SC_DSP_INDEX_HEAD);

            if (result == MA_SUCCESS)
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
                                    strcpy(filePath, systemConfig->pluginPath);
                                    strcat(filePath, "/");
                                    strcat(filePath, directoryEntry->d_name);

                                    sc_clap clapPlugin;
                                    if (sc_clap_load(filePath, &clapPlugin) == MA_SUCCESS)
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

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_system_close(sc_system* system)
{
    sc_result result = MA_SUCCESS;

    if (system)
    {
        ma_engine_uninit((ma_engine*)system);

        sc_system_release_clap_plugins(system);

        ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Closed Sound Chef");
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

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

sc_result sc_system_create_sound(sc_system* system, const char* fileName, sc_sound_mode mode, sc_sound** sound)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(fileName != NULL);
    SC_CHECK_ARG(sound != NULL);

    SC_CREATE(*sound, sc_sound, system);

    (*sound)->mode = mode;
    (*sound)->owningSystem = system;

    return ma_sound_init_from_file((ma_engine*)system, fileName, get_flags_from_mode(mode), NULL, NULL,
                                   &(*sound)->sound);
}

sc_result sc_system_create_sound_memory(
    sc_system* system, void* data, size_t dataSize, sc_sound_mode mode, sc_sound** sound)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(data != NULL);
    SC_CHECK_ARG(dataSize > 0);
    SC_CHECK_ARG(sound != NULL);

    SC_CREATE(*sound, sc_sound, system);
    SC_CREATE((*sound)->memoryDecoder, ma_decoder, system);

    (*sound)->mode = mode;
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
        return decoderInitResult;
    }

    return ma_sound_init_from_data_source((ma_engine*)system, (*sound)->memoryDecoder, get_flags_from_mode(mode), NULL,
                                          &(*sound)->sound);
}

sc_result sc_system_play_sound(
    sc_system* system, sc_sound* sound, sc_sound_instance** instance, sc_node_group* parent, sc_bool paused)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(instance != NULL);

    SC_CREATE(*instance, sc_sound_instance, system);
    (*instance)->mode = sound->mode;
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

            const ma_result initResult = ma_sound_init_from_data_source((ma_engine*)system, (*instance)->memoryDecoder,
                                                                        sound->mode, NULL, &(*instance)->sound);
            SC_CHECK_RESULT(initResult);
        }
    }
    else
    {
        const ma_result copyResult =
            ma_sound_init_copy((ma_engine*)system, &sound->sound, sound->mode, NULL, &(*instance)->sound);
        SC_CHECK_RESULT(copyResult);
    }

    if (parent != NULL)
    {
        ma_result attachResult = ma_node_attach_output_bus(*instance, 0, parent->tail->state->userData, 0);
        SC_CHECK_RESULT(attachResult);
    }
    else if (system->masterNodeGroup != NULL)
    {
        ma_result attachResult =
            ma_node_attach_output_bus(*instance, 0, system->masterNodeGroup->tail->state->userData, 0);
        SC_CHECK_RESULT(attachResult);
    }

    if (paused == MA_FALSE)
    {
        ma_result startResult = ma_sound_start(&(*instance)->sound);
        SC_CHECK_RESULT(startResult);
    }

    return MA_SUCCESS;
}

sc_result sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(nodeGroup != NULL);

    sc_node_group* const master = system->masterNodeGroup;

    sc_result result = MA_ERROR;

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

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->vtable != NULL);
    SC_CHECK_ARG(config->vtable->create != NULL);
    SC_CHECK_ARG(config->vtable->release != NULL);
    SC_CHECK_ARG(dsp != NULL);

    sc_result result = MA_ERROR;

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

    if (result != MA_SUCCESS)
    {
        sc_dsp_release(*dsp);
        *dsp = NULL;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

#pragma endregion

#pragma region Sound

sc_result sc_sound_release(sc_sound* sound)
{
    SC_CHECK_ARG(sound != NULL);
    ma_sound_uninit(&sound->sound);

    if (sound->memoryDecoder != NULL)
    {
        ma_decoder_uninit(sound->memoryDecoder);
        SC_FREE(sound->memoryDecoder, sound->owningSystem);
    }

    SC_FREE(sound, sound->owningSystem);

    return MA_SUCCESS;
}

sc_result SC_API sc_sound_instance_is_playing(sc_sound_instance* instance, sc_bool* isPlaying)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(isPlaying != NULL);

    *isPlaying = ma_sound_is_playing(&instance->sound);

    return MA_SUCCESS;
}

sc_result sc_sound_instance_release(sc_sound_instance* instance)
{
    SC_CHECK_ARG(instance != NULL);
    ma_sound_uninit(&instance->sound);
    SC_FREE(instance, instance->owningSystem);
    return MA_SUCCESS;
}

#pragma endregion

#pragma region DSP

sc_result sc_dsp_get_parameter_float(sc_dsp* dsp, int index, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->vtable != NULL);
    SC_CHECK_ARG(dsp->vtable->getFloat != NULL);
    SC_CHECK_ARG(dsp->state != NULL);
    SC_CHECK_ARG(value != NULL);
    SC_CHECK_ARG(index >= 0);

    return dsp->vtable->getFloat(dsp->state, index, value);
}

sc_result sc_dsp_set_parameter_float(sc_dsp* dsp, int index, float value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->vtable != NULL);
    SC_CHECK_ARG(dsp->vtable->setFloat != NULL);
    SC_CHECK_ARG(dsp->state != NULL);
    SC_CHECK_ARG(index >= 0);

    return dsp->vtable->setFloat(dsp->state, index, value);
}

sc_result sc_dsp_release(sc_dsp* dsp)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->vtable != NULL);
    SC_CHECK_ARG(dsp->vtable->release != NULL);
    SC_CHECK_ARG(dsp->state != NULL);

    const ma_allocation_callbacks* allocCallbacks = &((sc_system*)dsp->state->system)->engine.allocationCallbacks;

    sc_result result = dsp->vtable->release(dsp->state);
    ma_free(dsp->state, allocCallbacks);
    ma_free(dsp, allocCallbacks);

    return result;
}

#pragma endregion

#pragma region Node Group

sc_result sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index)
{
    SC_CHECK(index == SC_DSP_INDEX_HEAD, MA_NOT_IMPLEMENTED);
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(dsp->prev == NULL,
             MA_NOT_IMPLEMENTED);  // don't have detatch logic
    SC_CHECK(dsp->next == NULL,
             MA_NOT_IMPLEMENTED);  // don't have detatch logic

    sc_result result = MA_ERROR;

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
                result = ma_node_attach_output_bus(dsp->state->userData, 0, currentParent, 0);
                SC_CHECK_RESULT(result);

                // Make the current head attach to the DSP (which is now the
                // head)
                result = ma_node_attach_output_bus(currentHead->state->userData, 0, dsp->state->userData, 0);
                SC_CHECK_RESULT(result);

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

            result = ma_node_attach_output_bus(dsp->state->userData, 0, currentTail->state->userData, 0);
            SC_CHECK_RESULT(result);

            break;
        }
        default:
            break;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(parent != NULL);

    return ma_node_attach_output_bus(nodeGroup->head->state->userData, 0, parent->tail->state->userData, 0);
}

sc_result sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    sc_system* const system = (sc_system*)nodeGroup->fader->state->system;
    SC_CHECK(system != NULL, MA_BAD_ADDRESS);

    ma_node* const endPoint = ma_node_graph_get_endpoint((ma_node_graph*)system);
    SC_CHECK(endPoint != NULL, MA_BAD_ADDRESS);

    return ma_node_attach_output_bus(nodeGroup->head->state->userData, 0, endPoint, 0);
}

sc_result sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(nodeGroup->tail != NULL, MA_INVALID_DATA);

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

    return MA_SUCCESS;
}

sc_result sc_node_group_release(sc_node_group* nodeGroup)
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

    return MA_SUCCESS;
}

#pragma endregion

#pragma region DSP Low Level

enum
{
    SC_DSP_CUTOFF_MIN           = 1,
    SC_DSP_CUTOFF_MAX           = 22000,
    SC_DSP_DEFAULT_FILTER_ORDER = 2,
};

#pragma region Fader

static sc_result sc_dsp_fader_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_sound_group), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_sound_group_config config = ma_sound_group_config_init_2((ma_engine*)state->system);
    return ma_sound_group_init_ex((ma_engine*)state->system, &config, (ma_sound_group*)state->userData);
}

static sc_result sc_dsp_fader_release(sc_dsp_state* state)
{
    ma_sound_group_uninit((ma_sound_group*)state->userData);
    SC_FREE(state->userData, (sc_system*)state->system);
    return MA_SUCCESS;
}

static sc_dsp_vtable s_faderVtable = {sc_dsp_fader_create, sc_dsp_fader_release};

#pragma endregion

#pragma region Lowpass

static sc_result sc_dsp_lowpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_lpf_node), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_lpf_node_config config = ma_lpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_lpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_lpf_node*)state->userData);
}

static sc_result sc_dsp_lowpass_release(sc_dsp_state* state)
{
    ma_lpf_node_uninit((ma_lpf_node*)state->userData, NULL);
    SC_FREE(state->userData, (sc_system*)state->system);
    return MA_SUCCESS;
}

static sc_result sc_dsp_lowpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sc_result result = MA_ERROR;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_LOWPASS_CUTOFF:
        {
            ma_lpf_config lpfConfig =
                ma_lpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_lpf_node_reinit(&lpfConfig, state->userData);
            break;
        }
    }

    return result;
}

static sc_result sc_dsp_lowpass_get_param_float(sc_dsp_state* const state, int index, float* const value)
{
    (void)state;
    (void)value;

    sc_result result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_LOWPASS_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_lowpassCutoffParam = {SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MAX};

static sc_dsp_parameter* s_lowpassParams[SC_DSP_LOWPASS_NUM_PARAM] = {&s_lowpassCutoffParam};

static sc_dsp_vtable s_lowpassVtable = {
    sc_dsp_lowpass_create,          sc_dsp_lowpass_release, sc_dsp_lowpass_set_param_float,
    sc_dsp_lowpass_get_param_float, s_lowpassParams,        SC_DSP_LOWPASS_NUM_PARAM};

#pragma endregion

#pragma region Highpass

static sc_result sc_dsp_highpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_hpf_node), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_hpf_node_config config = ma_hpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_hpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_hpf_node*)state->userData);
}

static sc_result sc_dsp_highpass_release(sc_dsp_state* state)
{
    ma_hpf_node_uninit((ma_hpf_node*)state->userData, NULL);
    SC_FREE(state->userData, (sc_system*)state->system);
    return MA_SUCCESS;
}

static sc_result sc_dsp_highpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sc_result result = MA_ERROR;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_HIGHPASS_CUTOFF:
        {
            ma_hpf_config hpfConfig =
                ma_hpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_hpf_node_reinit(&hpfConfig, state->userData);
            break;
        }
    }

    return result;
}

static sc_result sc_dsp_highpass_get_param_float(sc_dsp_state* state, int index, float* const value)
{
    (void)state;
    (void)value;

    sc_result result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_HIGHPASS_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_highpassCutoffParam = {SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                 SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MIN};

static sc_dsp_parameter* s_highpassParams[SC_DSP_HIGHPASS_NUM_PARAM] = {&s_highpassCutoffParam};

static sc_dsp_vtable s_highpassVtable = {
    sc_dsp_highpass_create,          sc_dsp_highpass_release, sc_dsp_highpass_set_param_float,
    sc_dsp_highpass_get_param_float, s_highpassParams,        SC_DSP_HIGHPASS_NUM_PARAM};

#pragma endregion

#pragma region Meter

static void sc_meter_node_process_pcm_frames(
    ma_node* node, const float** framesIn, ma_uint32* const frameCountIn, float** framesOut, ma_uint32* frameCountOut)
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
    MA_ZERO_MEMORY(channelSums, SC_DSP_METER_MAX_CHANNELS * sizeof(float));

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
        const float squareRoot = sqrtf(channelSum / (float)inputFrames);

        const float rms = squareRoot;
        ma_atomic_store_explicit_f32(&meter->rmsLevels[channelIndex].value, rms, ma_atomic_memory_order_relaxed);
    }
}

static ma_node_vtable sc_meter_node_vtable = {sc_meter_node_process_pcm_frames, NULL, 1, 1, MA_NODE_FLAG_PASSTHROUGH};

static ma_uint32 channels = 2;

static sc_result sc_meter_node_init(ma_node_graph* nodeGraph,
                                    const ma_allocation_callbacks* allocCallbacks,
                                    sc_meter_node* node)
{
    ma_node_config baseNodeConfig  = ma_node_config_init();
    baseNodeConfig.vtable          = &sc_meter_node_vtable;
    baseNodeConfig.pInputChannels  = &channels;
    baseNodeConfig.pOutputChannels = &channels;

    return ma_node_init(nodeGraph, &baseNodeConfig, allocCallbacks, node);
}
static void sc_meter_node_uninit(sc_meter_node* node, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_node_uninit(node, allocationCallbacks);
}

static sc_result sc_dsp_meter_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(sc_meter_node), &((sc_system*)state->system)->engine.allocationCallbacks);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    return sc_meter_node_init((ma_node_graph*)state->system, NULL, (sc_meter_node*)state->userData);
}

static sc_result sc_dsp_meter_release(sc_dsp_state* state)
{
    sc_meter_node_uninit((sc_meter_node*)state->userData, NULL);
    SC_FREE(state->userData, (sc_system*)state->system);
    return MA_SUCCESS;
}

static sc_dsp_vtable s_meterVtable = {sc_dsp_meter_create, sc_dsp_meter_release, NULL, NULL, NULL, 0};

sc_result sc_dsp_get_metering_info(sc_dsp* dsp, ma_uint32 channelIndex, sc_dsp_meter meterType, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->type == SC_DSP_TYPE_METER);
    SC_CHECK_ARG(channelIndex >= 0);
    SC_CHECK_ARG(channelIndex <= SC_DSP_METER_MAX_CHANNELS);
    SC_CHECK_ARG(meterType >= 0);
    SC_CHECK_ARG(meterType < SC_DSP_METER_NUM_PARAM);
    SC_CHECK_ARG(value != NULL);

    sc_meter_node* meterNode = (sc_meter_node*)dsp->state->userData;
    SC_CHECK(meterNode != NULL, MA_INVALID_DATA);

    switch (meterType)
    {
        case SC_DSP_METER_PEAK:
            *value = ma_atomic_load_explicit_f32(&meterNode->meter.peakLevels[channelIndex].value,
                                                 ma_atomic_memory_order_relaxed);
            break;
        case SC_DSP_METER_RMS:
            *value = ma_atomic_load_explicit_f32(&meterNode->meter.rmsLevels[channelIndex].value,
                                                 ma_atomic_memory_order_relaxed);
            break;
        default:
            break;
    }

    return MA_SUCCESS;
}

#pragma endregion

#pragma region CLAP

#define SC_CLAP_INPUT_BUS  1
#define SC_CLAP_OUTPUT_BUS 1

static ma_uint32 sc_clap_input_events_size(const clap_input_events_t* list) { return 0; }

static const clap_event_header_t* sc_clap_input_events_get(const clap_input_events_t* list, ma_uint32 index) { return NULL; }

static bool sc_clap_output_events_try_push(const clap_output_events_t* list, const clap_event_header_t* event)
{
    return true;
}

static void sc_clap_node_process_pcm_frames(
    ma_node* node, const float** framesIn, ma_uint32* const frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{
    sc_clap_node* const clapNode    = (sc_clap_node*)node;
    clap_plugin_t* const clapPlugin = clapNode->clapPlugin;

    const ma_uint32 inputChannels  = ma_node_get_input_channels(node, 0);
    const ma_uint32 outputChannels = ma_node_get_output_channels(node, 0);

    assert(inputChannels == outputChannels);
    assert(*frameCountIn == *frameCountOut);

    if (clapPlugin->start_processing(clapPlugin))
    {
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

        void* deinterleavedInputFrames[MA_MAX_CHANNELS];
        void* deinterleavedOutputFrames[MA_MAX_CHANNELS];

        for (int channel = 0; channel < inputChannels; ++channel)
        {
            deinterleavedInputFrames[channel] = ma_malloc(ma_get_bytes_per_sample(ma_format_f32) * *frameCountIn, &((ma_engine*)clapNode->baseNode.pNodeGraph)->allocationCallbacks);
            deinterleavedOutputFrames[channel] = ma_malloc(ma_get_bytes_per_sample(ma_format_f32) * *frameCountOut, &((ma_engine*)clapNode->baseNode.pNodeGraph)->allocationCallbacks);
        }

        ma_deinterleave_pcm_frames(ma_format_f32, inputChannels, *frameCountIn, framesIn[0], &deinterleavedInputFrames);

        inputBuffer.data32        = &deinterleavedInputFrames;
        inputBuffer.channel_count = inputChannels;

        outputBuffer.data32        = &deinterleavedOutputFrames;
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
            ma_silence_pcm_frames(framesOut, *frameCountOut, ma_format_f32, ma_node_get_output_channels(node, 0));
        }
        else
        {
            ma_interleave_pcm_frames(ma_format_f32, outputChannels, *frameCountOut, &deinterleavedOutputFrames,
                                     framesOut[0]);
        }

        for (int channel = 0; channel < inputChannels; ++channel)
        {
            ma_free(deinterleavedInputFrames[channel], &((ma_engine*)clapNode->baseNode.pNodeGraph)->allocationCallbacks);
            ma_free(deinterleavedOutputFrames[channel], &((ma_engine*)clapNode->baseNode.pNodeGraph)->allocationCallbacks);
        }
    }
}

static ma_node_vtable sc_clap_node_vtable = {sc_clap_node_process_pcm_frames, NULL, SC_CLAP_INPUT_BUS,
                                             SC_CLAP_OUTPUT_BUS, 0};

static sc_result sc_clap_node_init(ma_node_graph* nodeGraph,
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

    return ma_node_init(nodeGraph, &baseNodeConfig, allocCallbacks, node);
}
static void sc_clap_node_uninit(sc_clap_node* node, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_node_uninit(node, allocationCallbacks);
}

static sc_result sc_dsp_clap_create(sc_dsp_state* state)
{
    SC_CREATE(state->userData, sc_clap_node, (sc_system*)state->system);

    sc_system* const system                  = state->system;
    sc_clap_node* const clapNode             = (sc_clap_node*)state->userData;
    sc_dsp* const dsp                        = (sc_dsp*)state->instance;
    clap_plugin_factory_t* const clapFactory = dsp->clapFactory;
    SC_CHECK_ARG(clapFactory != NULL);

    const clap_plugin_descriptor_t* const clapDescriptor = clapFactory->get_plugin_descriptor(clapFactory, 0);
    SC_CHECK(clapDescriptor != NULL, MA_ERROR);

    const clap_plugin_t* clapPlugin = clapFactory->create_plugin(clapFactory, &system->clapHost, clapDescriptor->id);
    SC_CHECK(clapPlugin != NULL, MA_ERROR);

    if (!clapPlugin->init(clapPlugin))
    {
        clapPlugin->destroy(clapPlugin);
        return MA_ERROR;
    }

    if (!clapPlugin->activate(clapPlugin, ma_engine_get_sample_rate((ma_engine*)system), 1, 36))
    {
        clapPlugin->destroy(clapPlugin);
        return MA_ERROR;
    }

    clapNode->clapPlugin = clapPlugin;

    return sc_clap_node_init((ma_node_graph*)system, NULL, clapNode);
}

static sc_result sc_dsp_clap_release(sc_dsp_state* state)
{
    SC_CHECK_ARG(state != NULL);
    SC_CHECK_ARG(state->instance != NULL);
    SC_CHECK_ARG(state->system != NULL);
    SC_CHECK_ARG(state->userData != NULL);

    sc_clap_node* const clapNode    = (sc_clap_node*)state->userData;
    clap_plugin_t* const clapPlugin = clapNode->clapPlugin;

    clapPlugin->stop_processing(clapPlugin);
    clapPlugin->deactivate(clapPlugin);
    clapPlugin->destroy(clapPlugin);
    clapNode->clapPlugin = NULL;

    sc_clap_node_uninit(clapNode, &((sc_system*)state->system)->engine.allocationCallbacks);
    SC_FREE(state->userData, (sc_system*)state->system);

    return MA_SUCCESS;
}

static sc_result sc_dsp_clap_set_floatParam(sc_dsp_state* dspState, int index, float value)
{
    return MA_NOT_IMPLEMENTED;
}

static sc_result sc_dsp_clap_get_floatParam(sc_dsp_state* dspState, int index, float* value)
{
    return MA_NOT_IMPLEMENTED;
}

static sc_dsp_vtable s_clapVtable = {
    sc_dsp_clap_create, sc_dsp_clap_release, sc_dsp_clap_set_floatParam, sc_dsp_clap_get_floatParam, NULL, 0};

#pragma endregion

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
            result.vtable = &s_faderVtable;
            break;
        case SC_DSP_TYPE_LOWPASS:
            result.vtable = &s_lowpassVtable;
            break;
        case SC_DSP_TYPE_HIGHPASS:
            result.vtable = &s_highpassVtable;
            break;
        case SC_DSP_TYPE_DELAY:
            break;
        case SC_DSP_TYPE_METER:
            result.vtable = &s_meterVtable;
            break;
        case SC_DSP_TYPE_CLAP:
            result.vtable = &s_clapVtable;
            break;
    }

    return result;
}

sc_dsp_config sc_dsp_config_init_clap(clap_plugin_factory_t* pluginFactory)
{
    sc_dsp_config config = sc_dsp_config_init(SC_DSP_TYPE_CLAP);
    config.clapFactory   = pluginFactory;
    return config;
}

#pragma endregion

sc_result sc_system_clap_get_count(sc_system* system, ma_uint32* count)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(count != NULL);

    *count = arrlen(system->clapPlugins);

    return MA_SUCCESS;
}

sc_result sc_system_clap_get_at(sc_system* system, ma_uint32 index, sc_clap** plugin)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(plugin != NULL);
    SC_CHECK_ARG(index >= 0);

    ma_uint32 clapCount = 0;
    sc_system_clap_get_count(system, &clapCount);
    SC_CHECK_ARG(index < clapCount);
    SC_CHECK(clapCount > 0, MA_DOES_NOT_EXIST);

    if (clapCount > 0)
    {
        *plugin = &system->clapPlugins[index];
    }

    return MA_SUCCESS;
}

#pragma endregion

#pragma region DSP Low Level

enum
{
    SC_DSP_CUTOFF_MIN           = 1,
    SC_DSP_CUTOFF_MAX           = 22000,
    SC_DSP_DEFAULT_FILTER_ORDER = 2,
};

#pragma region Fader

static sc_result sc_dsp_fader_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_sound_group), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_sound_group_config config = ma_sound_group_config_init_2((ma_engine*)state->system);
    return ma_sound_group_init_ex((ma_engine*)state->system, &config, (ma_sound_group*)state->userData);
}

static sc_result sc_dsp_fader_release(sc_dsp_state* state)
{
    ma_sound_group_uninit((ma_sound_group*)state->userData);
    ma_free(state->userData, NULL);
    return MA_SUCCESS;
}

static sc_dsp_vtable s_faderVtable = {sc_dsp_fader_create, sc_dsp_fader_release};

#pragma endregion

#pragma region Lowpass

static sc_result sc_dsp_lowpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_lpf_node), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_lpf_node_config config = ma_lpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MAX, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_lpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_lpf_node*)state->userData);
}

static sc_result sc_dsp_lowpass_release(sc_dsp_state* state)
{
    ma_lpf_node_uninit((ma_lpf_node*)state->userData, NULL);
    ma_free(state->userData, NULL);
    return MA_SUCCESS;
}

static sc_result sc_dsp_lowpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sc_result result = MA_ERROR;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_LOWPASS_CUTOFF:
        {
            ma_lpf_config lpfConfig =
                ma_lpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_lpf_node_reinit(&lpfConfig, state->userData);
            break;
        }
    }

    return result;
}

static sc_result sc_dsp_lowpass_get_param_float(sc_dsp_state* const state, int index, float* const value)
{
    (void)state;
    (void)value;

    sc_result result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_LOWPASS_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_lowpassCutoffParam = {SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MAX};

static sc_dsp_parameter* s_lowpassParams[SC_DSP_LOWPASS_NUM_PARAM] = {&s_lowpassCutoffParam};

static sc_dsp_vtable s_lowpassVtable = {
    sc_dsp_lowpass_create,          sc_dsp_lowpass_release, sc_dsp_lowpass_set_param_float,
    sc_dsp_lowpass_get_param_float, s_lowpassParams,        SC_DSP_LOWPASS_NUM_PARAM};

#pragma endregion

#pragma region Highpass

static sc_result sc_dsp_highpass_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(ma_hpf_node), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    ma_hpf_node_config config = ma_hpf_node_config_init(ma_engine_get_channels((ma_engine*)state->system),
                                                        ma_engine_get_sample_rate((ma_engine*)state->system),
                                                        SC_DSP_CUTOFF_MIN, SC_DSP_DEFAULT_FILTER_ORDER);
    return ma_hpf_node_init((ma_node_graph*)state->system, &config, NULL, (ma_hpf_node*)state->userData);
}

static sc_result sc_dsp_highpass_release(sc_dsp_state* state)
{
    ma_hpf_node_uninit((ma_hpf_node*)state->userData, NULL);
    ma_free(state->userData, NULL);
    return MA_SUCCESS;
}

static sc_result sc_dsp_highpass_set_param_float(sc_dsp_state* state, int index, float value)
{
    (void)value;

    sc_result result = MA_ERROR;

    ma_format format     = ma_format_f32;
    ma_uint32 channels   = ma_node_get_output_channels(state->userData, 0);
    ma_uint32 sampleRate = ma_engine_get_sample_rate((ma_engine*)state->system);

    switch (index)
    {
        default:
            break;
        case SC_DSP_HIGHPASS_CUTOFF:
        {
            ma_hpf_config hpfConfig =
                ma_hpf_config_init(format, channels, sampleRate, value, SC_DSP_DEFAULT_FILTER_ORDER);
            result = ma_hpf_node_reinit(&hpfConfig, state->userData);
            break;
        }
    }

    return result;
}

static sc_result sc_dsp_highpass_get_param_float(sc_dsp_state* state, int index, float* const value)
{
    (void)state;
    (void)value;

    sc_result result = MA_ERROR;

    switch (index)
    {
        default:
        case SC_DSP_HIGHPASS_CUTOFF:
            break;
    }

    return result;
}

static sc_dsp_parameter s_highpassCutoffParam = {SC_DSP_PARAMETER_TYPE_FLOAT, "Cutoff", SC_DSP_CUTOFF_MIN,
                                                 SC_DSP_CUTOFF_MAX, SC_DSP_CUTOFF_MIN};

static sc_dsp_parameter* s_highpassParams[SC_DSP_HIGHPASS_NUM_PARAM] = {&s_highpassCutoffParam};

static sc_dsp_vtable s_highpassVtable = {
    sc_dsp_highpass_create,          sc_dsp_highpass_release, sc_dsp_highpass_set_param_float,
    sc_dsp_highpass_get_param_float, s_highpassParams,        SC_DSP_HIGHPASS_NUM_PARAM};

#pragma endregion

#pragma region Meter

static void sc_meter_node_process_pcm_frames(
    ma_node* node, const float** framesIn, ma_uint32* const frameCountIn, float** framesOut, ma_uint32* frameCountOut)
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
    MA_ZERO_MEMORY(channelSums, SC_DSP_METER_MAX_CHANNELS * sizeof(float));

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
        const float squareRoot = sqrtf(channelSum / (float)inputFrames);

        const float rms = squareRoot;
        ma_atomic_store_explicit_f32(&meter->rmsLevels[channelIndex].value, rms, ma_atomic_memory_order_relaxed);
    }
}

static ma_node_vtable sc_meter_node_vtable = {sc_meter_node_process_pcm_frames, NULL, 1, 1, MA_NODE_FLAG_PASSTHROUGH};

static ma_uint32 channels = 2;

static sc_result sc_meter_node_init(ma_node_graph* nodeGraph,
                                    const ma_allocation_callbacks* allocCallbacks,
                                    sc_meter_node* node)
{
    ma_node_config baseNodeConfig  = ma_node_config_init();
    baseNodeConfig.vtable          = &sc_meter_node_vtable;
    baseNodeConfig.pInputChannels  = &channels;
    baseNodeConfig.pOutputChannels = &channels;

    return ma_node_init(nodeGraph, &baseNodeConfig, allocCallbacks, node);
}
static void sc_meter_node_uninit(sc_meter_node* node, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_node_uninit(node, allocationCallbacks);
}

static sc_result sc_dsp_meter_create(sc_dsp_state* state)
{
    state->userData = ma_malloc(sizeof(sc_meter_node), NULL);
    if (state->userData == NULL)
    {
        return MA_OUT_OF_MEMORY;
    }

    return sc_meter_node_init((ma_node_graph*)state->system, NULL, (sc_meter_node*)state->userData);
}

static sc_result sc_dsp_meter_release(sc_dsp_state* state)
{
    sc_meter_node_uninit((sc_meter_node*)state->userData, NULL);
    ma_free(state->userData, NULL);
    return MA_SUCCESS;
}

static sc_dsp_vtable s_meterVtable = {sc_dsp_meter_create, sc_dsp_meter_release, NULL, NULL, NULL, 0};

sc_result sc_dsp_get_metering_info(sc_dsp* dsp, ma_uint32 channelIndex, sc_dsp_meter meterType, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->type == SC_DSP_TYPE_METER);
    SC_CHECK_ARG(channelIndex >= 0);
    SC_CHECK_ARG(channelIndex <= SC_DSP_METER_MAX_CHANNELS);
    SC_CHECK_ARG(meterType >= 0);
    SC_CHECK_ARG(meterType < SC_DSP_METER_NUM_PARAM);
    SC_CHECK_ARG(value != NULL);

    sc_meter_node* meterNode = (sc_meter_node*)dsp->state->userData;
    SC_CHECK(meterNode != NULL, MA_INVALID_DATA);

    switch (meterType)
    {
        case SC_DSP_METER_PEAK:
            *value = ma_atomic_load_explicit_f32(&meterNode->meter.peakLevels[channelIndex].value,
                                                 ma_atomic_memory_order_relaxed);
            break;
        case SC_DSP_METER_RMS:
            *value = ma_atomic_load_explicit_f32(&meterNode->meter.rmsLevels[channelIndex].value,
                                                 ma_atomic_memory_order_relaxed);
            break;
        default:
            break;
    }

    return MA_SUCCESS;
}

#pragma endregion

#pragma region CLAP

#define SC_CLAP_INPUT_BUS  1
#define SC_CLAP_OUTPUT_BUS 1

static ma_uint32 sc_clap_input_events_size(const clap_input_events_t* list) { return 0; }

static const clap_event_header_t* sc_clap_input_events_get(const clap_input_events_t* list, ma_uint32 index) { return NULL; }

static bool sc_clap_output_events_try_push(const clap_output_events_t* list, const clap_event_header_t* event)
{
    return true;
}

static void sc_clap_node_process_pcm_frames(
    ma_node* node, const float** framesIn, ma_uint32* const frameCountIn, float** framesOut, ma_uint32* frameCountOut)
{
    sc_clap_node* const clapNode    = (sc_clap_node*)node;
    clap_plugin_t* const clapPlugin = clapNode->clapPlugin;

    const ma_uint32 inputChannels  = ma_node_get_input_channels(node, 0);
    const ma_uint32 outputChannels = ma_node_get_output_channels(node, 0);

    assert(inputChannels == outputChannels);
    assert(*frameCountIn == *frameCountOut);

    if (clapPlugin->start_processing(clapPlugin))
    {
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

        void* deinterleavedInputFrames[MA_MAX_CHANNELS];
        void* deinterleavedOutputFrames[MA_MAX_CHANNELS];

        for (int channel = 0; channel < inputChannels; ++channel)
        {
            deinterleavedInputFrames[channel] = ma_malloc(ma_get_bytes_per_sample(ma_format_f32) * *frameCountIn, NULL);
            deinterleavedOutputFrames[channel] =
                ma_malloc(ma_get_bytes_per_sample(ma_format_f32) * *frameCountOut, NULL);
        }

        ma_deinterleave_pcm_frames(ma_format_f32, inputChannels, *frameCountIn, framesIn[0], &deinterleavedInputFrames);

        inputBuffer.data32        = &deinterleavedInputFrames;
        inputBuffer.channel_count = inputChannels;

        outputBuffer.data32        = &deinterleavedOutputFrames;
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
            ma_silence_pcm_frames(framesOut, *frameCountOut, ma_format_f32, ma_node_get_output_channels(node, 0));
        }
        else
        {
            ma_interleave_pcm_frames(ma_format_f32, outputChannels, *frameCountOut, &deinterleavedOutputFrames,
                                     framesOut[0]);
        }

        for (int channel = 0; channel < inputChannels; ++channel)
        {
            ma_free(deinterleavedInputFrames[channel], NULL);
            ma_free(deinterleavedOutputFrames[channel], NULL);
        }
    }
}

static ma_node_vtable sc_clap_node_vtable = {sc_clap_node_process_pcm_frames, NULL, SC_CLAP_INPUT_BUS,
                                             SC_CLAP_OUTPUT_BUS, 0};

static sc_result sc_clap_node_init(ma_node_graph* nodeGraph,
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

    return ma_node_init(nodeGraph, &baseNodeConfig, allocCallbacks, node);
}
static void sc_clap_node_uninit(sc_clap_node* node, const ma_allocation_callbacks* allocationCallbacks)
{
    ma_node_uninit(node, allocationCallbacks);
}

static sc_result sc_dsp_clap_create(sc_dsp_state* state)
{
    SC_CREATE(state->userData, sc_clap_node);

    sc_system* const system                  = state->system;
    sc_clap_node* const clapNode             = (sc_clap_node*)state->userData;
    sc_dsp* const dsp                        = (sc_dsp*)state->instance;
    clap_plugin_factory_t* const clapFactory = dsp->clapFactory;
    SC_CHECK_ARG(clapFactory != NULL);

    const clap_plugin_descriptor_t* const clapDescriptor = clapFactory->get_plugin_descriptor(clapFactory, 0);
    SC_CHECK(clapDescriptor != NULL, MA_ERROR);

    const clap_plugin_t* clapPlugin = clapFactory->create_plugin(clapFactory, &system->clapHost, clapDescriptor->id);
    SC_CHECK(clapPlugin != NULL, MA_ERROR);

    if (!clapPlugin->init(clapPlugin))
    {
        clapPlugin->destroy(clapPlugin);
        return MA_ERROR;
    }

    if (!clapPlugin->activate(clapPlugin, ma_engine_get_sample_rate((ma_engine*)system), 1, 36))
    {
        clapPlugin->destroy(clapPlugin);
        return MA_ERROR;
    }

    clapNode->clapPlugin = clapPlugin;

    return sc_clap_node_init((ma_node_graph*)system, NULL, clapNode);
}

static sc_result sc_dsp_clap_release(sc_dsp_state* state)
{
    SC_CHECK_ARG(state != NULL);
    SC_CHECK_ARG(state->instance != NULL);
    SC_CHECK_ARG(state->system != NULL);
    SC_CHECK_ARG(state->userData != NULL);

    sc_clap_node* const clapNode    = (sc_clap_node*)state->userData;
    clap_plugin_t* const clapPlugin = clapNode->clapPlugin;

    clapPlugin->stop_processing(clapPlugin);
    clapPlugin->deactivate(clapPlugin);
    clapPlugin->destroy(clapPlugin);
    clapNode->clapPlugin = NULL;

    sc_clap_node_uninit(clapNode, NULL);
    ma_free(state->userData, NULL);

    return MA_SUCCESS;
}

static sc_result sc_dsp_clap_set_floatParam(sc_dsp_state* dspState, int index, float value)
{
    return MA_NOT_IMPLEMENTED;
}

static sc_result sc_dsp_clap_get_floatParam(sc_dsp_state* dspState, int index, float* value)
{
    return MA_NOT_IMPLEMENTED;
}

static sc_dsp_vtable s_clapVtable = {
    sc_dsp_clap_create, sc_dsp_clap_release, sc_dsp_clap_set_floatParam, sc_dsp_clap_get_floatParam, NULL, 0};

#pragma endregion

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
            result.vtable = &s_faderVtable;
            break;
        case SC_DSP_TYPE_LOWPASS:
            result.vtable = &s_lowpassVtable;
            break;
        case SC_DSP_TYPE_HIGHPASS:
            result.vtable = &s_highpassVtable;
            break;
        case SC_DSP_TYPE_DELAY:
            break;
        case SC_DSP_TYPE_METER:
            result.vtable = &s_meterVtable;
            break;
        case SC_DSP_TYPE_CLAP:
            result.vtable = &s_clapVtable;
            break;
    }

    return result;
}

sc_dsp_config sc_dsp_config_init_clap(clap_plugin_factory_t* pluginFactory)
{
    sc_dsp_config config = sc_dsp_config_init(SC_DSP_TYPE_CLAP);
    config.clapFactory   = pluginFactory;
    return config;
}

#pragma endregion

sc_result sc_system_clap_get_count(sc_system* system, ma_uint32* count)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(count != NULL);

    *count = arrlen(system->clapPlugins);

    return MA_SUCCESS;
}

sc_result sc_system_clap_get_at(sc_system* system, ma_uint32 index, sc_clap** plugin)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(plugin != NULL);
    SC_CHECK_ARG(index >= 0);

    ma_uint32 clapCount = 0;
    sc_system_clap_get_count(system, &clapCount);
    SC_CHECK_ARG(index < clapCount);
    SC_CHECK(clapCount > 0, MA_DOES_NOT_EXIST);

    if (clapCount > 0)
    {
        *plugin = &system->clapPlugins[index];
    }

    return MA_SUCCESS;
}