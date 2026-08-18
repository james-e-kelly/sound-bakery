#ifndef SOUND_CHEF_H
#define SOUND_CHEF_H

/**
 * @file
 * @brief A wrapper library for miniaudio that emulates functionality of FMOD.
 *
 * The low level engine powering Sound Bakery.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "sound_chef/sound_chef_version.h"

#ifdef sound_chef_shared_EXPORTS
    #define SC_DLL
    #define MA_DLL
#endif

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

#ifdef __cplusplus
    #define SC_ALIGN_TO(x) alignas(x)
#else
    #define SC_ALIGN_TO(x) _Alignas(x)
#endif

// CPU pause hint for short spin loops. Not a scheduler yield
#if defined(_MSC_VER)
    #include <intrin.h>
    #define SC_PAUSE() _mm_pause()
#elif defined(__i386__) || defined(__x86_64__)
    #define SC_PAUSE() __asm__ __volatile__("pause")
#elif defined(__aarch64__) || defined(__arm__)
    #define SC_PAUSE() __asm__ __volatile__("yield")
#else
    #define SC_PAUSE() ((void)0)
#endif

/**
 * @def Sound Chef supports a max of 5th order ambisonics.
 * 
 * miniaudio can handle many more channels but Sound Chef is not a general purpose library.
 */
#define SC_MAX_CHANNELS         36

/**
* @def Some operations need an audio buffer on the heap (mainly CLAP processing).
*
* This gives us a reasonable buffer size.
*/
#define SC_MAX_FRAME_COUNT      2048

/**
* @def Max user-defined DSP types.
*
* sc_dsp_descriptions are kept in a static/preallocated array. Increase this if we need to support more DSP types.
* 
* It's assumed most users add no additional DSP types and those who do are adding only a few.
*/
#define SC_MAX_USER_DSP_TYPES   16

#include <assert.h>

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
        
typedef ma_uint8                sc_uint8;
typedef ma_uint16               sc_uint16;
typedef ma_bool32               sc_bool;
typedef ma_uint32               sc_uint32;
typedef ma_int32                sc_int32;
typedef ma_int64                sc_int64;
typedef ma_uint64               sc_uint64;

typedef void*                   sc_handle;

typedef struct sc_system        sc_system;
typedef struct sc_dsp           sc_dsp;
typedef struct sc_node_group    sc_node_group;

#define SBK_FALSE 0
#define SBK_TRUE 1

#define SC_COUNTOF(x)            (sizeof(x) / sizeof(x[0]))
#define SC_MAX(x, y)             (((x) > (y)) ? (x) : (y))
#define SC_MIN(x, y)             (((x) < (y)) ? (x) : (y))
#define SC_ABS(x)                (((x) > 0) ? (x) : -(x))
#define SC_CLAMP(x, lo, hi)      (SC_MAX(lo, SC_MIN(x, hi)))
#define SC_OFFSET_PTR(p, offset) (((sc_uint8*)(p)) + (offset))
#define SC_ALIGN(x, a)           (((x) + ((a)-1)) & ~((a)-1))
#define SC_ALIGN_64(x)           SC_ALIGN(x, 8)

/**************************************************************************************************************************************************************

Statuses And Error Handling

**************************************************************************************************************************************************************/

/**
 * @brief Status codes.
 * 
 * Extends the miniaudio @ref ma_result enum.
 * 
 * miniaudio defines success == 0 and errors less than 0.
 * Sound Chef/Bakery defines success == 0 and errors greater than 0.
 * 
 * Statuses are influenced by http status codes where codes are grouped.
 * Users don't need to know every code, but know that anything between 101-200 is an error from Sound Chef.
 */
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

#define SBK_FROM_MA(maResult)   ((sbk_status)(maResult))
#define MA_FROM_SBK(sbkStatus)  ((ma_result)(sbkStatus))

#define SC_CHECK(condition, result) \
    if ((condition) == SBK_FALSE)    \
    return (result)
