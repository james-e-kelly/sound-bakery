#ifndef SOUND_CHEF_COMMON
#define SOUND_CHEF_COMMON

#ifdef sound_chef_shared_EXPORTS
    #define SC_DLL
    #define MA_DLL
#endif

/*
 * Build configuration guards, shared by Sound Chef (C) and Sound Bakery (C++).
 *
 * Normally defined by CMake via the SoundBakery::BuildConfig target
 * (cmake/setup_build_config.cmake), which maps them Wwise/FMOD-style:
 *
 *   SBK_CONFIG_DEBUG    - full checks, everything on           (CMake Debug)
 *   SBK_CONFIG_PROFILE  - optimized but instrumented           (CMake RelWithDebInfo)
 *   SBK_CONFIG_RELEASE  - optimized, tooling compiled out      (CMake Release/MinSizeRel)
 *   SBK_CONFIG_ENABLE_LOGGING  - logging compiled in                  (default: not Release)
 *   SBK_CONFIG_ENABLE_PROFILING- profiling/remote connections compiled in (default: not Release)
 *
 * Always defined to 0 or 1 - test with `#if`, never `#ifdef`. The fallbacks
 * below only apply when building against the headers without the CMake
 * target, and derive a sensible mapping from NDEBUG.
 */
#ifndef SBK_HAS_BUILD_CONFIG
    #if defined(NDEBUG)
        #define SBK_CONFIG_DEBUG   0
        #define SBK_CONFIG_PROFILE 0
        #define SBK_CONFIG_RELEASE 1
    #else
        #define SBK_CONFIG_DEBUG   1
        #define SBK_CONFIG_PROFILE 0
        #define SBK_CONFIG_RELEASE 0
    #endif

    #ifndef SBK_CONFIG_ENABLE_LOGGING
        #define SBK_CONFIG_ENABLE_LOGGING (!SBK_CONFIG_RELEASE)
    #endif

    #ifndef SBK_CONFIG_ENABLE_PROFILING
        #define SBK_CONFIG_ENABLE_PROFILING (!SBK_CONFIG_RELEASE)
    #endif
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
    #define SC_API      SC_DLL_EXPORT SC_CALL
    #define SC_CLASS    SC_DLL_EXPORT
#else
    #define SC_API SC_CALL
    #define SC_CLASS
#endif

#define SC_ZERO_OBJECT(p) memset((p), 0, sizeof(*(p)))

#define SC_COUNTOF(x)            (sizeof(x) / sizeof(x[0]))
#define SC_MAX(x, y)             (((x) > (y)) ? (x) : (y))
#define SC_MIN(x, y)             (((x) < (y)) ? (x) : (y))
#define SC_ABS(x)                (((x) > 0) ? (x) : -(x))
#define SC_CLAMP(x, lo, hi)      (ma_max(lo, ma_min(x, hi)))
#define SC_OFFSET_PTR(p, offset) (((ma_uint8*)(p)) + (offset))
#define SC_ALIGN(x, a)           (((x) + ((a)-1)) & ~((a)-1))
#define SC_ALIGN_64(x)           ma_align(x, 8)

#define SC_MAX_CHANNELS         36      //< Support a max of 5th order ambisonics
#define SC_MAX_FRAME_COUNT      2048    //< Safe default for allocating staging areas in memory

#define SC_MAX_USER_DSP_TYPES   16      //< sc_dsp_descriptions are kept in a static array. Increase this if we need to support more DSP types

#ifndef SC_ASSERT
    #define SC_ASSERT(condition) assert(condition)
#endif

#define MA_COINIT_VALUE 0x2  //< COINIT_APARTMENTTHREADED

// Disable built-in decoding in favour of the ones from the example
#define MA_NO_VORBIS
#define MA_NO_OPUS

// Override miniaudio macros so they are usable outside of MINIAUDIO_IMPLEMENTATION sections
#undef MA_ZERO_OBJECT
#undef MA_ZERO_MEMORY
#define MA_ZERO_OBJECT(p)       SC_ZERO_OBJECT((p))
#define MA_ZERO_MEMORY(p, sz)   memset((p), 0, (sz))

#include "miniaudio.h"
#include "clap/clap.h"
#include "c89atomic.h"

#include <assert.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif
        
#include "sound_chef/sound_chef_version.h"

