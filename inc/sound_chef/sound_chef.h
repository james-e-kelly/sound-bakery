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
 * @brief Sound Chef supports a max of 5th order ambisonics.
 *
 * miniaudio can handle many more channels but Sound Chef is not a general purpose library.
 */
enum
{
    SC_MAX_CHANNELS = 36
};

/**
* @brief Some operations need an audio buffer on the heap (like CLAP processing).
*
* This gives us a reasonable buffer size.
*/
enum
{
    SC_MAX_FRAME_COUNT = 2048
};

/**
 * @brief Size of temp audio buffers allocated on the stack.
 */
enum
{
    SC_TEMP_STACK_BUFFER_SIZE = 4096
};

/**
* @brief Max user-defined DSP types.
*
* sc_dsp_descriptions are kept in a static/preallocated array. Increase this if we need to support more DSP types.
*
* It's assumed most users add no additional DSP types and those who do are adding only a few.
*/
enum
{
    SC_MAX_USER_DSP_TYPES = 16
};

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

#define SC_FALSE 0
#define SC_TRUE 1

#define SC_COUNTOF(x)            (sizeof(x) / sizeof(x[0]))
#define SC_MAX(x, y)             (((x) > (y)) ? (x) : (y))
#define SC_MIN(x, y)             (((x) < (y)) ? (x) : (y))
#define SC_ABS(x)                (((x) > 0) ? (x) : -(x))
#define SC_CLAMP(x, lo, hi)      (SC_MAX(lo, SC_MIN(x, hi)))
#define SC_OFFSET_PTR(p, offset) (((sc_uint8*)(p)) + (offset))
#define SC_ALIGN(x, a)           (((x) + ((a)-1)) & ~((a)-1))
#define SC_ALIGN_64(x)           SC_ALIGN(x, 8)

/**************************************************************************************************************************************************************

Naming Conventions

Sound Chef follows miniaudio's convention for enum member case:

  - SCREAMING_SNAKE_CASE for enum values that behave like `#define` constants:
    flag bitmasks (OR-able), status/result codes, and integer indices/counts.
    Examples: SC_VOICE_FLAG_PAUSED, SC_SOUND_MODE_STREAM, SBK_SUCCESS,
    SC_DSP_LOWPASS_PARAM_CUTOFF.

  - lowercase_snake_case for enum values that name one-of-many discrete kinds
    (a "type of" something). Examples: sc_encoding_format_wav,
    sc_dsp_parameter_type_float. This mirrors miniaudio's ma_format_f32,
    ma_channel_left, etc.

**************************************************************************************************************************************************************/

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
 *
 * @note The SBK_ prefix is intentional. Sound Bakery takes precedence over Sound Chef.
 */
