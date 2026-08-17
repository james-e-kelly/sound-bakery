#ifndef SOUND_CHEF_H
#define SOUND_CHEF_H

/**
 * @file
 * @brief A wrapper library for miniaudio that emulates functionality of FMOD.
 *
 * The low level engine powering Sound Bakery.
 */

#include "sound_chef_common.h"
#include "sound_chef_dsp.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @defgroup System System Functions
     * 
     * System objects must be created and then initialized.
     * When finished, system objects must be closed and then released.
     * @{
     */

    sbk_status SC_API sc_system_create(sc_system** outSystem);
    sbk_status SC_API sc_system_release(sc_system* system);

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
     * @brief Reads/pulls PCM data from the node graph and outputs them to @p framesOut.
     */
    sbk_status SC_API sc_system_read_pcm_frames(sc_system* system, void* framesOut, ma_uint64 frameCount, ma_uint64* framesRead);

    /**
     * @defgroup SystemSound System Sound Functions
     * @ingroup System
     * @{
     */

    sc_sound_config SC_API sc_sound_config_init_file(const char* filePath, sc_sound_mode mode);
    sc_sound_config SC_API sc_sound_config_init_memory(const void* memory, size_t memorySize, sc_sound_mode mode);

    sbk_status SC_API sc_system_create_sound(sc_system* system, const sc_sound_config* config, sc_sound** sound);

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

    /**@}*/

    /**
     * @defgroup SystemNodeGroup System Node Group Functions
     * @ingroup System
     * @{
     */

    sbk_status SC_API sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup);

    /**@}*/

    /**
     * @defgroup SystemDSP System DSP Creation Functions
     * @ingroup System
     *
     * DSP units are created with @ref sc_dsp_description objects.
     * The sc_system stores/knows about two description arrays.
     * One is internal and lets users create units using the @ref sc_dsp_type.
     * The other is external and lets users create units with a custom type.
     * 
     * The support creating internal and external units, all descriptions are looked up by a handle.
     * If the handle is between 0 and SC_DSP_TYPE_COUNT, the handle is used to index the internal description array.
     * If the handle is greater than SC_DSP_TYPE_COUNT, SC_DSP_TYPE_COUNT is subtracted from the handle and used as the index into the external description array.
     * @{
     */

    sc_dsp_config SC_API sc_dsp_config_init(const sc_dsp_description* description);
    sc_dsp_config SC_API sc_dsp_config_init_type(const sc_system* system, sc_dsp_type type);
    sc_dsp_config SC_API sc_dsp_config_init_handle(const sc_system* system, sc_uint32 handle);
    sc_dsp_config SC_API sc_dsp_config_init_clap(const sc_system* system, const clap_plugin_factory_t* pluginFactory);
    
    sbk_status SC_API sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp);

    /**
     * @brief Returns a pointer to the description for the given handle.
     * @remark The description is owned by the system; do not free the returned pointer.
     */
    sbk_status SC_API sc_system_get_dsp_desc(const sc_system* system, sc_uint32 handle, const sc_dsp_description** outDescription);

    /**@}*/

    /**
     * @defgroup SystemClap System CLAP Functions
     * @ingroup System
     *
     * The system loads CLAP plugins from a directory and stores pointers to the plugin structures.
     * These functions let users query the number of plugins and get pointers to the plugin structures.
     * 
     * @remark As the memory is owned by the system, the pointers should never be freed or used after the system is released.
     * @{
     */

    sbk_status SC_API sc_system_clap_get_count(const sc_system* system, ma_uint32* count);
    sbk_status SC_API sc_system_clap_get_at(const sc_system* system, ma_uint32 index, sc_clap** plugin);

    /**@}*/
    /**@}*/

    /**
     * @defgroup Sound Sound Functions
     * @{
     */

    sbk_status SC_API sc_sound_get_length(sc_sound* sound, float* lengthInSeconds);
    sbk_status SC_API sc_sound_release(sc_sound* sound);

    /**@}*/

    /**
     * @defgroup SoundInstance Sound Instance Functions
     * @{
     */

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

    /**@}*/

    /**
     * @defgroup DSP DSP Functions
     * @{
     */

    sbk_status SC_API sc_dsp_get_parameter_float(sc_dsp* dsp, sc_uint32 index, float* value);
    sbk_status SC_API sc_dsp_set_parameter_float(sc_dsp* dsp, sc_uint32 index, float value);
    sbk_status SC_API sc_dsp_get_metering_info(sc_dsp* dsp, ma_uint32 channelIndex, sc_dsp_meter_query meterType, float* value);
    sbk_status SC_API sc_dsp_release(sc_dsp* dsp);

    /**@}*/

    /**
     * @defgroup NodeGroup Node Group Functions
     * @{
     */

    sbk_status SC_API sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent);

    /**
     * @brief Routes the group's output directly to the graph endpoint (the audio device).
     *
     * Use this for top-level groups that should feed straight to the output
     * rather than through another group.
     */
    sbk_status SC_API sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup);

    /**
     * @brief Finds the first DSP in the group whose handle matches @p type.
     *
     * @return SBK_SUCCESS if the DSP was found and @p dsp is valid.
     * @return SBK_ERR_NOT_FOUND if the DSP was not found.
     */
    sbk_status SC_API sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp);

    sbk_status SC_API sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index);
    sbk_status SC_API sc_node_group_release(sc_node_group* nodeGroup);

    /**@}*/

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H