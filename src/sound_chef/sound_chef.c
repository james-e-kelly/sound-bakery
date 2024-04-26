#define MINIAUDIO_IMPLEMENTATION

#define MA_NO_VORBIS
#define MA_NO_OPUS

#include "sound_chef/sound_chef.h"

#include "extras/miniaudio_libopus.h"
#include "extras/miniaudio_libvorbis.h"

#ifndef NDEBUG
    #define DEBUG_ASSERT(condition) MA_ASSERT(condition)
#else
    #define DEBUG_ASSERT(condition)
#endif  // NDEBUG

#define SC_CHECK(condition, result) \
    if ((condition) == MA_FALSE)    \
    return (result)
#define SC_CHECK_RESULT(result) \
    if ((result) != MA_SUCCESS) \
    return (result)
#define SC_CHECK_ARG(condition)  \
    if ((condition) == MA_FALSE) \
    return MA_INVALID_ARGS
#define SC_CHECK_MEM(ptr) \
    if ((ptr) == NULL)    \
    return MA_OUT_OF_MEMORY
#define SC_CHECK_MEM_FREE(ptr, freePtr) \
    if ((ptr) == NULL)                  \
    {                                   \
        ma_free((freePtr), NULL);       \
        return MA_OUT_OF_MEMORY;        \
    }

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