typedef enum
{
    // < 0: miniaudio Errors
    SBK_SUCCESS = MA_SUCCESS,       //< Success

    // 1-100: User Errors
    SBK_ERR_USER = 1,               //< Generic user error
    SBK_ERR_INVALID_PARAMETER,      //< Invalid parameter given to the function
    SBK_ERR_ALREADY_INITIALIZED,    //< The resource was already initialized
    SBK_ERR_UNINITIALIZED,          //< The resource was not initialized
    SBK_ERR_INVALID_OPERATION,      //< Operation is unsupported in this state

    // 101-200: Sound Chef Errors
    SBK_ERR_CHEF = 101,             //< Generic Sound Chef error
    SBK_ERR_CHEF_UNINITIALIZED,

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

#define SC_STATUS_FROM_MA_RESULT(maResult)   ((sbk_status)(maResult))
#define SC_MA_RESULT_FROM_STATUS(sbkStatus)  ((ma_result)(sbkStatus))

/**
 * @brief Checks @p condition for success. On failure, returns @p status.
 */
#define SC_CHECK(condition, status) \
    if ((condition) == SC_FALSE)    \
    return (status)

/**
 * @brief Checks if @p status is equal to @p SBK_SUCCESS. On failure, returns @p status.
 */
#define SC_CHECK_STATUS(status) \
    if (((sbk_status)(status)) != SBK_SUCCESS) \
    return (status)

/**
 * @brief Checks if @p ptr is null. If it is, returns @p SBK_ERR_OUT_OF_MEMORY.
 */
#define SC_CHECK_ARG(condition)  \
    if ((condition) == SC_FALSE) \
    return SBK_ERR_INVALID_PARAMETER

/**
 * @brief Checks if @p ptr is null. If it is, returns @p SBK_ERR_OUT_OF_MEMORY.
 */
#define SC_CHECK_MEM(ptr) \
    if ((ptr) == NULL)    \
    return SBK_ERR_OUT_OF_MEMORY

/**
 * @brief Checks @p condition for success. On failure, goes to @p dest.
 */
#define SC_CHECK_AND_GOTO(condition, dest) \
    if ((condition) == SC_FALSE)           \
    goto dest

/**
 * @brief Checks a status for success. On failure, goes to @p dest.
 */
#define SC_CHECK_STATUS_ELSE_GOTO(condition, dest)  \
    if ((condition) != SBK_SUCCESS)                 \
    goto dest

/**************************************************************************************************************************************************************

Creation And Deletion Macros

**************************************************************************************************************************************************************/

#include <string.h>

#define SC_ZERO_MEMORY(ptr, size) memset((ptr), 0, (size))                  //< Zeroes the memory between @ref ptr and @ref ptr + @ref size
#define SC_ZERO_OBJECT(ptr)       SC_ZERO_MEMORY((ptr), sizeof(*(ptr)))     //< Zeroes the object memory pointed at by @ref ptr

/**
 * @brief Allocates memory and checks for errors and OOM.
 */
#define SC_CALLOC(ptr, size, system)                                      \
    do                                                                    \
    {                                                                     \
        SC_CHECK_ARG((size) > 0);                                         \
        SC_CHECK_ARG((system) != NULL);                                   \
        (ptr) = ma_malloc((size), &(system)->engine.allocationCallbacks); \
        SC_CHECK_MEM((ptr));                                              \
    } while (0)

/**
 * @brief Allocates memory, zeroes, and checks for errors and OOM.
 */
#define SC_CALLOC(ptr, size, system)                                        \
    do                                                                      \
    {                                                                       \
        SC_CHECK_ARG((size) > 0);                                           \
        SC_CHECK_ARG((system) != NULL);                                     \
        (ptr) = ma_calloc((size), &(system)->engine.allocationCallbacks);   \
        SC_CHECK_MEM((ptr));                                                \
    } while (0)

/**
 * @brief Creates an object of type @p type, zeroes the memory, and returns on errors.
 *
 * Example:
 * @code
 *  sc_dsp* dsp = NULL;
 *  SC_CREATE(dsp, sc_dsp, system);
 * @endcode
 */
#define SC_CREATE(ptr, type, system) SC_CALLOC((ptr), sizeof(type), (system))

/**
 * @brief Creates an object of @p type and goes to @p dest on failure.
 */
#define SC_CREATE_ELSE_GOTO(ptr, type, system, dest)                                \
    do                                                                              \
    {                                                                               \
        if(sizeof(type) <= 0) goto dest;                                            \
        if ((system) == NULL) goto dest;                                            \
        (ptr) = ma_calloc(sizeof(type), &(system)->engine.allocationCallbacks);     \
        if ((ptr) == NULL) goto dest;                                               \
    } while (0)

/**
 * @brief Frees the memory at @p ptr.
 */
#define SC_FREE(ptr, system)                                   \
    do                                                         \
    {                                                          \
        assert((system) != NULL);                              \
        ma_free((ptr), &(system)->engine.allocationCallbacks); \
        (ptr) = NULL;                                          \
    } while (0)

/**
 * @brief Creates an object of @p type and frees @p toFree on failure.
 */
#define SC_CREATE_ELSE_FREE(ptr, type, system, toFree)                          \
    do                                                                          \
    {                                                                           \
        if (sizeof(type) <= 0)                                                  \
        {                                                                       \
            SC_FREE((toFree), (system));                                        \
            return SBK_ERR_INVALID_PARAMETER;                                   \
        }                                                                       \
        if ((system) == NULL)                                                   \
        {                                                                       \
            SC_FREE((toFree), (system));                                        \
            return SBK_ERR_INVALID_PARAMETER;                                   \
        }                                                                       \
        (ptr) = ma_calloc(sizeof(type), &(system)->engine.allocationCallbacks); \
        if ((ptr) == NULL)                                                      \
        {                                                                       \
            SC_FREE((toFree), (system));                                        \
            return SBK_ERR_OUT_OF_MEMORY;                                       \
        }                                                                       \
    } while (0)

/**
 * @brief Calls a function. On failure, frees a pointer and returns the status.
 */
#define SC_CHECK_STATUS_ELSE_FREE(func, ptrToFree, system)      \
    do                                                          \
    {                                                           \
        sbk_status status = (func);                             \
        if (status != SBK_SUCCESS)                              \
        {                                                       \
            SC_FREE((ptrToFree), (system));                     \
            return status;                                      \
        }                                                       \
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

Encoding

**************************************************************************************************************************************************************/

/**
 * @brief Encoding formats that Sound Chef supports.
 * 
 * Extends @ref ma_encoding_format.
 */
typedef enum sc_encoding_format
{
    sc_encoding_format_unknown  = 0,
    sc_encoding_format_wav,
    sc_encoding_format_adpcm    = 10,
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

DSP Framework

**************************************************************************************************************************************************************/

enum
{
    SC_STRING_NAME_LENGTH = 16
};

/**
 * @brief Built-in DSP types.
 *
 * It is expected that user DSP types would have numbers higher than sc_dsp_type_count
 * example: my_dsp_type = sc_dsp_type_count + 1
 * The system can then store user DSP descriptions and find them by "handle"
 */
typedef enum sc_dsp_type
{
    sc_dsp_type_unknown,
    sc_dsp_type_fader,
    sc_dsp_type_lowpass,
    sc_dsp_type_highpass,
    sc_dsp_type_delay,
    sc_dsp_type_meter,
    sc_dsp_type_clap,       //< Wraps a CLAP plugin
    sc_dsp_type_count       //< Count of types. sc_dsp_type_count - 1 == number of built in types
} sc_dsp_type;

/**
 * @brief Predefined positions to insert DSP units into a @ref sc_node_group.
 *
 * Values are negative so positive index values always refer to a specific position in the node group.
 */
typedef enum sc_dsp_index
{
    sc_dsp_index_tail = -2,  //< Left/back of the chain and becomes the new input
    sc_dsp_index_head = -1   //< Right/top of the chain and becomes the new output
} sc_dsp_index;

/**
 * @brief DSP units that connect and form chains within @ref sc_node_group. Extensions of @ref ma_node types with extra information.
 *
 * DSP units are created with @ref sc_dsp_description objects and allow for simpler creation of DSP units than @ref ma_node types.
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
 * @brief Runtime configuration for creating a DSP unit.
 *
 * DSP units are created with @ref sc_dsp_description objects.
 * The sc_system stores/knows about two description arrays.
 * One is internal and lets users create units using the @ref sc_dsp_type.
 * The other is external and lets users create units with a custom type.
 *
 * To support creating internal and external units, all descriptions are looked up by a handle.
 * If the handle is between 0 and sc_dsp_type_count, the handle is used to index the internal description array.
 * If the handle is greater than sc_dsp_type_count, sc_dsp_type_count is subtracted from the handle and used as the index into the external description array.
 */
typedef struct sc_dsp_config
{
    sc_uint32                           handle;         //< Type/handle/index
    const sc_dsp_description*           dspDescription; //< Required: holds a vtable for DSP creation and deletion
    const clap_plugin_factory_t*        clapFactory;    //< Optional: passed to CLAP DSP types so a specific CLAP plugin can be created
} sc_dsp_config;

/**
 * @brief Metering channels the generic sc_dsp_get_metering_info accessor can query.
 * @see sc_meter, sc_meter_node in the Built-in DSPs section.
 */
typedef enum sc_dsp_meter_query
{
    sc_dsp_meter_query_peak,
    sc_dsp_meter_query_rms,
    sc_dsp_meter_query_count
} sc_dsp_meter_query;

sc_dsp_config SC_API sc_dsp_config_init(const sc_dsp_description* description);
sc_dsp_config SC_API sc_dsp_config_init_type(const sc_system* system, sc_dsp_type type);
sc_dsp_config SC_API sc_dsp_config_init_handle(const sc_system* system, sc_uint32 handle);
sc_dsp_config SC_API sc_dsp_config_init_clap(const sc_system* system, const clap_plugin_factory_t* pluginFactory);

sbk_status SC_API sc_dsp_get_parameter_float(sc_dsp* dsp, sc_uint32 index, float* value);
sbk_status SC_API sc_dsp_set_parameter_float(sc_dsp* dsp, sc_uint32 index, float value);
sbk_status SC_API sc_dsp_get_metering_info(sc_dsp* dsp, ma_uint32 channelIndex, sc_dsp_meter_query meterType, float* value);
sbk_status SC_API sc_dsp_release(sc_dsp* dsp);

/**************************************************************************************************************************************************************

Built-in DSPs

**************************************************************************************************************************************************************/

/**
 * @brief Sample values under this threshold are silenced. DSP uses this to calculate when reverb/delay echos finish.
 */
#define SC_DELAY_SILENCE_THRESHOLD 0.0001F

typedef enum sc_dsp_lowpass_param
{
    SC_DSP_LOWPASS_PARAM_CUTOFF,
    SC_DSP_LOWPASS_PARAM_COUNT
} sc_dsp_lowpass_param;

typedef enum sc_dsp_highpass_param
{
    SC_DSP_HIGHPASS_PARAM_CUTOFF,
    SC_DSP_HIGHPASS_PARAM_COUNT
} sc_dsp_highpass_param;

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

typedef enum sc_dsp_delay_param
{
    SC_DSP_DELAY_PARAM_DELAY_SECONDS,
    SC_DSP_DELAY_PARAM_DRY,
    SC_DSP_DELAY_PARAM_WET,
    SC_DSP_DELAY_PARAM_FEEDBACK,
    SC_DSP_DELAY_PARAM_COUNT
} sc_dsp_delay_param;

typedef struct sc_delay_config
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

typedef struct sc_delay
{
    sc_delay_config config;
    ma_uint32       writeCursor;
    ma_uint32       bufferSizeInFrames; //< Total buffer size. Not the delay time/size
    float*          buffer;
    ma_uint32       silentFrameCount;   //< Audio thread. Counts number of silent frames so we know when we're idle
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

typedef struct sc_delay_node_config
{
    ma_node_config nodeConfig;
    sc_delay_config delayConfig;
} sc_delay_node_config;

sc_delay_node_config SC_API sc_delay_node_config_init(ma_uint32 channels, ma_uint32 sampleRate, ma_uint32 maxDelayInFrames);

typedef struct sc_delay_node
{
    ma_node_base baseNode;
    sc_delay delay;
} sc_delay_node;

sbk_status SC_API sc_delay_node_init(ma_node_graph* nodeGraph, const sc_delay_node_config* config, const ma_allocation_callbacks* allocationCallbacks, sc_delay_node* delayNode);
void SC_API sc_delay_node_uninit(sc_delay_node* delayNode, const ma_allocation_callbacks* allocationCallbacks);

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
 * 
 * @remark Node groups own the DSP inside them and will free them when the node group is released.
 */
typedef struct sc_node_group
{
    sc_dsp* tail;   //< Left most node. Sounds and child groups connect to this
    sc_dsp* fader;  //< Controls the volume and more of the group. Exists at start
    sc_dsp* head;   //< Right/top most node. Nodes in the group route to this. The head then outputs to a parent
} sc_node_group;

sbk_status SC_API sc_node_group_init(sc_system* system, sc_node_group* nodeGroup);
sbk_status SC_API sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent);            //< Routes the group's output to the @ref parent.
sbk_status SC_API sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup);                          //< Routes the group's output directly to the graph endpoint (the audio device).
sbk_status SC_API sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp);      //< Finds the first DSP of type @p type.
sbk_status SC_API sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index);     //< Adds an existing DSP to the group at @p index. The node group owns the DSP after this.
sbk_status SC_API sc_node_group_calculate_is_idle(sc_node_group* nodeGroup, sc_bool* outIsIdle);        //< Calculates whether the node group is idle by querying all its DSP.
sbk_status SC_API sc_node_group_uninit(sc_node_group* nodeGroup);                                       //< Releases all DSP held within the node group without releasing the node group itself.
sbk_status SC_API sc_node_group_release(sc_node_group* nodeGroup);                                      //< Releases the node group and all DSP within it.

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
 * Sounds are intended to just be a loaded audio buffer plus default playback
 * settings (loop points, looping) that new voices inherit at play time.
 */