typedef c89atomic_int8      sc_atomic_int8;
typedef c89atomic_uint8     sc_atomic_uint8;
typedef c89atomic_int16     sc_atomic_int16;
typedef c89atomic_uint16    sc_atomic_uint16;
typedef c89atomic_int32     sc_atomic_int32;
typedef c89atomic_uint32    sc_atomic_uint32;
typedef c89atomic_int64     sc_atomic_int64;
typedef c89atomic_uint64    sc_atomic_uint64;
typedef c89atomic_bool      sc_atomic_bool;
typedef float               sc_atomic_float;

typedef ma_uint8            sc_uint8;
typedef ma_uint16           sc_uint16;
typedef ma_bool32           sc_bool;
typedef ma_uint32           sc_uint32;
typedef ma_int32            sc_int32;
typedef ma_int64            sc_int64;
typedef ma_uint64           sc_uint64;

typedef sc_uint64           sc_voice_handle;        //< Voice handle. Contains both a reference count and an index
typedef sc_uint32           sc_voice_refcount;      //< References to this slot. Used to check if a handle is old/stale
typedef sc_uint32           sc_voice_slot;          //< Slot/index into the voice array

static MA_INLINE sc_voice_refcount sc_voice_handle_extract_refcount(sc_voice_handle handle)
{
    return (sc_uint32)(handle >> 32);
}

static MA_INLINE sc_voice_slot sc_voice_handle_extract_slot(sc_voice_handle handle)
{
    return (sc_voice_slot)(handle & 0xFFFFFFFFu);
}

static MA_INLINE sc_voice_handle sc_voice_handle_make(sc_voice_refcount rc, sc_voice_slot slot)
{
    return ((sc_voice_handle)rc << 32) | (sc_voice_handle)slot;
}

#define SBK_FALSE 0
#define SBK_TRUE 1

typedef enum
{
    // < 0: miniaudio Errors
    SBK_SUCCESS = MA_SUCCESS,       //< Success

    // 1-100: User Errors
    SBK_ERR_USER = 1,               //< Generic user error
    SBK_ERR_INVALID_PARAMETER,      //< Invalid parameter given to the function
    SBK_ERR_ALREADY_INITIALIZED,    //< The resource was already initialized
    SBK_ERR_UNITIALIZED,            //< The resource was not initialized
    SBK_ERR_INVALID_OPERATION,      //< Operation is unsupported in this state

    // 101-200: Sound Chef Errors
    SBK_ERR_CHEF = 101,             //< Generic Sound Chef error
    SBK_ERR_CHEF_UNITIALIZED,

    // 201-300: Sound Bakery Errors
    SBK_ERR_BAKERY = 201,           //< Generic Sound Bakery error
    SBK_ERR_BAKERY_UNINITIALIZED,   //< The system object is not created or not initialized
    SBK_ERR_BAKERY_SERIALIZATION,   //< An error happened during serialization
    SBK_ERR_BAKERY_OBJECT_NOT_FOUND,//< An object with the ID or name was not found
    SBK_ERR_BAKERY_OBJECT_EXISTS,   //< The object with this ID or name already exists

    // 301-400: System Errors
    SBK_ERR_SYSTEM = 301,           //< Generic System error
    SBK_ERR_OUT_OF_MEMORY,          //< Could not allocate memory       
    SBK_ERR_INVALID_FILE,
    SBK_ERR_NULL,                   //< Found a null pointer where there shouldn't be one
    SBK_ERR_FULL,                   //< The buffer or container was full and cannot accept more
    SBK_ERR_EMPTY,                  //< The buffer or container was empty and nothing could be read
    SBK_ERR_TOO_LARGE,              //< The request was too large and nothing could be read
    SBK_ERR_AT_END,                 //< At the end of the buffer and cannot go further
    SBK_ERR_NOT_FOUND,              //< The resource was not found

    SBK_ERROR_MAX
} sbk_status;

/**
 * sbk_status shares its numeric space with ma_result (miniaudio codes are <= 0,
 * Sound Bakery codes are > 0), so converting between the two enums is safe.
 * These macros make the conversion explicit at the boundary.
 */
#define SBK_FROM_MA(maResult) ((sbk_status)(maResult))
#define MA_FROM_SBK(sbkStatus) ((ma_result)(sbkStatus))

#define SC_CHECK(condition, result) \
    if ((condition) == MA_FALSE)    \
    return (result)
#define SC_CHECK_STATUS(result) \
    if (((sbk_status)(result)) != SBK_SUCCESS) \
    return (result)
#define SC_CHECK_ARG(condition)  \
    if ((condition) == MA_FALSE) \
    return SBK_ERR_INVALID_PARAMETER
#define SC_CHECK_MEM(ptr) \
    if ((ptr) == NULL)    \
    return SBK_ERR_OUT_OF_MEMORY
