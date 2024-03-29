#ifndef SOUND_CHEF_H
#define SOUND_CHEF_H

/**
 * @file
 * @brief A wrapper library for miniaudio that adds types like NodeGroup to act
 * similarly to an FMOD ChannelGroup.
 */

/**
 * @page UserManual User Manual
 *
 * - @subpage Design
 * - @subpage Roadmap
 * - @subpage Guides
 */

/**
 * @page Guides Guides
 *
 * - @subpage UserGuide "User Guide"
 * - @subpage ProgrammerGuide "Programmer's Guide"
 */

/**
 * @page UserGuide User Guide
 */

/**
 * @page ProgrammerGuide Programmer's Guide
 *
 * @subpage ChefProgrammerGuide "Sound Chef Programmer Guide"
 */

/**
 * @page ChefProgrammerGuide Sound Chef Programmer's Guide
 *
 * Welcome to Sound Chef's programmer's guide. Sound Chef is a low level audio playback library suited for video games.
 * It can be thought of as an extension and wrapper of miniaudio. Miniaudio will be referenced heavily in this guide and
 * it is recommended to be familiar with the tool. You can read its docs
 * [here](https://miniaud.io/docs/manual/index.html).
 *
 * ## Creating The System Object
 *
 * SC_SYSTEM objects can be created with calls to @ref SC_System_Create. The system must be released by the user with a
 * call to @ref SC_System_Release.
 *
 * Once created, call @ref SC_System_Init to intialize the system and connect it to an audio output.
 *
 * @code
 * SC_SYSTEM* system = NULL;
 * SC_System_Create(&system);
 * SC_System_Init(system);
 * @endcode
 *
 * The system manages the node graph, sounds, resources, and outputting audio to the audio device.
 *
 * ## Playing A Sound
 *
 * With a @ref SC_SYSTEM object, playing a sound is incredibly easy.
 *
 * @code
 * SC_SYSTEM* system = NULL;
 * SC_System_Create(&system);
 * SC_System_Init(system);
 *
 * SC_SOUND* sound = NULL;
 * SC_System_CreateSound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 *
 * SC_SOUND_INSTANCE* instance = NULL;
 * SC_System_PlaySound(system, sound, &instance, NULL, SC_FALSE);
 * @endcode
 *
 * Sound Chef seperates a loaded sound and a playing sound with the @ref SC_SOUND and @ref SC_SOUND_INSTANCE objects.
 * Internally, they are the same object and are simple extensions of the `ma_sound` object.
 *
 * This is done to ensure each call to @ref SC_System_PlaySound creates a brand new sound with its play position set to
 * 0. If multiple calls to @ref SC_System_PlaySound were made to the @ref SC_SOUND object, only one sound would be heard
 * and not many.
 *
 * Memory bloat isn't an issue here as miniaudio doesn't copy the loaded sound buffer but the pointers to that resource.
 * Miniaudio also handles reference counting and will release the resource when all sounds have finished playing and the
 * original @ref SC_SOUND is released.
 *
 * ## DSP
 *
 * For programs where the outcome is not predetermined, there is a need to abstract away strongly defined types. Sound
 * Bakery is a clear example as a user can insert many different types of effects into the signal chain; this would be
 * cumbersome if unique code was requried each time and for each effect.
 *
 * By abstracting effects into @ref SC_DSP objects, a user can quickly call functions like @ref SC_DSP_Config_Init and
 * @ref SC_System_CreateDSP to create many different effects with ease. Sound Chef handles initializing the underlying
 * `ma_*_node` objects.
 *
 * @note
 * Sound Chef uses many of the miniaudio effects like `ma_lpf_node`, `ma_hpf_node` etc. Refer to miniaudio's
 * documentation on how these work.
 *
 * With a DSP created, the user can call @ref SC_DSP_SetParameterFloat to change properties of the effect on the fly.
 *
 * @code
 * const SC_DSP_CONFIG lpfConfig = SC_DSP_Config_Init(SC_DSP_TYPE_LOWPASS);
 * SC_DSP* lowpass = NULL;
 * SC_System_CreateDSP(system, &lpfConfig, &lowpass);
 *
 * SC_DSP_SetParameterFloat(m_lowpass, SC_DSP_LOWPASS_CUTOFF, 500.0f);
 * @endcode
 *
 * Many DSP parameters are defined in @ref sound_chef_dsp.h
 *
 * ## Node Groups
 *
 * Sound Chef adds the concept of "Node Groups". These are analogous to busses and Channels/ChannelControls in FMOD. The
 * intention is to create an easy API for inserting, removing, and modifying DSPs.
 *
 * To create a new @ref SC_NODE_GROUP, call @ref SC_System_CreateNodeGroup. The created node group connects to the
 * endpoint/audio device by default.
 *
 * Sounds can be connected to node groups during calls to @ref SC_System_PlaySound.
 *
 * @code
 * SC_SYSTEM* system = NULL;
 * SC_System_Create(&system);
 * SC_System_Init(system);
 *
 * SC_SOUND* sound = NULL;
 * SC_System_CreateSound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 *
 * SC_NODE_GROUP* nodeGroup = NULL;
 * SC_System_CreateNodeGroup(system, &nodeGroup);
 *
 * SC_SOUND_INSTANCE* instance = NULL;
 * SC_System_PlaySound(system, sound, &instance, nodeGroup, SC_FALSE);
 * @endcode
 *
 * DSP units can be inserted with ease.
 *
 * @code
 * SC_SYSTEM* system = NULL;
 * SC_System_Create(&system);
 * SC_System_Init(system);
 *
 * SC_SOUND* sound = NULL;
 * SC_System_CreateSound(system, "some_sound.wav", SC_SOUND_MODE_DEFAULT, &sound);
 *
 * SC_NODE_GROUP* nodeGroup = NULL;
 * SC_System_CreateNodeGroup(system, &nodeGroup);
 *
 * const SC_DSP_CONFIG lpfConfig = SC_DSP_Config_Init(SC_DSP_TYPE_LOWPASS);
 * SC_DSP* lowpass = NULL;
 * SC_System_CreateDSP(system, &lpfConfig, &lowpass);
 * SC_NodeGroup_AddDSP(nodeGroup, lowpass, SC_DSP_INDEX_HEAD);
 *
 * SC_SOUND_INSTANCE* instance = NULL;
 * SC_System_PlaySound(system, sound, &instance, nodeGroup, SC_FALSE);
 * @endcode
 *
 * Node Groups can also be connected together to create a complex graph.
 *
 * @code
 * SC_NODE_GROUP* parent = NULL;
 * SC_NODE_GROUP* child = NULL;
 *
 * SC_NodeGroup_SetParent(child, parent);
 * @endcode
 *
 * It is recommended to refer to the @ref SB::Engine::NodeInstance class for how Sound Bakery uses Sound Chef to connect
 * node groups together, insert lowpass and highpass effects, and insert user-defined effects.
 */

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
    SC_RESULT SC_API SC_System_PlaySound(
        SC_SYSTEM* system, SC_SOUND* sound, SC_SOUND_INSTANCE** instance, SC_NODE_GROUP* parent, SC_BOOL paused);

    /**
     * @brief Creates a new Node Group
     *
     * Creates a new Node Group with a SC_DSP_TYPE_FADER by default (for
     * handling volume, pitch etc.). Connects to the endpoint by default.
     */
    SC_RESULT SC_API SC_System_CreateNodeGroup(SC_SYSTEM* system, SC_NODE_GROUP** nodeGroup);

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
    SC_RESULT SC_API SC_System_CreateDSP(SC_SYSTEM* system, const SC_DSP_CONFIG* config, SC_DSP** dsp);

    /**
     * @brief Release the resources held by the sound.
     * @param sound to release
     */
    SC_RESULT SC_API SC_Sound_Release(SC_SOUND* sound);

    SC_RESULT SC_API SC_SoundInstance_IsPlaying(SC_SOUND_INSTANCE* instance, SC_BOOL* isPlaying);

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
    SC_RESULT SC_API SC_DSP_GetParameterFloat(SC_DSP* dsp, int index, float* value);

    /**
     * @brief Sets the value on the parameter at that index.
     */
    SC_RESULT SC_API SC_DSP_SetParameterFloat(SC_DSP* dsp, int index, float value);

    /**
     * @brief Releases and frees the DSP object.
     */
    SC_RESULT SC_API SC_DSP_Release(SC_DSP* dsp);

    /**
     * @brief Sets the volume of the group.
     *
     * Sets the volumes on the fader DSP.
     */
    SC_RESULT SC_API SC_NodeGroup_SetVolume(SC_NODE_GROUP* nodeGroup, float volume);

    /**
     * @brief Gets the volume of the group.
     *
     * Gets the volume on the fader DSP.
     */
    SC_RESULT SC_API SC_NodeGroup_GetVolume(SC_NODE_GROUP* nodeGroup, float* volume);

    /**
     * @brief Attaches this group to a parent where this group outputs its audio
     * to the parent.
     *
     * Takes the head DSP and makes its output the tail of the parent.
     */
    SC_RESULT SC_API SC_NodeGroup_SetParent(SC_NODE_GROUP* nodeGroup, SC_NODE_GROUP* parent);

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
    SC_RESULT SC_API SC_NodeGroup_AddDSP(SC_NODE_GROUP* nodeGroup, SC_DSP* dsp, SC_DSP_INDEX index);

    /**
     * @brief
     * @param nodeGroup
     * @param dsp
     * @return
     */
    SC_RESULT SC_API SC_NodeGroup_RemoveDSP(SC_NODE_GROUP* nodeGroup, SC_DSP* dsp);

#include "sound_chef_dsp.h"

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H