typedef struct sc_sound
{
    sc_system*                          system;
    ma_resource_manager_data_source*    dataSource;
    sc_sound_mode                       mode;
    sc_bool                             defaultLooping;
    float                               defaultLoopStartSeconds;
    float                               defaultLoopEndSeconds;
} sc_sound;

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
    const char*     filePath;               //< File path to load the sound from
    const void*     memory;                 //< In-memory sound data
    size_t          memorySize;             //< Size in bytes of the @ref memory buffer
    sc_sound_mode   mode;                   //< Sound loading/playback mode
    sc_bool         looping;                //< Default looping state for voices created from this sound
    float           loopStartSeconds;       //< Default loop start (seconds). 0 == start of sound
    float           loopEndSeconds;         //< Default loop end (seconds). <= 0 == end of sound
} sc_sound_config;

sc_sound_config SC_API sc_sound_config_init_file(const char* filePath, sc_sound_mode mode);
sc_sound_config SC_API sc_sound_config_init_memory(const void* memory, size_t memorySize, sc_sound_mode mode);

sbk_status SC_API sc_sound_get_length(sc_sound* sound, float* lengthInSeconds);
sbk_status SC_API sc_sound_release(sc_sound* sound);

/**************************************************************************************************************************************************************

Voice

**************************************************************************************************************************************************************/