#define SC_CHECK_AND_GOTO(condition, dest) \
    if ((condition) == MA_FALSE)           \
    goto dest

typedef struct sc_system            sc_system;
typedef struct sc_dsp               sc_dsp;

enum
{
    SC_STRING_NAME_LENGTH = 16
};

/**
 * @brief The different ways to create a sound.
 * 
 * These types are basically the same as @ref ma_sound_flags.
 * 
 * @see sc_sound_config_init_file
 * @see sc_sound_config_init_memory
 * @see sc_system_create_sound
 */
typedef enum sc_sound_mode
{
    SC_SOUND_MODE_DEFAULT   = 0x00000000,   //< Creates a sound in memory and decompresses during runtime
    SC_SOUND_MODE_DECODE    = 0x00000001,   //< Decodes the sound upon loading, instead of runtime
    SC_SOUND_MODE_ASYNC     = 0x00000002,   //< Loads the sound in a background thread
    SC_SOUND_MODE_STREAM    = 0x00000004,   //< Streams parts of the sound from disk during runtime
} sc_sound_mode;

/**
 * @brief Playback state of an @ref sc_voice.
 *
 * A voice moves through these states over its lifetime. Pause freezes the
 * play cursor but does not itself decide audibility; on resume the voice
 * runs the same virtualization check as @ref SC_VOICE_STATE_STARTING and
 * lands in either @ref SC_VOICE_STATE_PLAYING or @ref SC_VOICE_STATE_VIRTUAL
 * depending on the real-voice budget.
 *
 * Failures (async load errors, decoder errors, etc.) do not have their own
 * state; the voice transitions to @ref SC_VOICE_STATE_STOPPING and the slot
 * is returned to the pool. Subsequent use of a stale handle returns
 * @ref SBK_ERR_NOT_FOUND; check the logs or profiler for the
 * underlying cause.
 *
 * Transitions (rows = from, columns = to; dash = not allowed):
 *
 * | from \ to | STOPPED | STARTING | VIRTUAL | PLAYING | PAUSED | STOPPING |
 * | --------- | :-----: | :------: | :-----: | :-----: | :----: | :------: |
 * | STOPPED   |    -    |   play   |    -    |    -    |    -   |     -    |
 * | STARTING  |    -    |     -    |  limit  |  ready  |    -   |   stop   |
 * | VIRTUAL   |    -    |     -    |    -    |  devirt |  pause |   stop   |
 * | PLAYING   |    -    |     -    |   virt  |    -    |  pause | stop/eof |
 * | PAUSED    |    -    |     -    |  resume |  resume |    -   |   stop   |
 * | STOPPING  |   tail  |     -    |    -    |    -    |    -   |     -    |
 */
typedef enum sc_voice_state
{
    SC_VOICE_STATE_STOPPED,     //< Idle. The slot is free or has just been returned to the pool.
    SC_VOICE_STATE_STARTING,    //< Play requested; waiting on async load or first render before becoming @ref SC_VOICE_STATE_PLAYING or @ref SC_VOICE_STATE_VIRTUAL.
    SC_VOICE_STATE_VIRTUAL,     //< Cursor advancing but not rendered; culled by the real-voice budget.
    SC_VOICE_STATE_PLAYING,     //< Cursor advancing and mixed into the output.
    SC_VOICE_STATE_PAUSED,      //< Cursor frozen. On resume, virtualization decides @ref SC_VOICE_STATE_PLAYING vs @ref SC_VOICE_STATE_VIRTUAL.
    SC_VOICE_STATE_STOPPING     //< Tail/fade-out in progress; transitions to @ref SC_VOICE_STATE_STOPPED when done.
} sc_voice_state;

/**
 * @brief Built-in DSP types.
 * 
 * It is expected that user DSP types would have numbers higher than SC_DSP_TYPE_COUNT
 * example: MY_DSP_TYPE = SC_DSP_TYPE_COUNT + 1
 * The system can then store user DSP descriptions and find them by "handle"
 */
typedef enum sc_dsp_type
{
    SC_DSP_TYPE_UNKNOWN,
    SC_DSP_TYPE_FADER,
    SC_DSP_TYPE_LOWPASS,
    SC_DSP_TYPE_HIGHPASS,
    SC_DSP_TYPE_DELAY,
    SC_DSP_TYPE_METER,
    SC_DSP_TYPE_CLAP,           //< Wraps a CLAP plugin
    SC_DSP_TYPE_COUNT           //< Count of types. SC_DSP_TYPE_COUNT - 1 == number of built in types
} sc_dsp_type;