#define SC_CHECK_STATUS(result) \
    if (((sbk_status)(result)) != SBK_SUCCESS) \
    return (result)
#define SC_CHECK_ARG(condition)  \
    if ((condition) == SBK_FALSE) \
    return SBK_ERR_INVALID_PARAMETER
#define SC_CHECK_MEM(ptr) \
    if ((ptr) == NULL)    \
    return SBK_ERR_OUT_OF_MEMORY
#define SC_CHECK_AND_GOTO(condition, dest) \
    if ((condition) == SBK_FALSE)           \
    goto dest

/**************************************************************************************************************************************************************

Creation And Deltion Macros

**************************************************************************************************************************************************************/

#include <string.h>

/**
 * @def Zeroes the memory at @p ptr.
 */
#define SC_ZERO_OBJECT(ptr) memset((ptr), 0, sizeof(*(ptr)))

/**
 * @def Creates an object of type @p type, zeroes the memory, and returns on errors.
 *
 * Example:
 * @code
 *  sc_dsp* dsp = NULL;
 *  SC_CREATE(dsp, sc_dsp, system);
 * @endcode
 */
#define SC_CREATE(ptr, type, system)                                                   \
    do                                                                                 \
    {                                                                                  \
        SC_CHECK_ARG((system) != NULL);                                                \
        (ptr) = (type*)ma_calloc(sizeof(type), &(system)->engine.allocationCallbacks); \
        SC_CHECK_MEM((ptr));                                                           \
    } while (0)

/**
 * @def Frees the memory at @p ptr.
 */
#define SC_FREE(ptr, system)                                   \
    do                                                         \
    {                                                          \
        assert((system) != NULL);                              \
        ma_free((ptr), &(system)->engine.allocationCallbacks); \
        (ptr) = NULL;                                          \
    } while (0)

/**************************************************************************************************************************************************************

Atomics

**************************************************************************************************************************************************************/

#include "c89atomic.h"

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

/**************************************************************************************************************************************************************

CLAP

**************************************************************************************************************************************************************/

#include "clap/clap.h"

/**
 * @brief Holds a DLL handle and plugin entry for a CLAP plugin.
 *
 * The system loads CLAP plugins from a directory and stores pointers to the plugin structures.
 */
typedef struct sc_clap
{
    sc_handle dynamicLibraryHandle;              //< Handle to the .clap file
    clap_plugin_entry_t* clapEntry;              //< Entry point of the plugin
    const clap_plugin_factory_t* pluginFactory;  //< Plugin factory to poll and create plugins from
} sc_clap;

sbk_status SC_API sc_clap_load(const char* clapFilePath, sc_clap* clapPlugin);
sbk_status SC_API sc_clap_unload(sc_clap* clapPlugin);

/**************************************************************************************************************************************************************

Encoding

**************************************************************************************************************************************************************/

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

typedef struct sc_encoder_config
{
    ma_encoder_config   baseConfig;
    ma_uint8            quality;        //< Quality setting for formats that allow it
    sc_encoding_format  encodingFormat;
} sc_encoder_config;

typedef struct sc_encoder
{
    ma_encoder          baseEncoder;
    sc_encoder_config   config;
} sc_encoder;

sc_encoder_config SC_API sc_encoder_config_init(sc_encoding_format encodingFormat, ma_format format, ma_uint32 channels, ma_uint32 sampleRate, ma_uint8 quality);

sbk_status SC_API sc_encoder_init(ma_encoder_write_proc onWrite, ma_encoder_seek_proc onSeek, void* userData, const sc_encoder_config* config, sc_encoder* encoder);
sbk_status SC_API sc_encoder_init_file(const char* filePath, const sc_encoder_config* config, sc_encoder* encoder);
sbk_status SC_API sc_encoder_uninit(sc_encoder* encoder);

