/**
 * @file
 * @brief A wrapper library for miniaudio that adds types like NodeGroup to act
 * similarly to an FMOD ChannelGroup.
 */

/**
 * @page ChefProgrammerGuide Sound Chef Programmer's Guide
 * 
 * Welcome to Sound Chef's programmer's guide. This guide aims to explain the codebase and help integrate it into a game engine.
 * Check out the UserGuide for help on using the editor.
 * 
 * ## Creating The System Object
 * 
 * How to create the system and why it is important.
 * 
 * ## Playing A Sound
 * 
 * Here is an easy way to create and play a sound.
 */

#ifndef SOUND_CHEF_H
#define SOUND_CHEF_H

/**
 * @todo Add proper DLL support and testing
 */
#ifdef SC_DLL
    #define MA_DLL
#endif

#include "miniaudio.h"

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_WIN32)
    #define SC_CALL __stdcall
#else
    #define SC_CALL
#endif

#if defined(_WIN32)
    #define SC_DLL_IMPORT  __declspec(dllimport)
    #define SC_DLL_EXPORT  __declspec(dllexport)
    #define SC_DLL_PRIVATE static
#elif defined(__APPLE__) || defined(__ANDROID__) || defined(__linux__)
    #define SC_DLL_IMPORT  __attribute__((visibility("default")))
    #define SC_DLL_EXPORT  __attribute__((visibility("default")))
    #define SC_DLL_PRIVATE __attribute__((visibility("hidden")))
#else
    #define SC_DLL_IMPORT
    #define SC_DLL_EXPORT
    #define SC_DLL_PRIVATE
#endif

#ifdef SC_DLL
    #define SC_API SC_DLL_EXPORT SC_CALL
#else
    #define SC_API SC_CALL
#endif

    typedef ma_result SC_RESULT;
    typedef ma_sound SC_SOUND;

    typedef struct SC_SYSTEM SC_SYSTEM;
    typedef struct SC_NODE_GROUP SC_NODE_GROUP;
    typedef struct SC_DSP SC_DSP;
    typedef struct SC_DSP_PARAMETER SC_DSP_PARAMETER;
    typedef struct SC_DSP_CONFIG SC_DSP_CONFIG;

    /**
     * @brief Predefined list of DSP types Sound Chef understands
     */
    typedef enum SC_DSP_TYPE
    {
        SC_DSP_TYPE_UNKOWN,  // User created
        SC_DSP_TYPE_FADER,   // ma_engine_node. Calling it a fader to match its
                             // use in busses
        SC_DSP_TYPE_LOWPASS,
        SC_DSP_TYPE_HIGHPASS,
        SC_DSP_TYPE_DELAY
    } SC_DSP_TYPE;

    /**
     * @brief Predefined places to insert a DSP into a NODE_GROUP
     */
    typedef enum SC_DSP_INDEX
    {
        SC_DSP_INDEX_TAIL =
            -2,  // Left/back of the chain and becomes the new input
        SC_DSP_INDEX_HEAD =
            -1  // Right/top of the chain and becomes the new output
    } SC_DSP_INDEX;

    /**
     * @brief Groups nodes together into one unit.
     *
     * Nodes in the group go from left to right, tail to head. Imagine a snake
     * and everything is moving towards the endpoint/device. Any input to the
     * group goes to the tail and all outputs leave from the head.
     *
     * Nodes can be inserted in any position. The specified index becomes the
     * index for the inserted node. Index 0 is the tail.
     */
    struct SC_NODE_GROUP
    {
        SC_DSP* m_tail;  // left/bottom most node in the group. sounds and child
                         // groups connect to this. will equal m_fader upon
                         // startup
        SC_DSP* m_fader;  // the ma_sound_group object which sits in the middle
                          // of the group. effects are added either side of it.
                          // handles volume, pitching etc
        SC_DSP* m_head;   // right/top most node in the group. every node in the
                          // group routes to this before going to a parent group
                          // or endpoint
    };

    /**
     * @brief Object that manages the node graph, sounds, output etc.
     *
     * The SC_SYSTEM is a wrapper for the ma_engine type from miniaudio.
     * This means that SC_SYSTEM has a node graph, resource manager, can output
     * to the user's audio device and everything expected from miniaudio's
     * high-level API.
     *
     * SC_SYSTEM also holds and manages custom Sound Chef types like Node
     * Groups.
     *
     * Sound Chef allows for multiple system objects but it is likely unneeded
     * as future versions will support multiple outputs.
     */
    struct SC_SYSTEM
    {
        ma_engine m_engine;
    };

    /**
     * @name Initialization
     * System intialization and destruction.
     * @{
     */

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

    /**@}*/

    /**
     * @name Object Creation
     * Functions for creating Sound Chef objects.
     * @{
     */

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
     * @brief Returns a valid SC_DSP_CONFIG object for the DSP type.
     *
     * @warning Will not return a valid config if the type ==
     * SC_DSP_TYPE_UNKOWN.
     */
    SC_DSP_CONFIG SC_API SC_DSP_Config_Init(SC_DSP_TYPE type);

    /**@}*/

    /**
     * @name DSP
     * DSP is an abstraction over specific effects and the user doesn't need to
     * know the structure of the effect, only its type and any parameters they
     * want to change. This simplifies the design and API for higher level uses
     * like UI and serialization as only become interested in its type and
     * parameter indexes.
     * @{
     */

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

    /**@}*/

    /**
     * @name Node Group
     * @{
     */

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

    /**@}*/

    /**
     * @name Node Group DSP
     * @{
     */

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

    /**@}*/

#include "sound_chef_dsp.h"

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H