typedef sc_uint64 sc_voice_handle;      //< Voice handle. Contains both a reference count and an index into the voice array
typedef sc_uint32 sc_voice_refcount;    //< References to this slot. Used to check if a handle is old/stale
typedef sc_uint32 sc_voice_index;       //< Index into the voice array

/**
 * @brief Gets the refcount from the upper 32 bits of the handle. 
 */
#define SC_VOICE_HANDLE_EXTRACT_REFCOUNT(handle)    (sc_voice_refcount)((handle) >> 32)

/**
 * @brief Gets the index from the lower 32 bits of the handle.
 */
#define SC_VOICE_HANDLE_EXTRACT_INDEX(handle)        (sc_voice_index)((handle) & 0xFFFFFFFFu)

/**
 * @brief Makes a voice handle, packing the refcount into the upper 32 bits and the index into the lower 32 bits.
 */
#define SC_VOICE_MAKE_HANDLE(refcount, slot)        (((sc_voice_handle)(refcount) << 32) | (sc_voice_handle)(slot))

typedef enum sc_voice_tiebreak_policy
{
    sc_voice_tiebreak_kill_oldest,  //< Newest voice wins ties; oldest goes virtual.
    sc_voice_tiebreak_kill_newest   //< Oldest voice wins ties; newest goes virtual.
} sc_voice_tiebreak_policy;