sbk_status SC_API sc_encoder_write_pcm_frames(sc_encoder* encoder, const void* framesIn, ma_uint64 frameCount, ma_uint64* framesWritten);
sbk_status SC_API sc_encoder_write_from_file(const char* decodeFilePath, const char* encodeFilePath, const sc_encoder_config* config);

ma_result SC_API sc_encoder_vorbis_on_init(ma_encoder* encoder);
void SC_API sc_encoder_vorbis_on_uninit(ma_encoder* encoder);
ma_result SC_API sc_encoder_vorbis_write_pcm_frames(ma_encoder* encoder, const void* framesIn, ma_uint64 frameCount, ma_uint64* framesWritten);

/**************************************************************************************************************************************************************

DSP

**************************************************************************************************************************************************************/

/**
 * @def Sample values under this threshold are silenced. DSP uses this to calculate when reverb/delay echos finish. 
 */
#define SC_DELAY_SILENCE_THRESHOLD 0.0001F

enum
{
    SC_STRING_NAME_LENGTH = 16
};

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
    SC_DSP_TYPE_CLAP,  //< Wraps a CLAP plugin
    SC_DSP_TYPE_COUNT  //< Count of types. SC_DSP_TYPE_COUNT - 1 == number of built in types
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
 * @brief DSP units that connect and form chains within @ref sc_node_group. Extensions of ma_node types with extra information.
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

/**
 * @brief 
 * 
 * DSP units are created with @ref sc_dsp_description objects.
 * The sc_system stores/knows about two description arrays.
 * One is internal and lets users create units using the @ref sc_dsp_type.
 * The other is external and lets users create units with a custom type.
 * 
 * The support creating internal and external units, all descriptions are looked up by a handle.
 * If the handle is between 0 and SC_DSP_TYPE_COUNT, the handle is used to index the internal description array.
 * If the handle is greater than SC_DSP_TYPE_COUNT, SC_DSP_TYPE_COUNT is subtracted from the handle and used as the index into the external description array.
 */
typedef struct sc_dsp_config
{
    sc_uint32                           handle;         //< Type/handle/index
    const sc_dsp_description*           dspDescription; //< Required: holds a vtable for DSP creation and deletion
    const clap_plugin_factory_t*        clapFactory;    //< Optional: passed to CLAP DSP types so a specific CLAP plugin can be created
} sc_dsp_config;

enum
{
    SC_DSP_LOWPASS_PARAM_CUTOFF,
    SC_DSP_LOWPASS_PARAM_COUNT
};

enum
{
    SC_DSP_HIGHPASS_PARAM_CUTOFF,
    SC_DSP_HIGHPASS_PARAM_COUNT
};

enum
{
    SC_DSP_DELAY_PARAM_DELAY_SECONDS,
    SC_DSP_DELAY_PARAM_DRY,
    SC_DSP_DELAY_PARAM_WET,
    SC_DSP_DELAY_PARAM_FEEDBACK,
    SC_DSP_DELAY_PARAM_COUNT
};

typedef enum sc_dsp_meter_query
{
    SC_DSP_METER_QUERY_PEAK,
    SC_DSP_METER_QUERY_RMS,
    SC_DSP_METER_QUERY_COUNT
} sc_dsp_meter_query;

enum
{
    SC_DSP_METER_MAX_CHANNELS = SC_MAX_CHANNELS
};

typedef struct sc_meter
{
    sc_atomic_uint32 channels;
    sc_atomic_float peakLevels[SC_DSP_METER_MAX_CHANNELS];
    sc_atomic_float rmsLevels[SC_DSP_METER_MAX_CHANNELS];
} sc_meter;

typedef struct sc_meter_node
{
    ma_node_base baseNode;
    sc_meter meter;
} sc_meter_node;

typedef struct sc_clap_node
{
    ma_node_base baseNode;
    const clap_plugin_t* clapPlugin;
    sc_bool isProcessing;  //< Whether start_processing() has been called on the plugin.
} sc_clap_node;

