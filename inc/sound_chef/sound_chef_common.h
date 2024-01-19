#ifndef SOUND_CHEF_COMMON
#define SOUND_CHEF_COMMON

/**
 * @todo Add proper DLL support and testing
 */
#ifdef SC_DLL
    #define MA_DLL
#endif

#include "miniaudio.h"

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

#ifdef __cplusplus
extern "C"
{
#endif

typedef ma_bool32 SC_BOOL;
typedef ma_result SC_RESULT;

typedef struct SC_SYSTEM SC_SYSTEM;
typedef struct SC_SOUND SC_SOUND;
typedef struct SC_SOUND SC_SOUND_INSTANCE;
typedef struct SC_NODE_GROUP SC_NODE_GROUP;
typedef struct SC_DSP SC_DSP;
typedef struct SC_DSP_PARAMETER SC_DSP_PARAMETER;
typedef struct SC_DSP_CONFIG SC_DSP_CONFIG;

typedef enum SC_SOUND_MODE
{
    /**
     * @brief Creates a sound in memory and decompresses during runtime
    */
    SC_SOUND_MODE_DEFAULT   = 0x00000000,
    /**
     * @brief Decodes the sound upon loading, instead of runtime
    */
    SC_SOUND_MODE_DECODE    = 0x00000001, 
    /**
     * @brief Loads the sound in a background thread
    */
    SC_SOUND_MODE_ASYNC     = 0x00000002,
    /**
     * @brief Streams parts of the sound from disk during runtime
    */
    SC_SOUND_MODE_STREAM    = 0x00000004,
} SC_SOUND_MODE;

/**
 * @brief Predefined list of DSP types Sound Chef understands
 */
typedef enum SC_DSP_TYPE
{
    SC_DSP_TYPE_UNKOWN,  //< User created
    SC_DSP_TYPE_FADER,   //< ma_engine_node. Calling it a fader to match its
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
 * @brief An extension of an ma_sound type that remembers its flags.
*/
struct SC_SOUND
{
    ma_sound m_sound;

    SC_SOUND_MODE m_mode;
};

/**
 * @brief Groups nodes/DSPs together into one.
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
    /**
     * @brief Left/bottom most node in the group. 
     * 
     * Sounds and child groups connect to this. Will equal m_fader upon startup
     */
    SC_DSP* m_tail;  

    /**
     * @brief The ma_sound_group object which sits in the middle of the group. 
     * 
     * Effects are added either side of it. Handles volume, pitching etc.
     */
    SC_DSP* m_fader;

    /**
     * @brief Right/top most node in the group. 
     * 
     * Every node in the group routes to this before going to a parent group or endpoint
     */
    SC_DSP* m_head;
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

#ifdef __cplusplus
}
#endif

#endif