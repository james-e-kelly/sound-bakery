/**
 * @file
 * @brief DSP object definitions.
 *
 *
 */

#ifndef SOUND_CHEF_DSP
#define SOUND_CHEF_DSP

#define SC_STRING_NAME_LENGTH   16

/**
 * @brief Holds instance data for a single SC_DSP.
 *
 * Each DSP callback is passed a SC_DSP_STATE object. This state object can be
 * used to access the system, the user created object (likely a miniaudio node)
 * and the SC_DSP object.
 */
typedef struct SC_DSP_STATE
{
    void* m_instance;  //< points to the current SC_DSP object
    void* m_userData;  //< points to uer created object, likely some type of
                       // ma_node
    void* m_system;    //< points to the owning SC_SYSTEM object
} SC_DSP_STATE;

typedef SC_RESULT(SC_CALL* SC_DSP_CREATE_CALLBACK)(SC_DSP_STATE* dsp_state);
typedef SC_RESULT(SC_CALL* SC_DSP_RELEASE_CALLBACK)(SC_DSP_STATE* dsp_state);

typedef SC_RESULT(SC_CALL* SC_DSP_SET_PARAM_FLOAT_CALLBACK)(
    SC_DSP_STATE* dsp_state, int index, float value);
typedef SC_RESULT(SC_CALL* SC_DSP_GET_PARAM_FLOAT_CALLBACK)(
    SC_DSP_STATE* dsp_state, int index, float* value);

typedef struct
{
    SC_DSP_CREATE_CALLBACK m_create;
    SC_DSP_RELEASE_CALLBACK m_release;
    SC_DSP_SET_PARAM_FLOAT_CALLBACK m_setFloat;
    SC_DSP_GET_PARAM_FLOAT_CALLBACK m_getFloat;

    SC_DSP_PARAMETER** m_params;
    int m_numParams;
} SC_DSP_VTABLE;

struct SC_DSP_CONFIG
{
    SC_DSP_TYPE m_type;
    SC_DSP_VTABLE* m_vtable;
};

/**
 * @brief ma_node with an additional enum descriptor.
 *
 * Helper struct to make adding/create DSPs as simple as calling a single
 * function. Raw miniaudio requires you to know the effect you're inserting.
 */
struct SC_DSP
{
    SC_DSP_STATE* m_state;    // holds the instance data for the dsp
    SC_DSP_VTABLE* m_vtable;  // holds the functions for interacting with the
                              // underlying node type. Must be not null

    SC_DSP_TYPE m_type;

    SC_DSP* m_next;  // when in a node group, the parent/next dsp. Can be null
                     // if the head node
    SC_DSP* m_prev;  // when in a node group, the child/previous dsp. Can be
                     // null if the tail node
};

typedef enum SC_DSP_PARAMETER_TYPE
{
    SC_DSP_PARAMETER_TYPE_FLOAT
} SC_DSP_PARAMETER_TYPE;

typedef struct SC_DSP_PARAMETER_FLOAT
{
    float m_min;
    float m_max;
    float m_value;
} SC_DSP_PARAMETER_FLOAT;

struct SC_DSP_PARAMETER
{
    SC_DSP_PARAMETER_TYPE m_type;
    char m_name[SC_STRING_NAME_LENGTH];

    union
    {
        SC_DSP_PARAMETER_FLOAT m_float;
    };
};

typedef enum SC_DSP_LOWPASS
{
    SC_DSP_LOWPASS_CUTOFF,
    SC_DSP_LOWPASS_NUM_PARAM
} SC_DSP_LOWPASS;

typedef enum SC_DSP_HIGHPASS
{
    SC_DSP_HIGHPASS_CUTOFF,
    SC_DSP_HIGHPASS_NUM_PARAM
} SC_DSP_HIGHPASS;

#endif