typedef struct
{
    ma_uint32 channels;
    ma_uint32 sampleRate;
    ma_uint32 delayInFrames;
    ma_uint32 maxDelayInFrames;
    float dry;       //< Dry signal gain (0 - 1). Defaults to 1
    float wet;       //< Wet signal gain (0 - 1). Defaults to 0 (no delay)
    float feedback;  //< Feedback signal gain (0 - 1). Defaults to 0 (no feedback)
} sc_delay_config;

sc_delay_config SC_API sc_delay_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames);

typedef struct
{
    sc_delay_config config;
    ma_uint32       writeCursor;
    ma_uint32       bufferSizeInFrames; //< Total buffer size. Not the delay time/size
    float*          buffer;
    ma_uint32       silentFrameCount;   //< Audio thread. Counts number of silent frames so we know when are idle
    sc_atomic_bool  isIdle;
} sc_delay;

sbk_status SC_API sc_delay_init(const sc_delay_config* config, const ma_allocation_callbacks* allocationCallbacks, sc_delay* delay);
void SC_API sc_delay_uninit(sc_delay* delay, const ma_allocation_callbacks* allocationCallbacks);
sbk_status SC_API sc_delay_process_pcm_frames(sc_delay* delay, void* framesOut, const void* framesIn, ma_uint32 frameCount);
sbk_status SC_API sc_delay_set_delay_ms(sc_delay* delay, float value);
sbk_status SC_API sc_delay_get_delay_ms(const sc_delay* delay, float* outValue);
sbk_status SC_API sc_delay_set_wet(sc_delay* delay, float value);
sbk_status SC_API sc_delay_get_wet(const sc_delay* delay, float* outValue);
sbk_status SC_API sc_delay_set_dry(sc_delay* delay, float value);
sbk_status SC_API sc_delay_get_dry(const sc_delay* delay, float* outValue);
sbk_status SC_API sc_delay_set_feedback(sc_delay* delay, float value);
sbk_status SC_API sc_delay_get_feedback(const sc_delay* delay, float* outValue);

typedef struct
{
    ma_node_config nodeConfig;
    sc_delay_config delayConfig;
} sc_delay_node_config;

sc_delay_node_config SC_API sc_delay_node_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames);

typedef struct
{
    ma_node_base baseNode;
    sc_delay delay;
} sc_delay_node;

sbk_status SC_API sc_delay_node_init(ma_node_graph* pNodeGraph, const sc_delay_node_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, sc_delay_node* pDelayNode);
void SC_API sc_delay_node_uninit(sc_delay_node* pDelayNode, const ma_allocation_callbacks* pAllocationCallbacks);

sc_dsp_config SC_API sc_dsp_config_init(const sc_dsp_description* description);
sc_dsp_config SC_API sc_dsp_config_init_type(const sc_system* system, sc_dsp_type type);
sc_dsp_config SC_API sc_dsp_config_init_handle(const sc_system* system, sc_uint32 handle);
sc_dsp_config SC_API sc_dsp_config_init_clap(const sc_system* system, const clap_plugin_factory_t* pluginFactory);

sbk_status SC_API sc_dsp_get_parameter_float(sc_dsp* dsp, sc_uint32 index, float* value);
sbk_status SC_API sc_dsp_set_parameter_float(sc_dsp* dsp, sc_uint32 index, float value);
sbk_status SC_API sc_dsp_get_metering_info(sc_dsp* dsp, ma_uint32 channelIndex, sc_dsp_meter_query meterType, float* value);
sbk_status SC_API sc_dsp_release(sc_dsp* dsp);

/**************************************************************************************************************************************************************

Node Group

**************************************************************************************************************************************************************/

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

sbk_status SC_API sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent);

/**
 * @brief Routes the group's output directly to the graph endpoint (the audio device).
 */
sbk_status SC_API sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup);

