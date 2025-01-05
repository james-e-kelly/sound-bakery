#ifndef SOUND_CHEF_COMMON
#define SOUND_CHEF_COMMON

#ifdef sound_chef_shared_EXPORTS
    #define SC_DLL
    #define MA_DLL
#endif

#include "miniaudio.h"
#include "stb_ds.h"
#include "clap/clap.h"

#include <assert.h>
#include <string.h>

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
    #define SC_API      SC_DLL_EXPORT SC_CALL
    #define SC_CLASS    SC_DLL_EXPORT 
#else
    #define SC_API SC_CALL
    #define SC_CLASS
#endif

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
#define SC_CHECK_AND_GOTO(condition, dest) \
    if ((condition) == MA_FALSE)           \
    goto dest

#define SC_ZERO_OBJECT(p) memset((p), 0, sizeof(*(p)))

#define SC_COUNTOF(x)            (sizeof(x) / sizeof(x[0]))
#define SC_MAX(x, y)             (((x) > (y)) ? (x) : (y))
#define SC_MIN(x, y)             (((x) < (y)) ? (x) : (y))
#define SC_ABS(x)                (((x) > 0) ? (x) : -(x))
#define SC_CLAMP(x, lo, hi)      (ma_max(lo, ma_min(x, hi)))
#define SC_OFFSET_PTR(p, offset) (((ma_uint8*)(p)) + (offset))
#define SC_ALIGN(x, a)           (((x) + ((a)-1)) & ~((a)-1))
#define SC_ALIGN_64(x)           ma_align(x, 8)

