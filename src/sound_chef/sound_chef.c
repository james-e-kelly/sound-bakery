#define MINIAUDIO_IMPLEMENTATION

// Disable built-in decoding in favour of the ones from the example
#define MA_NO_VORBIS
#define MA_NO_OPUS

#include "sound_chef/sound_chef.h"
#include "sound_chef_encoder.h"

#include "extras/miniaudio_libopus.h"
#include "extras/miniaudio_libvorbis.h"

#ifndef NDEBUG
    #define DEBUG_ASSERT(condition) MA_ASSERT(condition)
#else
    #define DEBUG_ASSERT(condition)
#endif  // NDEBUG

// BACKENDS

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
    ma_decoding_backend_init__libopus, ma_decoding_backend_init_file__libopus, 
    NULL,   /* onInitFileW() */
    NULL,   /* onInitMemory() */
    ma_decoding_backend_uninit__libopus
};

// SOUND CHEF

/**
 * @def mallocs an object, sets it to 0 and checks for errors and potentially
 * returns.
 *
 * Convinience macro for allocating memory _and_ doing checks on it.
 */
#define SC_CREATE(ptr, t)               \
    (ptr) = ma_malloc(sizeof(t), NULL); \
    SC_CHECK_MEM((ptr));                \
    MA_ZERO_OBJECT((ptr))

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

sc_result sc_system_init(sc_system* system)
{
    sc_result result = MA_ERROR;

    if (system)
    {
        ma_engine* engine = (ma_engine*)system;

        ma_decoding_backend_vtable* customBackendVTables[] = {&g_ma_decoding_backend_vtable_libvorbis,
                                                            &g_ma_decoding_backend_vtable_libopus};

        ma_resource_manager_config resourceManagerConfig        = ma_resource_manager_config_init();
        resourceManagerConfig.ppCustomDecodingBackendVTables    = customBackendVTables;
        resourceManagerConfig.customDecodingBackendCount        = sizeof(customBackendVTables) / sizeof(customBackendVTables[0]);
        resourceManagerConfig.pCustomDecodingBackendUserData    = NULL; /* <-- This will be passed in to the pUserData parameter of each function in the decoding backend vtables. */
        resourceManagerConfig.pLog = &system->log;

        result = ma_resource_manager_init(&resourceManagerConfig, &system->resourceManager);
        SC_CHECK_RESULT(result);

        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.pResourceManager = &system->resourceManager;
        engineConfig.listenerCount    = 1;
        engineConfig.channels         = 2;
        engineConfig.sampleRate       = ma_standard_sample_rate_48000;
        engineConfig.pLog             = &system->log;

        result = ma_engine_init(&engineConfig, engine);

        if (result == MA_SUCCESS)
        {
            ma_log_post(&system->log, MA_LOG_LEVEL_INFO, "Initialized Sound Chef");
        }
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_system_close(sc_system* system)
{
    sc_result result = MA_ERROR;

    if (system)
    {
        ma_engine_uninit((ma_engine*)system);
        result = MA_SUCCESS;

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

    SC_CREATE(*sound, sc_sound);

    (*sound)->mode = mode;

    return ma_sound_init_from_file((ma_engine*)system, fileName, get_flags_from_mode(mode), NULL, NULL,
                                   &(*sound)->sound);
}

sc_result sc_system_play_sound(
    sc_system* system, sc_sound* sound, sc_sound_instance** instance, sc_node_group* parent, sc_bool paused)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(instance != NULL);

    SC_CREATE(*instance, sc_sound_instance);
    (*instance)->mode = sound->mode;

    ma_result copyResult =
        ma_sound_init_copy((ma_engine*)system, &sound->sound, sound->mode, NULL, &(*instance)->sound);
    SC_CHECK_RESULT(copyResult);

    if (parent != NULL)
    {
        ma_result attachResult = ma_node_attach_output_bus(*instance, 0, parent->tail->state->userData, 0);
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

    sc_result result = MA_ERROR;

    /**
     * @todo Check for memory allocation failure
     */
    *nodeGroup = (sc_node_group*)ma_malloc(sizeof(sc_node_group), NULL);
    SC_CHECK_MEM(*nodeGroup);
    MA_ZERO_OBJECT(*nodeGroup);

    // Always create a fader/sound_group by default
    sc_dsp_config faderConfig = sc_dsp_config_init(SC_DSP_TYPE_FADER);
    result                    = sc_system_create_dsp(system, &faderConfig, &(*nodeGroup)->fader);

    (*nodeGroup)->head = (*nodeGroup)->fader;
    (*nodeGroup)->tail = (*nodeGroup)->fader;

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

    *dsp = (sc_dsp*)ma_malloc(sizeof(sc_dsp), NULL);
    SC_CHECK_MEM(*dsp);
    MA_ZERO_OBJECT(*dsp);

    (*dsp)->state = ma_malloc(sizeof(sc_dsp_state), NULL);
    SC_CHECK_MEM_FREE((*dsp)->state, *dsp);
    MA_ZERO_OBJECT((*dsp)->state);

    (*dsp)->state->instance = *dsp;
    (*dsp)->state->system   = system;

    (*dsp)->type   = config->type;
    (*dsp)->vtable = config->vtable;

    result = (*dsp)->vtable->create((*dsp)->state);

    if (result != MA_SUCCESS)
    {
        sc_dsp_release(*dsp);
        *dsp = NULL;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

sc_result sc_sound_release(sc_sound* sound)
{
    SC_CHECK_ARG(sound != NULL);
    ma_sound_uninit(&sound->sound);
    ma_free(sound, NULL);
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
    ma_free(instance, NULL);
    return MA_SUCCESS;
}

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

    sc_result result = dsp->vtable->release(dsp->state);
    ma_free(dsp->state, NULL);
    ma_free(dsp, NULL);

    return result;
}

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
                // Attach the dsp to the parent output
                result = ma_node_attach_output_bus(dsp->state->userData, 0, currentParent, 0);
                SC_CHECK_RESULT(result);

                // Make the current head attach to the DSP (which is now the
                // head)
                result = ma_node_attach_output_bus(currentHead->state->userData, 0, dsp->state->userData, 0);
                SC_CHECK_RESULT(result);

                nodeGroup->head->next = dsp;
                dsp->prev               = nodeGroup->head;

                nodeGroup->head = dsp;
                ;
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

sc_result sc_node_group_release(sc_node_group* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    sc_dsp* iDSP = nodeGroup->tail;

    while (iDSP != NULL)
    {
        sc_dsp* toFreeDSP = iDSP;
        iDSP              = toFreeDSP->next;
        sc_dsp_release(toFreeDSP);
    }

    ma_free(nodeGroup, NULL);

    return MA_SUCCESS;
}