/**
 * @brief Finds the first DSP in the group whose handle matches @p type.
 *
 * @return SBK_SUCCESS if the DSP was found and @p dsp is valid.
 * @return SBK_ERR_NOT_FOUND if the DSP was not found.
 */
sbk_status SC_API sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp);

/**
 * @brief Adds a DSP to the node group chain.
 * @remark The node group owns the DSP after this point. All DSP are released during @ref sc_node_group_release.
 */
sbk_status SC_API sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index);

/**
 * @brief Releases the node group and all DSP within it.
 */
sbk_status SC_API sc_node_group_release(sc_node_group* nodeGroup);

/**************************************************************************************************************************************************************

Sound

**************************************************************************************************************************************************************/

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
    SC_SOUND_MODE_DEFAULT = 0x00000000,  //< Creates a sound in memory and decompresses during runtime
    SC_SOUND_MODE_DECODE  = 0x00000001,  //< Decodes the sound upon loading, instead of runtime
    SC_SOUND_MODE_ASYNC   = 0x00000002,  //< Loads the sound in a background thread
    SC_SOUND_MODE_STREAM  = 0x00000004,  //< Streams parts of the sound from disk during runtime
} sc_sound_mode;

/**
 * @brief Basic piece of playing audio.
 * 
 * sc_sound is currently being deprecated in favour of @ref sc_voice.
 * 
 * Sounds are intended to just be a loaded audio buffer.
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

typedef sc_sound_config sc_voice_config;    // Make the voice config just a sound config until it becomes its own thing

sc_sound_config SC_API sc_sound_config_init_file(const char* filePath, sc_sound_mode mode);
sc_sound_config SC_API sc_sound_config_init_memory(const void* memory, size_t memorySize, sc_sound_mode mode);

sbk_status SC_API sc_sound_get_length(sc_sound* sound, float* lengthInSeconds);
sbk_status SC_API sc_sound_release(sc_sound* sound);

sbk_status SC_API sc_sound_instance_is_playing(sc_sound_instance* instance, sc_bool* isPlaying);
sbk_status SC_API sc_sound_instance_start(sc_sound_instance* instance);
sbk_status SC_API sc_sound_instance_pause(sc_sound_instance* instance);
sbk_status SC_API sc_sound_instance_get_cursor_in_seconds(sc_sound_instance* instance, float* seconds);
sbk_status SC_API sc_sound_instance_set_cursor_in_seconds(sc_sound_instance* instance, float seconds);
sbk_status SC_API sc_sound_instance_get_loop_position_in_seconds(sc_sound_instance* instance, float* seconds);
sbk_status SC_API sc_sound_instance_set_loop_position_in_seconds(sc_sound_instance* instance, float loopStartSeconds, float loopEndSeconds);
sbk_status SC_API sc_sound_instance_is_looping(sc_sound_instance* instance, sc_bool* looping);
sbk_status SC_API sc_sound_instance_set_looping(sc_sound_instance* instance, sc_bool looping);
sbk_status SC_API sc_sound_instance_release(sc_sound_instance* instance);

/**************************************************************************************************************************************************************

Voice

**************************************************************************************************************************************************************/

typedef sc_uint64 sc_voice_handle;    //< Voice handle. Contains both a reference count and an index
typedef sc_uint32 sc_voice_refcount;  //< References to this slot. Used to check if a handle is old/stale
typedef sc_uint32 sc_voice_slot;      //< Slot/index into the voice array

