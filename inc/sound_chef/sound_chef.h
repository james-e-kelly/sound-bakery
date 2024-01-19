#ifndef SOUND_CHEF_H
#define SOUND_CHEF_H

/**
 * @file
 * @brief A wrapper library for miniaudio that adds types like NodeGroup to act
 * similarly to an FMOD ChannelGroup.
 */

/**
 * @page ChefProgrammerGuide Sound Chef Programmer's Guide
 *
 * Welcome to Sound Chef's programmer's guide. This guide aims to explain the
 * codebase and help integrate it into a game engine. Check out the UserGuide
 * for help on using the editor.
 *
 * ## Creating The System Object
 *
 * How to create the system and why it is important.
 *
 * ## Playing A Sound
 *
 * Here is an easy way to create and play a sound.
 */

/**
 * @todo Add proper DLL support and testing
 */
#ifdef SC_DLL
    #define MA_DLL
#endif

#include "sound_chef_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Creates a new System object.
     * @see SC_System_Init SC_System_Release
     */
    SC_RESULT SC_API SC_System_Create(SC_SYSTEM** outSystem);

    /**
     * @brief Releases the system memory.
     * @see SC_System_Close SC_System_Create
     */
    SC_RESULT SC_API SC_System_Release(SC_SYSTEM* system);

    /**
     * @brief Initialises the system.
     *
     * Initializes the ma_engine and therefore the ma_device. This makes the
     * system connect to the user's audio device and is then ready for playing
     * sounds.
     */
    SC_RESULT SC_API SC_System_Init(SC_SYSTEM* system);

    /**
     * @brief Closes the system.
     *
     * Disconnects the system from the user's audio device and stops all sounds.
     */
    SC_RESULT SC_API SC_System_Close(SC_SYSTEM* system);

    ////////////////////////////////////////////////////////////////////////////

    SC_RESULT SC_API SC_System_CreateSound(SC_SYSTEM* system,
                                           const char* fileName,
                                           SC_SOUND_MODE mode,
                                           SC_SOUND** sound);

    /**
     * @brief Plays a sound and returns the playing instance.
     *
     * Internally, the function copies the passed in sound to the instance. This
     * doesn't copy the internal audio data but rather the runtime parameters
     * like play position etc. This gives us a new ma_sound we can attach into
     * the node graph.
     *
     * @param system system object
     * @param sound to copy to the instance
     * @param instance of the new sound for playing
     * @param parent optional parameter. Outputs to the master node group by
     * default
     * @param paused whether this sound is paused upon creation or played
     * instantly
     * @return
     */
    SC_RESULT SC_API SC_System_PlaySound(SC_SYSTEM* system,
                                         SC_SOUND* sound,
                                         SC_SOUND_INSTANCE** instance,
                                         SC_NODE_GROUP* parent,
                                         SC_BOOL paused);

    /**
     * @brief Creates a new Node Group
     *
     * Creates a new Node Group with a SC_DSP_TYPE_FADER by default (for
     * handling volume, pitch etc.). Connects to the endpoint by default.
     */
    SC_RESULT SC_API SC_System_CreateNodeGroup(SC_SYSTEM* system,
                                               SC_NODE_GROUP** nodeGroup);

    /**
     * @brief Creates a new DSP based on the config.
     *
     * @warning The config parameter must have its m_vtable pointer set.
     * Using SC_DSP_Config_Init will ensure the vtable is correct for built in
     * types. However, users must fill it themselves for custom DSP types.
     *
     * Example:
     * @code
     * SC_DSP* dsp = NULL;
     * SC_DSP_CONFIG lpfConfig = SC_DSP_Config_Init(SC_DSP_TYPE_LOWPASS);
     * SC_System_CreateDSP(system, &lpfConfig, &dsp);
     * @endcode
     *
     * @see SC_DSP_Config_Init
     */
    SC_RESULT SC_API SC_System_CreateDSP(SC_SYSTEM* system,
                                         const SC_DSP_CONFIG* config,
                                         SC_DSP** dsp);

    /**
     * @brief Release the resources held by the sound.
     * @param sound to release
     */
    SC_RESULT SC_API SC_Sound_Release(SC_SOUND* sound);

    SC_RESULT SC_API SC_SoundInstance_IsPlaying(SC_SOUND_INSTANCE* instance,
                                                SC_BOOL* isPlaying);

    SC_RESULT SC_API SC_SoundInstance_Release(SC_SOUND_INSTANCE* instance);

    /**
     * @brief Returns a valid SC_DSP_CONFIG object for the DSP type.
     *
     * @warning Will not return a valid config if the type ==
     * SC_DSP_TYPE_UNKOWN.
     */
    SC_DSP_CONFIG SC_API SC_DSP_Config_Init(SC_DSP_TYPE type);

    /**
     * @brief Gets the paramter value at that index.
     */
    SC_RESULT SC_API SC_DSP_GetParameterFloat(SC_DSP* dsp,
                                              int index,
                                              float* value);

    /**
     * @brief Sets the value on the parameter at that index.
     */
    SC_RESULT SC_API SC_DSP_SetParameterFloat(SC_DSP* dsp,
                                              int index,
                                              float value);

    /**
     * @brief Releases and frees the DSP object.
     */
    SC_RESULT SC_API SC_DSP_Release(SC_DSP* dsp);

    /**
     * @brief Sets the volume of the group.
     *
     * Sets the volumes on the fader DSP.
     */
    SC_RESULT SC_API SC_NodeGroup_SetVolume(SC_NODE_GROUP* nodeGroup,
                                            float volume);

    /**
     * @brief Gets the volume of the group.
     *
     * Gets the volume on the fader DSP.
     */
    SC_RESULT SC_API SC_NodeGroup_GetVolume(SC_NODE_GROUP* nodeGroup,
                                            float* volume);

    /**
     * @brief Attaches this group to a parent where this group outputs its audio
     * to the parent.
     *
     * Takes the head DSP and makes its output the tail of the parent.
     */
    SC_RESULT SC_API SC_NodeGroup_SetParent(SC_NODE_GROUP* nodeGroup,
                                            SC_NODE_GROUP* parent);

    /**
     * @brief Releases and frees this group.
     */
    SC_RESULT SC_API SC_NodeGroup_Release(SC_NODE_GROUP* nodeGroup);

    /**
     * @brief
     * @param nodeGroup
     * @param dsp
     * @param index
     * @return
     */
    SC_RESULT SC_API SC_NodeGroup_AddDSP(SC_NODE_GROUP* nodeGroup,
                                         SC_DSP* dsp,
                                         SC_DSP_INDEX index);

    /**
     * @brief
     * @param nodeGroup
     * @param dsp
     * @return
     */
    SC_RESULT SC_API SC_NodeGroup_RemoveDSP(SC_NODE_GROUP* nodeGroup,
                                            SC_DSP* dsp);

#include "sound_chef_dsp.h"

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H