SC_RESULT SC_System_Create(SC_SYSTEM** outSystem)
{
    SC_RESULT result = MA_ERROR;

    if (outSystem)
    {
        *outSystem = (SC_SYSTEM*)ma_malloc(sizeof(SC_SYSTEM), NULL);

        result = *outSystem ? MA_SUCCESS : MA_ERROR;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

SC_RESULT SC_System_Release(SC_SYSTEM* system)
{
    SC_RESULT result = MA_ERROR;

    if (system)
    {
        ma_free(system, NULL);
        result = MA_SUCCESS;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

SC_RESULT SC_System_Init(SC_SYSTEM* system)
{
    SC_RESULT result = MA_ERROR;

    if (system)
    {
        ma_engine* engine = (ma_engine*)system;

        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.listenerCount    = 1;
        engineConfig.channels         = 2;
        engineConfig.sampleRate       = ma_standard_sample_rate_48000;

        result = ma_engine_init(&engineConfig, engine);
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

SC_RESULT SC_System_Close(SC_SYSTEM* system)
{
    SC_RESULT result = MA_ERROR;

    if (system)
    {
        ma_engine_uninit((ma_engine*)system);
        result = MA_SUCCESS;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

static ma_uint32 GetFlagsFromMode(SC_SOUND_MODE mode)
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

SC_RESULT SC_System_CreateSound(SC_SYSTEM* system, const char* fileName, SC_SOUND_MODE mode, SC_SOUND** sound)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(fileName != NULL);
    SC_CHECK_ARG(sound != NULL);

    SC_CREATE(*sound, SC_SOUND);

    (*sound)->m_mode = mode;

    return ma_sound_init_from_file((ma_engine*)system, fileName, GetFlagsFromMode(mode), NULL, NULL,
                                   &(*sound)->m_sound);
}

SC_RESULT SC_System_PlaySound(
    SC_SYSTEM* system, SC_SOUND* sound, SC_SOUND_INSTANCE** instance, SC_NODE_GROUP* parent, SC_BOOL paused)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(sound != NULL);
    SC_CHECK_ARG(instance != NULL);

    SC_CREATE(*instance, SC_SOUND_INSTANCE);
    (*instance)->m_mode = sound->m_mode;

    ma_result copyResult =
        ma_sound_init_copy((ma_engine*)system, &sound->m_sound, sound->m_mode, NULL, &(*instance)->m_sound);
    SC_CHECK_RESULT(copyResult);

    if (parent != NULL)
    {
        ma_result attachResult = ma_node_attach_output_bus(*instance, 0, parent->m_tail->m_state->m_userData, 0);
        SC_CHECK_RESULT(attachResult);
    }

    if (paused == MA_FALSE)
    {
        ma_result startResult = ma_sound_start(&(*instance)->m_sound);
        SC_CHECK_RESULT(startResult);
    }

    return MA_SUCCESS;
}

SC_RESULT SC_System_CreateNodeGroup(SC_SYSTEM* system, SC_NODE_GROUP** nodeGroup)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(nodeGroup != NULL);

    SC_RESULT result = MA_ERROR;

    /**
     * @todo Check for memory allocation failure
     */
    *nodeGroup = (SC_NODE_GROUP*)ma_malloc(sizeof(SC_NODE_GROUP), NULL);
    SC_CHECK_MEM(*nodeGroup);
    MA_ZERO_OBJECT(*nodeGroup);

    // Always create a fader/sound_group by default
    SC_DSP_CONFIG faderConfig = SC_DSP_Config_Init(SC_DSP_TYPE_FADER);
    result                    = SC_System_CreateDSP(system, &faderConfig, &(*nodeGroup)->m_fader);

    (*nodeGroup)->m_head = (*nodeGroup)->m_fader;
    (*nodeGroup)->m_tail = (*nodeGroup)->m_fader;

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

SC_RESULT SC_System_CreateDSP(SC_SYSTEM* system, const SC_DSP_CONFIG* config, SC_DSP** dsp)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(config != NULL);
    SC_CHECK_ARG(config->m_vtable != NULL);
    SC_CHECK_ARG(config->m_vtable->m_create != NULL);
    SC_CHECK_ARG(config->m_vtable->m_release != NULL);
    SC_CHECK_ARG(dsp != NULL);

    SC_RESULT result = MA_ERROR;

    *dsp = (SC_DSP*)ma_malloc(sizeof(SC_DSP), NULL);
    SC_CHECK_MEM(*dsp);
    MA_ZERO_OBJECT(*dsp);

    (*dsp)->m_state = ma_malloc(sizeof(SC_DSP_STATE), NULL);
    SC_CHECK_MEM_FREE((*dsp)->m_state, *dsp);
    MA_ZERO_OBJECT((*dsp)->m_state);

    (*dsp)->m_state->m_instance = *dsp;
    (*dsp)->m_state->m_system   = system;

    (*dsp)->m_type   = config->m_type;
    (*dsp)->m_vtable = config->m_vtable;

    result = (*dsp)->m_vtable->m_create((*dsp)->m_state);

    if (result != MA_SUCCESS)
    {
        SC_DSP_Release(*dsp);
        *dsp = NULL;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

SC_RESULT SC_Sound_Release(SC_SOUND* sound)
{
    SC_CHECK_ARG(sound != NULL);
    ma_sound_uninit(&sound->m_sound);
    ma_free(sound, NULL);
    return MA_SUCCESS;
}

SC_RESULT SC_API SC_SoundInstance_IsPlaying(SC_SOUND_INSTANCE* instance, SC_BOOL* isPlaying)
{
    SC_CHECK_ARG(instance != NULL);
    SC_CHECK_ARG(isPlaying != NULL);

    *isPlaying = ma_sound_is_playing(&instance->m_sound);

    return MA_SUCCESS;
}

SC_RESULT SC_SoundInstance_Release(SC_SOUND_INSTANCE* instance)
{
    SC_CHECK_ARG(instance != NULL);
    ma_sound_uninit(&instance->m_sound);
    ma_free(instance, NULL);
    return MA_SUCCESS;
}

SC_RESULT SC_DSP_GetParameterFloat(SC_DSP* dsp, int index, float* value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->m_vtable != NULL);
    SC_CHECK_ARG(dsp->m_vtable->m_getFloat != NULL);
    SC_CHECK_ARG(dsp->m_state != NULL);
    SC_CHECK_ARG(value != NULL);
    SC_CHECK_ARG(index >= 0);

    return dsp->m_vtable->m_getFloat(dsp->m_state, index, value);
}

SC_RESULT SC_DSP_SetParameterFloat(SC_DSP* dsp, int index, float value)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->m_vtable != NULL);
    SC_CHECK_ARG(dsp->m_vtable->m_setFloat != NULL);
    SC_CHECK_ARG(dsp->m_state != NULL);
    SC_CHECK_ARG(index >= 0);

    return dsp->m_vtable->m_setFloat(dsp->m_state, index, value);
}

SC_RESULT SC_DSP_Release(SC_DSP* dsp)
{
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK_ARG(dsp->m_vtable != NULL);
    SC_CHECK_ARG(dsp->m_vtable->m_release != NULL);
    SC_CHECK_ARG(dsp->m_state != NULL);

    SC_RESULT result = dsp->m_vtable->m_release(dsp->m_state);
    ma_free(dsp->m_state, NULL);
    ma_free(dsp, NULL);

    return result;
}

SC_RESULT SC_NodeGroup_AddDSP(SC_NODE_GROUP* nodeGroup, SC_DSP* dsp, SC_DSP_INDEX index)
{
    SC_CHECK(index == SC_DSP_INDEX_HEAD, MA_NOT_IMPLEMENTED);
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(dsp != NULL);
    SC_CHECK(dsp->m_prev == NULL,
             MA_NOT_IMPLEMENTED);  // don't have detatch logic
    SC_CHECK(dsp->m_next == NULL,
             MA_NOT_IMPLEMENTED);  // don't have detatch logic

    SC_RESULT result = MA_ERROR;

    switch ((int)index)
    {
        case SC_DSP_INDEX_HEAD:
        {
            SC_DSP* currentHead = nodeGroup->m_head;
            DEBUG_ASSERT(currentHead->m_next == NULL);  // head nodes can't have
                                                        // something after them
            ma_node_base* currentParent = ((ma_node_base*)currentHead->m_state->m_userData)->pOutputBuses[0].pInputNode;
            DEBUG_ASSERT(currentParent != NULL);  // must be attached to
                                                  // something, even if it's the
                                                  // endpoint

            if (currentParent)
            {
                // Attach the dsp to the parent output
                result = ma_node_attach_output_bus(dsp->m_state->m_userData, 0, currentParent, 0);
                SC_CHECK_RESULT(result);

                // Make the current head attach to the DSP (which is now the
                // head)
                result = ma_node_attach_output_bus(currentHead->m_state->m_userData, 0, dsp->m_state->m_userData, 0);
                SC_CHECK_RESULT(result);

                nodeGroup->m_head->m_next = dsp;
                dsp->m_prev               = nodeGroup->m_head;

                nodeGroup->m_head = dsp;
                ;
            }

            break;
        }
        case 0:
        case SC_DSP_INDEX_TAIL:
        {
            SC_DSP* currentTail = nodeGroup->m_tail;

            result = ma_node_attach_output_bus(dsp->m_state->m_userData, 0, currentTail->m_state->m_userData, 0);
            SC_CHECK_RESULT(result);

            break;
        }
        default:
            break;
    }

    DEBUG_ASSERT(result == MA_SUCCESS);

    return result;
}

SC_RESULT SC_NodeGroup_SetParent(SC_NODE_GROUP* nodeGroup, SC_NODE_GROUP* parent)
{
    SC_CHECK_ARG(nodeGroup != NULL);
    SC_CHECK_ARG(parent != NULL);

    return ma_node_attach_output_bus(nodeGroup->m_head->m_state->m_userData, 0, parent->m_tail->m_state->m_userData, 0);
}

SC_RESULT SC_NodeGroup_Release(SC_NODE_GROUP* nodeGroup)
{
    SC_CHECK_ARG(nodeGroup != NULL);

    SC_DSP* iDSP = nodeGroup->m_tail;

    while (iDSP != NULL)
    {
        SC_DSP* toFreeDSP = iDSP;
        iDSP              = toFreeDSP->m_next;
        SC_DSP_Release(toFreeDSP);
    }

    ma_free(nodeGroup, NULL);

    return MA_SUCCESS;
}