/**
 * @brief Playback lifecycle position of an @ref sc_voice.
 *
 * The state describes where the voice is in its lifecycle; audibility and
 * cursor progression are expressed as orthogonal flags:
 *   - @ref SC_VOICE_FLAG_PAUSED  - user paused; the audio callback freezes the play cursor.
 *   - @ref SC_VOICE_FLAG_VIRTUAL - system culled to stay under the real-voice budget; no mix.
 *
 * Callers only ever request @ref SC_VOICE_STATE_STOPPED or
 * @ref SC_VOICE_STATE_PLAYING on the desired word. @ref SC_VOICE_STATE_STARTING
 * and @ref SC_VOICE_STATE_STOPPING are pump-owned transients.
 *
 * Failures (async load errors, decoder errors, etc.) do not have their own
 * state; the voice transitions to @ref SC_VOICE_STATE_STOPPING and the slot
 * is returned to the pool. Subsequent use of a stale handle returns
 * @ref SBK_ERR_NOT_FOUND; check the logs or profiler for the
 * underlying cause.
 *
 * Transitions the pump drives (rows = current, columns = desired; dash = not allowed):
 *
 * | current \ desired | STOPPED  | PLAYING |
 * | ----------------- | :------: | :-----: |
 * | STOPPED           |    -     |    -    |  play requests enter via sc_system_play_sound_voice (→ STARTING)
 * | STARTING          |  cancel  |  ready  |
 * | PLAYING           |   tail   |    -    |  paused/virtual are flag flips; no state change
 * | STOPPING          |  drain   |  wins   |  tail runs to completion even if a new play arrives
 */
typedef enum sc_voice_state
{
    SC_VOICE_STATE_STOPPED,   //< Idle. The slot is free or has just been returned to the pool.
    SC_VOICE_STATE_STARTING,  //< Play requested; waiting on async load or first render before becoming @ref SC_VOICE_STATE_PLAYING.
    SC_VOICE_STATE_PLAYING,   //< Live. Audibility is governed by @ref SC_VOICE_FLAG_VIRTUAL, cursor by @ref SC_VOICE_FLAG_PAUSED.
    SC_VOICE_STATE_STOPPING   //< Tail/fade-out in progress; transitions to @ref SC_VOICE_STATE_STOPPED when done.
} sc_voice_state;

// State + flags are packed into one 32-bit atomic word so callers see the
// whole voice status in one load. Layout:
//   bits  0..7  : sc_voice_state (exclusive lifecycle position)
//   bits  8..31 : sc_voice_flags (orthogonal modifiers, OR-able)
//
// Any write that changes only the state must preserve the flag bits (and vice
// versa) - use sc_voice_word_with_state / sc_voice_word_with_flags to build
// the replacement word, and CAS to install it.
#define SC_VOICE_STATE_MASK 0x000000FFu
#define SC_VOICE_FLAGS_MASK 0xFFFFFF00u

// Reserve the low 8 bits for the state. Flag values must start at bit 8.
// PAUSED lives on the desired word (user request). VIRTUAL lives on the current
// word (system decision based on the real-voice budget). Everything else is
// evaluated per-flag when the pump reads its word.
typedef enum sc_voice_flags
{
    SC_VOICE_FLAG_NONE    = 0,
    SC_VOICE_FLAG_PAUSED  = 1u << 8,  //< Freeze the play cursor. Set by the user via sc_voice_pause.
    SC_VOICE_FLAG_VIRTUAL = 1u << 9,  //< Not connected to a real voice; audio callback skips the mix.
    SC_VOICE_FLAG_FADING  = 1u << 10  //< A volume ramp is in progress (start/stop fade, ducking, etc.).
} sc_voice_flags;

/**
 * @brief A voice is a window into a single sound that may or may not be audible and rendering.
 * 
 * Voices are limited by the @ref sc_system_config::maxVoices value passed during system initialization.
 */