/**
 * @brief States set by the user, read by the system to move between the different @ref sc_voice_state states.
 * 
 * The state list is intentionally limited. Users generally want to play or stop sounds. Pausing is handled by @ref sc_voice_set_paused and setting the @ref SC_VOICE_FLAG_PAUSED state on the voice.
 */
typedef enum sc_voice_desired_state
{
    sc_voice_desired_stopped,   //< User wants the voice idle. Either not yet started, or stop/drain now.
    sc_voice_desired_playing    //< User wants the voice live. Set by sc_system_play_sound_voice.
} sc_voice_desired_state;

/**
 * @brief Current state of a voice. Written by the system, except for @ref sc_voice_state_starting which is set when a user calls @ref sc_system_play_sound.
 * 
 * The states are used as a state machine. States are written into @ref sc_voice::currentState.
 */
typedef enum sc_voice_state
{
    sc_voice_state_free,      //< Slot is empty and in the pool, ready for @ref sc_system_play_sound to claim it.
    sc_voice_state_starting,  //< Play requested; waiting on async load or first render before becoming @ref sc_voice_state_playing.
    sc_voice_state_playing,   //< Live. Audibility is governed by @ref SC_VOICE_FLAG_VIRTUAL, cursor by @ref SC_VOICE_FLAG_PAUSED.
    sc_voice_state_stopping,  //< Tail/fade-out in progress; transitions to @ref sc_voice_state_stopped when done.
    sc_voice_state_stopped    //< Drained and awaiting reap. @ref sc_system_update returns the slot to the pool on the next tick, giving in-flight stale-handle setters one full update period to observe @ref SBK_ERR_NOT_FOUND before recycling.
} sc_voice_state;

/**
 * @brief Flags written into @ref sc_voice::flags by either the user or the system.
 * 
 * Flags like @ref SC_VOICE_FLAG_PAUSED are written by the user.
 * Flags like @ref SC_VOICE_FLAG_VIRTUAL are written by the system when calculating whether a voice is real or virtual.
 */
typedef enum sc_voice_flags
{
    SC_VOICE_FLAG_NONE    = 0,
    SC_VOICE_FLAG_PAUSED  = 1u << 0,  //< Freeze the play cursor. Set by the user via sc_voice_pause.
    SC_VOICE_FLAG_VIRTUAL = 1u << 1,  //< Not connected to a real voice; audio callback skips the mix.
    SC_VOICE_FLAG_FADING  = 1u << 2   //< A volume ramp is in progress (start/stop fade, ducking, etc.).
} sc_voice_flags;

/**
 * @brief Test whether a flags word carries a given flag.
 */
#define SC_VOICE_HAS_FLAG(flags, flag) (((flags) & (sc_uint32)(flag)) != 0)