/**
 * @brief Predefined positions to insert DSP units into a @ref sc_node_group.
 * 
 * Values are negative so positive index values always refer to a specific position in the node group.
 */
typedef enum sc_dsp_index
{
    SC_DSP_INDEX_TAIL = -2,  //< Left/back of the chain and becomes the new input
    SC_DSP_INDEX_HEAD = -1   //< Right/top of the chain and becomes the new output
} sc_dsp_index;

/**
 * @brief Encoding formats that Sound Chef supports.
 * 
 * Extends @ref ma_encoding_format.
 */
typedef enum sc_encoding_format
{
    sc_encoding_format_unknown = 0,
    sc_encoding_format_wav,
    sc_encoding_format_adpcm = 10,
    sc_encoding_format_vorbis,
    sc_encoding_format_opus
} sc_encoding_format;

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
typedef struct sc_node_group
{
    sc_dsp* tail;   //< Left most node. Sounds and child groups connect to this
    sc_dsp* fader;  //< Controls the volume and more of the group. Exists at start
    sc_dsp* head;   //< Right/top most node. Nodes in the group route to this. The head then outputs to a parent
} sc_node_group;

/**
 * @brief DSP units that can fit into node groups. Extensions of ma_node types with extra information.
 * 
 * DSP units are created with @ref sc_dsp_description objects and allow for simpler creation of DSP units than ma_node types.
 */
struct sc_dsp
{
    sc_uint32       handle;         //< Either a sc_dsp_type or a user type
    ma_node*        node;           //< The dynamically allocated node that can process audio
    sc_system*      system;         //< Owning system
    sc_node_group*  groupOwner;     //< Owning node group. Can be null
    sc_dsp*         next;           //< when in a node group, the parent/next dsp. Can be null if the head node
    sc_dsp*         prev;           //< when in a node group, the child/previous dsp. Can be null if the tail node
};

typedef enum sc_dsp_parameter_type
{
    sc_dsp_parameter_type_float
} sc_dsp_parameter_type;

typedef struct sc_dsp_parameter_float
{
    float min;
    float max;
    float value;
} sc_dsp_parameter_float;

typedef struct sc_dsp_parameter
{
    sc_dsp_parameter_type type;
    char name[SC_STRING_NAME_LENGTH];

    union
    {
        sc_dsp_parameter_float floatParameter;
    };
} sc_dsp_parameter;

typedef sbk_status(SC_CALL* sc_dsp_create_proc)(sc_system* system, sc_dsp* dsp, const void* userData);
typedef sbk_status(SC_CALL* sc_dsp_release_proc)(sc_system* system, sc_dsp* dsp);
typedef sbk_status(SC_CALL* sc_dsp_is_idle_proc)(sc_dsp* dsp, sc_bool* outIsIdle);
typedef sbk_status(SC_CALL* sc_dsp_set_param_float_proc)(sc_dsp* dsp, sc_uint32 index, float value);
typedef sbk_status(SC_CALL* sc_dsp_get_param_float_proc)(sc_dsp* dsp, sc_uint32 index, float* value);

/**
 * @brief Structure to create, destroy, and update DSP units of a specific handle.
 */
typedef struct sc_dsp_description
{
    sc_dsp_create_proc          create;             //< Allocates and initializes the ma_node that will handle the DSP processing
    sc_dsp_release_proc         release;
    sc_dsp_is_idle_proc         isIdle;             //< Optional: For delays with feedback, this is used to detect if the delay has gone silent and the voice can be ended
    sc_dsp_set_param_float_proc setFloat;
    sc_dsp_get_param_float_proc getFloat;

    const sc_dsp_parameter**    params;
    sc_uint32                   numParams;
} sc_dsp_description;

typedef struct sc_dsp_config
{
    sc_uint32                       handle;         //< Type/handle/index
    const sc_dsp_description*       dspDescription; //< Required: holds a vtable for DSP creation and deletion
    const clap_plugin_factory_t*    clapFactory;    //< Optional: passed to CLAP DSP types so a specific CLAP plugin can be created
} sc_dsp_config;

/**
 * @brief Basic piece of playing audio.
 * 
 * sc_sound is currently being deprecated in favour of @ref sc_voice.
 */
typedef struct sc_sound
{
    ma_sound        sound;
    sc_sound_mode   mode;
    ma_decoder*     memoryDecoder;
    sc_system*      owningSystem;
} sc_sound;

typedef struct sc_sound sc_sound_instance;  //< Sound instances are just sounds. The resource is not copied but reference counted