typedef struct sc_voice
{
    sc_system*                      system;
    sc_sound*                       sound;                  //< Source to read from
    sc_uint64                       playCursor;             //< Audio thread
    MA_ATOMIC(8, sc_atomic_uint64)  handle;                 //< Handle for this slot's current occupant. Stale-handle callers compare against this and get SBK_ERR_NOT_FOUND on mismatch.
    MA_ATOMIC(4, sc_atomic_uint32)  currentStateAndFlags;
    MA_ATOMIC(4, sc_atomic_uint32)  desiredStateAndFlags;
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

static MA_INLINE sc_uint32 sc_voice_word_make(sc_voice_state state, sc_uint32 flags)
{
    return ((sc_uint32)state & SC_VOICE_STATE_MASK) | (flags & SC_VOICE_FLAGS_MASK);
}

static MA_INLINE sc_voice_state sc_voice_word_state(sc_uint32 word)
{
    return (sc_voice_state)(word & SC_VOICE_STATE_MASK);
}

static MA_INLINE sc_uint32 sc_voice_word_flags(sc_uint32 word)
{
    return word & SC_VOICE_FLAGS_MASK;
}

static MA_INLINE sc_uint32 sc_voice_word_with_state(sc_uint32 word, sc_voice_state state)
{
    return (word & SC_VOICE_FLAGS_MASK) | ((sc_uint32)state & SC_VOICE_STATE_MASK);
}

static MA_INLINE sc_uint32 sc_voice_word_with_flags(sc_uint32 word, sc_uint32 flags)
{
    return (word & SC_VOICE_STATE_MASK) | (flags & SC_VOICE_FLAGS_MASK);
}

static MA_INLINE sc_bool sc_voice_word_has_flag(sc_uint32 word, sc_voice_flags flag)
{
    return (word & (sc_uint32)flag) != 0 ? SBK_TRUE : SBK_FALSE;
}

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

sbk_status SC_API sc_voice_pause(sc_system* system, sc_voice_handle handle);
sbk_status SC_API sc_voice_resume(sc_system* system, sc_voice_handle handle);
sbk_status SC_API sc_voice_stop(sc_system* system, sc_voice_handle handle);

/**
 * @brief Returns whether the voice is currently paused (@ref SC_VOICE_FLAG_PAUSED).
 *
 * Reads the desired-side flag, so it reflects the caller's last pause/resume
 * request without waiting for the audio pump.
 * Returns @ref SBK_ERR_NOT_FOUND if @p handle is stale.
 */
sbk_status SC_API sc_voice_get_paused(sc_system* system, sc_voice_handle handle, sc_bool* outPaused);

/**
 * @brief Returns whether the voice is currently virtualised (@ref SC_VOICE_FLAG_VIRTUAL).
 *
 * Reads the current-side flag: SBK_TRUE means the pump has no real voice
 * mixing this @ref sc_voice right now (culled by the real-voice budget).
 */
sbk_status SC_API sc_voice_get_virtual(sc_system* system, sc_voice_handle handle, sc_bool* outVirtual);

/**************************************************************************************************************************************************************

Bank

**************************************************************************************************************************************************************/

#define SC_BANK_VERSION 1u

#define FOURCC(a, b, c, d) ((sc_uint32)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

#define SC_BANK_ID             (FOURCC('S', 'C', 'B', 'K'))
#define SC_BANK_AUDIO_CHUNK_ID (FOURCC('S', 'C', 'A', 'C'))
#define SC_BANK_SUB_ID         (FOURCC('S', 'C', 'F', 'E'))

typedef struct sc_subChunk sc_subChunk;
typedef struct sc_riffChunk sc_riffChunk;
typedef struct sc_audioChunk sc_audioChunk;

enum
{
    SC_BANK_FILE_NAME_BUFFER_SIZE = 64
};

struct sc_audioChunk
{
    ma_uint32 id;
    ma_uint32 size;
    char name[SC_BANK_FILE_NAME_BUFFER_SIZE];
    void* data;
};

struct sc_riffChunk
{
    ma_uint32 id;
    ma_uint32 size;
    ma_uint32 version;
    ma_uint32 numOfSubchunks;
    sc_audioChunk** subChunks;
};

typedef struct sc_bank
{
    sc_riffChunk* riff;      //< bank data. Filled upon reading
    ma_vfs_file outputFile;  //< bank file used during read and write
} sc_bank;

sbk_status SC_API sc_bank_init(sc_bank* bank, const char* outputFile, ma_open_mode_flags openFlags);
sbk_status SC_API sc_bank_uninit(sc_bank* bank);

sbk_status SC_API sc_bank_build(sc_bank* bank, const char** inputFiles, sc_encoding_format* inputFileFormats, ma_uint32 inputFilesSize);
sbk_status SC_API sc_bank_read(sc_bank* bank);

/**************************************************************************************************************************************************************

System

**************************************************************************************************************************************************************/

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
 * 
 * @remark When finished, system objects but must closed and then released.
 * @see sc_system_create
 * @see sc_system_release
 * @see sc_system_init
 * @see sc_system_close
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
 * @brief Sets up logging.
 * @remark Must be called before @ref sc_system_init.
 */
sbk_status SC_API sc_system_log_init(sc_system* system, ma_log_callback_proc logCallback);

sc_system_config SC_API sc_system_config_init_default();
sc_system_config SC_API sc_system_config_init(const char* pluginPath);

sbk_status SC_API sc_system_create(sc_system** outSystem);
sbk_status SC_API sc_system_init(sc_system* system, const sc_system_config* systemConfig);
sbk_status SC_API sc_system_close(sc_system* system);
sbk_status SC_API sc_system_release(sc_system* system);

/**
* @brief Updates the Sound Chef @ref sc_system on the calling thread.
* 
* @remark When using Sound Bakery, the system thread will call this function, thereby moving audio updates off the game thread (if in async mode).
*/
sbk_status SC_API sc_system_update(sc_system* system);

/**
* @brief Reads/pulls PCM data from the node graph and outputs them to @p framesOut.
*/
sbk_status SC_API sc_system_read_pcm_frames(sc_system* system, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead);

sbk_status SC_API sc_system_create_sound(sc_system* system, const sc_sound_config* config, sc_sound** sound);
sbk_status SC_API sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup);

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
* @param parent optional parameter. Outputs to the master node group by default
* @param paused whether this sound is paused upon creation or played instantly
*/
sbk_status SC_API sc_system_play_sound(sc_system* system, sc_sound* sound, sc_sound_instance** instance, sc_node_group* parent, sc_bool paused);