/**
 * @brief A voice is a window into a single sound that may or may not be audible.
 *
 * Voices are limited by the @ref sc_system_config::maxVoices value passed during system initialization.
 * 
 * Voice lifetime is observed indirectly through a @ref sc_voice_handle handle. 
 * Once a voice ends (either naturally or via sc_voice_stop), any subsequent sc_voice_* call using that handle will return SBK_ERR_NOT_FOUND. 
 */
typedef struct sc_voice
{
    MA_ATOMIC(8, sc_atomic_uint64)  handle;                 //< Handle for this slot's current occupant. Stale-handle callers compare against this and get SBK_ERR_NOT_FOUND on mismatch.
    sc_voice_handle                 realVoiceHandle;        //< Non-owning reference to a real voice

    sc_sound*                       sound;                  //< Source to read from
    sc_node_group*                  group;                  //< Parent group to connect to when playing for real
    MA_ATOMIC(8, sc_atomic_uint64)  playCursor;             //< Advanced by the audio thread. Read by @ref sc_system_update to compare the ages of voices
    MA_ATOMIC(8, sc_atomic_int64)   pendingSeekFrames;      //< User-thread seek target in frames. -1 means no pending seek. Audio thread applies and clears via CAS so a concurrent second seek is never lost.

    MA_ATOMIC(4, sc_atomic_uint32)  currentState;           //< sc_voice_state observed by the update loop. Update writes; anyone reads.
    MA_ATOMIC(4, sc_atomic_uint32)  desiredState;           //< sc_voice_desired_state requested by callers. Callers write; update reads.
    MA_ATOMIC(4, sc_atomic_uint32)  flags;                  //< sc_voice_flags. PAUSED written by callers; VIRTUAL/FADING written by the update loop. CAS on bit updates.

    sc_atomic_float                 gain;
    sc_atomic_float                 pitch;

    float                           oldPitch;

    sc_uint8                        priority;               //< Higher number = higher priority

    sc_atomic_float                 loopStartSeconds;       //< Comes from @ref sc_sound::defaultLoop* when @ref loopEpoch is out of date
    sc_atomic_float                 loopEndSeconds;         //< <= 0 means "to end of source"
    MA_ATOMIC(4, sc_atomic_uint32)  looping;
    MA_ATOMIC(4, sc_atomic_uint32)  loopEpoch;              //< Bumped when any loop field (loop start, loop end, etc) is written to so the audio thread can check for new data.
} sc_voice;

sbk_status SC_API sc_voice_get_is_playing(sc_system* system, sc_voice_handle handle, sc_bool* outPlaying);
sbk_status SC_API sc_voice_get_cursor_position_in_seconds(sc_system* system, sc_voice_handle handle, float* outSeconds);
sbk_status SC_API sc_voice_set_cursor_position_in_seconds(sc_system* system, sc_voice_handle handle, float seconds);
sbk_status SC_API sc_voice_set_loop_position_in_seconds(sc_system* system, sc_voice_handle handle, float loopStart, float loopEnd);
sbk_status SC_API sc_voice_get_looping(sc_system* system, sc_voice_handle handle, sc_bool* outLooping);
sbk_status SC_API sc_voice_set_looping(sc_system* system, sc_voice_handle handle, sc_bool looping);
sbk_status SC_API sc_voice_get_paused(sc_system* system, sc_voice_handle handle, sc_bool* outPaused);
sbk_status SC_API sc_voice_set_paused(sc_system* system, sc_voice_handle handle, sc_bool paused);
sbk_status SC_API sc_voice_set_virtual(sc_voice* voice, sc_bool virtualised);
sbk_status SC_API sc_voice_get_virtual(sc_system* system, sc_voice_handle handle, sc_bool* outVirtual);
sbk_status SC_API sc_voice_stop(sc_system* system, sc_voice_handle handle);

/**
 * @brief A real voice that is connected to the DSP graph.
 * 
 * Real voices are limited by the @ref sc_system_config::maxRealVoices value passed during system initialization.
 */
typedef struct sc_real_voice
{
    ma_node_base                        baseNode;           //< Must be the first member for miniaudio node graph API
    sc_system*                          system;
    sc_voice*                           voiceRef;           //< The voice we are playing for
    sc_node_group*                      nodeGroup;          //< The voice's group of DSP nodes, disconnected from the graph when virtual, connected to @ref sc_voice::group when rendering

    ma_resource_manager_data_source*    dataSource;         //< Ref-counted copy of the @ref sc_voice's data source.

    sc_uint32                           appliedLoopEpoch;   //< Last sc_voice::loopEpoch pushed to @ref dataSource. Compared on the audio thread to detect user changes.

    ma_linear_resampler                 resampler;          //< Used for pitch shifting and resampling from data source sample rate to engine sample rate
    ma_fader                            fader;              //< for fade ins and outs
    ma_gainer                           gainer;             //< Gain multiplier


    void*                               heap;
    sc_bool                             ownsHeap;
} sc_real_voice;