#ifdef __cplusplus
extern "C"
{
#endif
        
#include "sound_chef/sound_chef_version.h"

typedef ma_bool32 sc_bool;
typedef ma_result sc_result;

typedef struct sc_system sc_system;
typedef struct sc_system_config sc_system_config;

typedef struct sc_sound sc_sound;
typedef struct sc_sound sc_sound_instance;

typedef struct sc_node_group sc_node_group;

typedef struct sc_dsp sc_dsp;
typedef struct sc_dsp_state sc_dsp_state;
typedef struct sc_dsp_config sc_dsp_config;
typedef struct sc_dsp_parameter sc_dsp_parameter;
typedef struct sc_dsp_vtable sc_dsp_vtable;

typedef struct sc_clap sc_clap;

typedef enum sc_sound_mode
{
    SC_SOUND_MODE_DEFAULT   = 0x00000000,   //< Creates a sound in memory and decompresses during runtime
    SC_SOUND_MODE_DECODE    = 0x00000001,   //< Decodes the sound upon loading, instead of runtime
    SC_SOUND_MODE_ASYNC     = 0x00000002,   //< Loads the sound in a background thread
    SC_SOUND_MODE_STREAM    = 0x00000004,   //< Streams parts of the sound from disk during runtime
} sc_sound_mode;

typedef enum sc_dsp_type
{
    SC_DSP_TYPE_UNKOWN,     //< User created
    SC_DSP_TYPE_FADER,
    SC_DSP_TYPE_LOWPASS,
    SC_DSP_TYPE_HIGHPASS,
    SC_DSP_TYPE_DELAY,
    SC_DSP_TYPE_METER,
    SC_DSP_TYPE_CLAP        //< Wraps a CLAP plugin
} sc_dsp_type;

typedef enum sc_dsp_index
{
    SC_DSP_INDEX_TAIL = -2,  //< Left/back of the chain and becomes the new input
    SC_DSP_INDEX_HEAD = -1   //< Right/top of the chain and becomes the new output
} sc_dsp_index;

typedef enum sc_encoding_format
{
    sc_encoding_format_unknown = 0,
    sc_encoding_format_wav,
    sc_encoding_format_adpcm = 10,
    sc_encoding_format_vorbis,
    sc_encoding_format_opus
} sc_encoding_format;

typedef sc_result(SC_CALL* SC_DSP_CREATE_CALLBACK)(sc_dsp_state* dspState);
typedef sc_result(SC_CALL* SC_DSP_RELEASE_CALLBACK)(sc_dsp_state* dspState);
typedef sc_result(SC_CALL* SC_DSP_SET_PARAM_FLOAT_CALLBACK)(sc_dsp_state* dspState, int index, float value);
typedef sc_result(SC_CALL* SC_DSP_GET_PARAM_FLOAT_CALLBACK)(sc_dsp_state* dspState, int index, float* value);

struct sc_dsp_vtable
{
    SC_DSP_CREATE_CALLBACK create;
    SC_DSP_RELEASE_CALLBACK release;
    SC_DSP_SET_PARAM_FLOAT_CALLBACK setFloat;
    SC_DSP_GET_PARAM_FLOAT_CALLBACK getFloat;

    sc_dsp_parameter** params;
    int numParams;
};

/**
 * @brief Holds instance data for a single sc_dsp.
 *
 * Each DSP callback is passed a sc_dsp_state object. This state object can be
 * used to access the system, the user created object (likely a miniaudio node)
 * and the sc_dsp object.
 */
struct sc_dsp_state
{
    void* instance;  //< points to the current sc_dsp object
    void* userData;  //< points to the user created object, likely some type of ma_node
    void* system;    //< points to the owning sc_system object
};

struct sc_dsp_config
{
    sc_dsp_type type;
    sc_dsp_vtable* vtable;
    clap_plugin_factory_t* clapFactory;
};

/**
 * @brief ma_node with an additional enum descriptor.
 *
 * Helper struct to make adding/create DSPs as simple as calling a single
 * function. Raw miniaudio requires you to know the effect you're inserting.
 */
struct sc_dsp
{
    sc_dsp_state* state;    //< holds the instance data for the dsp
    sc_dsp_vtable* vtable;  //< holds the functions for interacting with the underlying node type. Must be not null
    sc_dsp_type type;
    clap_plugin_factory_t* clapFactory; //< If this is a CLAP plugin, the factory to the plugin
    sc_dsp* next;  //< when in a node group, the get_parent/next dsp. Can be null if the head node
    sc_dsp* prev;  //< when in a node group, the child/previous dsp. Can be null if the tail node
};

struct sc_sound
{
    ma_sound sound;
    sc_sound_mode mode;
    ma_decoder* memoryDecoder;
    sc_system* owningSystem;
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
struct sc_node_group
{
    sc_dsp* tail;   //< Left/right most node. Sounds and child groups connect to this
    sc_dsp* fader;  //< Controls the volume and more of the group. Exists at start
    sc_dsp* head;   //< Right/top most node. Nodes in the group route to this. The head then outputs to a get_parent
};

/**
 * @brief Holds a DLL handle and plugin entry for a CLAP plugin.
 */
struct sc_clap
{
    ma_handle dynamicLibraryHandle;         //< Handle to the .clap file
    clap_plugin_entry_t* clapEntry;         //< Entry point of the plugin
    clap_plugin_factory_t* pluginFactory;    //< Plugin factory to poll and create plugins from
};

/**
 * @brief Object that manages the node graph, sounds, output etc.
 *
 * The sc_system is a wrapper for the ma_engine type from miniaudio.
 * This means that sc_system has a node graph, resource manager, can output
 * to the user's audio device and everything expected from miniaudio's
 * high-level API.
 *
 * sc_system also holds and manages custom Sound Chef types like Node
 * Groups.
 *
 * Sound Chef allows for multiple system objects but it is likely unneeded
 * as future versions will support multiple outputs.
 */
struct sc_system
{
    ma_engine engine;
    ma_resource_manager resourceManager; //< We need a custom resource manager for custom decoders
    ma_log log;

    clap_host_t clapHost;
    sc_clap* clapPlugins; //< Dynamic array of opened CLAP plugins

    sc_node_group* masterNodeGroup;
};

/**
 * @brief Configuration for initializing the sc_system.
 * @see sc_system_init
 */
struct sc_system_config
{
    const char* pluginPath; //< Folder path containing CLAP plugins to load
    ma_allocation_callbacks allocationCallbacks;
};

#ifdef __cplusplus
}
#endif

#endif