sbk_status SC_API sc_system_play_sound_voice(sc_system* system, sc_sound* sound, sc_voice_handle* outVoiceHandle, sc_node_group* parent, sc_bool paused);
sbk_status SC_API sc_system_stop_all_voices(sc_system* system);

sbk_status SC_API sc_system_clap_get_count(const sc_system* system, ma_uint32* count);

/**
 * @remark Memory is owned by the system. Do not free the pointer.
 */
sbk_status SC_API sc_system_clap_get_at(const sc_system* system, ma_uint32 index, sc_clap** plugin);

sbk_status SC_API sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp);

/**
* @brief Returns a pointer to the description for the given handle.
* @remark The description is owned by the system; do not free the returned pointer.
*/
sbk_status SC_API sc_system_get_dsp_desc(const sc_system* system, sc_uint32 handle, const sc_dsp_description** outDescription);

/**************************************************************************************************************************************************************

Utilities

**************************************************************************************************************************************************************/

static MA_INLINE sc_bool sc_is_pow2(size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

/**
 * @remark Returns 1 for x <= 1.
 */
static MA_INLINE size_t sc_next_pow2(size_t x)
{
    size_t v = 1;
    while (v < x)
    {
        v <<= 1;
    }
    return v;
}

ma_handle sc_dlopen(ma_log* pLog, const char* filename);
void sc_dlclose(ma_log* pLog, ma_handle handle);
ma_proc sc_dlsym(ma_log* pLog, ma_handle handle, const char* symbol);

SC_CLASS const char* SC_CALL sc_filename_get_ext(const char* filename);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H