typedef struct sc_real_voice_config
{
    sc_system*  system;
    sc_voice*   voiceRef;
    sc_uint32   channelsIn;
    sc_uint32   channelsOut;
} sc_real_voice_config;

sc_real_voice_config SC_API sc_real_voice_config_init(sc_system* system, sc_voice* voiceRef, sc_uint32 channelsIn, sc_uint32 channelsOut);

sbk_status SC_API sc_real_voice_init(const sc_real_voice_config* config, sc_real_voice* realVoice);
sbk_status SC_API sc_real_voice_uninit(sc_real_voice* realVoice);
sbk_status SC_API sc_real_voice_init_preallocated(const sc_real_voice_config* config, void* heap, sc_real_voice* realVoice);

/**
 * @brief Used to sort voices during @ref sc_system_update.
 */
typedef struct sc_virtual_voice_candidate
{
    sc_uint32   voiceIndex;
    sc_uint32   priority;       //< Voice priority. Main way of comparing a voice. Voices with higher priorities are never culled by voices with lower priority
    float       audibility;     //< Tiebreaker. Louder wins. @todo Make this an int so we are not comparing a float
    sc_uint64   playCursor;     //< Tiebreaker. Used to compare which voice is oldest
} sc_virtual_voice_candidate;

/**************************************************************************************************************************************************************

Bank

**************************************************************************************************************************************************************/

#define SC_BANK_VERSION 1u

#define SC_FOURCC(a, b, c, d) ((sc_uint32)(((d) << 24) | ((c) << 16) | ((b) << 8) | (a)))

#define SC_BANK_ID             (SC_FOURCC('S', 'C', 'B', 'K'))
#define SC_BANK_AUDIO_CHUNK_ID (SC_FOURCC('S', 'C', 'A', 'C'))
#define SC_BANK_SUB_ID         (SC_FOURCC('S', 'C', 'F', 'E'))

typedef struct sc_sub_chunk sc_sub_chunk;
typedef struct sc_riff_chunk sc_riff_chunk;
typedef struct sc_audio_chunk sc_audio_chunk;

enum
{
    SC_BANK_FILE_NAME_BUFFER_SIZE = 64
};

struct sc_audio_chunk
{
    ma_uint32 id;
    ma_uint32 size;
    char name[SC_BANK_FILE_NAME_BUFFER_SIZE];
    void* data;
};

struct sc_riff_chunk
{
    ma_uint32 id;
    ma_uint32 size;
    ma_uint32 version;
    ma_uint32 numOfSubchunks;
    sc_audio_chunk** subChunks;
};

typedef struct sc_bank
{
    sc_riff_chunk* riff;     //< bank data. Filled upon reading
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
    float                       vol0Threshold;          //< Volumes before this volume are automatically virtualised
    sc_voice_tiebreak_policy    tiebreakPolicy;         //< When voices of the same priority are in fighting for a real voice, do we prefer the oldest or newest?
    sc_bool                     noDevice;               //< When true, no audio device is created. Use sc_system_read_pcm_frames to pull audio manually.
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
 * @remark When finished, system objects must be closed with @ref sc_system_close.
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
    sc_voice*                   voiceBuffer;                                            //< Allocated voices. Indexed through the @ref voiceSlotAllocator

    ma_slot_allocator           realVoiceSlotAllocator;                                 //< Allocates handles for real voices
    sc_real_voice*              realVoiceBuffer;                                        //< Allocated real voices. Indexed through @ref realVoiceSlotAllocator

    float                       vol0Threshold;                                          //< Voices under this are virtualized
    sc_voice_tiebreak_policy    tiebreakerPolicy;                                       //< When voices of the same priority are in fighting for a real voice, do we prefer the oldest or newest?

    sc_virtual_voice_candidate* virtualizeCandidates;                                   //< [0-maxVoices]. Used to sort voices
    sc_virtual_voice_candidate* virtualizeBoundary;                                     //< [0-maxVoices]. Used to sort voices
};

/**
 * @brief Sets up logging.
 * @remark Must be called before @ref sc_system_init.
 */