/**
 * @brief Configuration for creating a @ref sc_sound.
 *
 * Exactly one of @ref filePath or @ref memory must be set. When @ref memory is set,
 * @ref memorySize must be the size of that buffer in bytes.
 *
 * @see sc_sound_config_init_file
 * @see sc_sound_config_init_memory
 * @see sc_system_create_sound
 */
typedef struct sc_sound_config
{
    const char*     filePath;       //< File path to load the sound from. Mutually exclusive with @ref memory
    const void*     memory;         //< In-memory sound data. Mutually exclusive with @ref filePath
    size_t          memorySize;     //< Size in bytes of the @ref memory buffer
    sc_sound_mode   mode;           //< Sound loading/playback mode
} sc_sound_config;

/**
 * @brief Holds a DLL handle and plugin entry for a CLAP plugin.
 */
typedef struct sc_clap
{
    ma_handle                       dynamicLibraryHandle;   //< Handle to the .clap file
    clap_plugin_entry_t*            clapEntry;              //< Entry point of the plugin
    const clap_plugin_factory_t*    pluginFactory;          //< Plugin factory to poll and create plugins from
} sc_clap;

/**
 * @brief A voice is a window into a single sound that may or may not be audible and rendering.
 * 
 * Voices are limited by the @ref sc_system_config::maxVoices value passed during system initialization.
 */
typedef struct sc_voice
{
    sc_system*                      system;
    sc_uint64                       playCursor;             //< Audio thread
    sc_atomic_uint8                 currentState;
    sc_atomic_uint8                 desiredState;
    sc_atomic_float                 gain;
    sc_atomic_float                 pitch;
    sc_uint8                        priority;               //< priority where the greater the number, the great the priority. Generally 0-100
    sc_node_group*                  group;
    sc_voice_handle                 realVoiceHandle;        //< Non-owning reference to a real voice
} sc_voice;

/**
 * @brief A real voice that is connected to the DSP graph.
 * 
 * Real voices are limited by the @ref sc_system_config::maxRealVoices value passed during system initialization.
 */
typedef struct sc_voice_real
{
    ma_node_base                    baseNode;
    sc_voice*                       voiceRef;       //< The voice we are playing for
    sc_sound_mode                   mode;
    ma_decoder*                     memoryDecoder;  //< @todo Work out if this should go in a resource manager
} sc_voice_real;

/**
 * @brief Object that manages the node graph, sounds, output etc.
 *
 * The sc_system is a wrapper for the @ref ma_engine from miniaudio.
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
    ma_engine                   engine;                                                 //< Must stay first for miniaudio node API
    ma_resource_manager         resourceManager;                                        //< We need a custom resource manager for custom decoders
    ma_log                      log;

    clap_host_t                 clapHost;
    sc_clap*                    clapPlugins;                                            //< CLAP plugins loaded from systemConfig->pluginPath, or NULL if none
    ma_uint32                   clapPluginCount;                                        //< Number of entries in clapPlugins
    float                       clapPluginScratch[SC_MAX_CHANNELS][SC_MAX_FRAME_COUNT]; //< CLAP plugins process deinterleaved audio. miniaudio processes interleaved. We need a space for CLAP plugins to output to, then can interleave it
    float*                      clapPluginChannels[SC_MAX_CHANNELS];                    //< CLAP processing expects pointers for each channel

    sc_node_group*              masterNodeGroup;

    const sc_dsp_description*   userDspRegistry[SC_MAX_USER_DSP_TYPES];                 //< DSP descriptions to create each DSP handle

    ma_slot_allocator           voiceSlotAllocator;                                     //< Allocates voice handles
    sc_voice*                   voiceBuffer;                                            //< Allocated voices. Indexed through the @r voiceSlotAllocator

    ma_slot_allocator           realVoiceSlotAllocator;                                 //< Allocates handles for real voices
    sc_voice_real*              realVoiceBuffer;                                        //< Allocated real voices. Indexed through @r realVoiceSlotAllocator
};

/**
 * @brief Configuration for initializing the sc_system.
 * @see sc_system_init
 */
typedef struct sc_system_config
{
    const char*                 pluginPath;             //< Folder path containing CLAP plugins to load
    ma_allocation_callbacks     allocationCallbacks;    //< External allocation callbacks to override all memory allocation in Sound Chef
    ma_device_data_proc         dataCallback;           //< Device render callback. Overriden in Sound Bakery for profiling
    sc_uint32                   maxVoices;              //< Max number of voices (both virtual and real)
    sc_uint32                   maxRealVoices;          //< Max number of real voices to mix at once. Voices over this limit get virtualized
} sc_system_config;

#ifdef __cplusplus
}
#endif

#endif