sbk_status SC_API sc_system_log_init(sc_system* system, ma_log_callback_proc logCallback);

sc_system_config SC_API sc_system_config_init_default();
sc_system_config SC_API sc_system_config_init(const char* pluginPath);

sbk_status SC_API sc_system_init(sc_system* system, const sc_system_config* systemConfig);
sbk_status SC_API sc_system_close(sc_system* system);

/**
* @brief Updates the Sound Chef @ref sc_system on the calling thread.
* 
* @remark When using Sound Bakery, the system thread will call this function, thereby moving audio updates off the game thread (if in async mode).
*/
sbk_status SC_API sc_system_update(sc_system* system);

/**
 * @brief Process all voices and calculates whether they are virtual or not. Called during @ref sc_system_update.
 * 
 * Top-K selection by counting sort over 8-bit priority: a 256-bucket
 * histogram (L1-resident) is walked top-down and stopped once cumulative
 * count reaches maxRealVoices. Linear in N, independent of K, no key
 * comparisons on the payload.
 *
 * Ties at the cutoff priority land in a small boundary set resolved by
 * insertion sort on (audibility desc, playCursor per tiebreak policy).
 * Insertion sort wins on constant factors at that size (usually < 10).
 *
 * Assumes priority fits in a byte. Widening past that requires either
 * quantising back into a small integer range or switching to a
 * heap-based partial sort.
 * 
 * @remark Does not need to be called by the user.
 */
sbk_status SC_API sc_system_calculate_virtual_voices(sc_system* system);

/**
* @brief Reads/pulls PCM data from the node graph and outputs them to @p framesOut.
*/
sbk_status SC_API sc_system_read_pcm_frames(sc_system* system, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead);

sbk_status SC_API sc_system_create_sound(sc_system* system, const sc_sound_config* config, sc_sound** sound);
sbk_status SC_API sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup);

/**
* @brief Tries to assign a new @ref sc_voice for the @p sound.
* 
* Playing is not guaranteed. Voices can:
*   - Fail to allocate a slot into the voice array. Sounds are dropped and will never play
*   - Allocate a voice but fail to allocate a real voice. The sound is tracked but not audible
*   - Allocate a voice and a real voice. The sound will be audible
*
* @param parent     Optional parameter. Outputs to the master node group by default
* 
* @see sc_voice, sc_real_voice
*/
sbk_status SC_API sc_system_play_sound(sc_system* system, sc_sound* sound, sc_voice_handle* outVoiceHandle, sc_node_group* parent, sc_bool paused);

sbk_status SC_API sc_system_stop_all_voices(sc_system* system);
sbk_status SC_API sc_system_clap_get_count(const sc_system* system, ma_uint32* count);
sbk_status SC_API sc_system_clap_get_at(const sc_system* system, ma_uint32 index, sc_clap** plugin);
sbk_status SC_API sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp);
sbk_status SC_API sc_system_get_dsp_desc(const sc_system* system, sc_uint32 handle, const sc_dsp_description** outDescription);

/**************************************************************************************************************************************************************

Utilities

**************************************************************************************************************************************************************/

static MA_INLINE sc_bool sc_is_pow2(size_t x)
{
    return x != 0 && (x & (x - 1)) == 0;
}

/**
 * @remark Returns 1 for x <= 1. Returns 0 when x is larger than the greatest
 * power of two representable in size_t (i.e. x > (SIZE_MAX >> 1) + 1); the
 * caller should treat 0 as "not representable".
 */
static MA_INLINE size_t sc_next_pow2(size_t x)
{
    if (x <= 1) return 1;
    size_t v = 1;
    while (v < x)
    {
        if (v > (SIZE_MAX >> 1))
        {
            return 0;
        }
        v <<= 1;
    }
    return v;
}

ma_handle SC_API sc_dlopen(ma_log* pLog, const char* filename);
void SC_API sc_dlclose(ma_log* pLog, ma_handle handle);
ma_proc SC_API sc_dlsym(ma_log* pLog, ma_handle handle, const char* symbol);

SC_CLASS const char* SC_CALL sc_filename_get_ext(const char* filename);

void SC_API sc_channel_map_apply_f32(float* pFramesOut, const ma_channel* pChannelMapOut, ma_uint32 channelsOut, const float* pFramesIn, const ma_channel* pChannelMapIn, ma_uint32 channelsIn, ma_uint64 frameCount, ma_channel_mix_mode mode, ma_mono_expansion_mode monoExpansionMode);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H