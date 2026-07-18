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
     * @brief Creates a new System object.
     * @see sc_system_init sc_system_release
     */
    sbk_status SC_API sc_system_create(sc_system** outSystem);

    /**
     * @brief Releases the system memory.
     * @see sc_system_close sc_system_create
     */
    sbk_status SC_API sc_system_release(sc_system* system);

    /**
     * @brief Sets up logging.
     *
     * Must be called before sc_system_init.
     */
    sbk_status SC_API sc_system_log_init(sc_system* system, ma_log_callback_proc logCallback);

    sc_system_config SC_API sc_system_config_init_default();
    sc_system_config SC_API sc_system_config_init(const char* pluginPath);

    /**
     * @brief Initialises the system.
     *
     * Initializes the ma_engine and therefore the ma_device. This makes the
     * system connect to the user's audio device and is then ready for playing
     * sounds.
     */
    sbk_status SC_API sc_system_init(sc_system* system, const sc_system_config* systemConfig);

    /**
     * @brief Closes the system.
     *
     * Disconnects the system from the user's audio device and stops all sounds.
     */
    sbk_status SC_API sc_system_close(sc_system* system);

    ////////////////////////////////////////////////////////////////////////////

    sbk_status SC_API sc_system_create_sound(sc_system* system,
                                            const char* fileName,
                                            sc_sound_mode mode,
                                            sc_sound** sound);

    sbk_status SC_API sc_system_create_sound_memory(
        sc_system* system, void* memoryLocation, size_t soundSize, sc_sound_mode mode, sc_sound** sound);

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
     * @param get_parent optional parameter. Outputs to the master node group by
     * default
     * @param paused whether this sound is paused upon creation or played
     * instantly
     * @return
     */
    sbk_status SC_API sc_system_play_sound(
        sc_system* system, sc_sound* sound, sc_sound_instance** instance, sc_node_group* parent, sc_bool paused);

    /**
     * @brief Creates a new Node Group
     *
     * Creates a new Node Group with a SC_DSP_TYPE_FADER by default (for
     * handling volume, pitch etc.). Connects to the endpoint by default.
     */
    sbk_status SC_API sc_system_create_node_group(sc_system* system, sc_node_group** nodeGroup);

    /**
     * @brief Creates a new DSP based on the config.
     *
     * @warning The config parameter must have its vtable pointer set.
     * Using sc_dsp_config_init will ensure the vtable is correct for built in
     * types. However, users must fill it themselves for custom DSP types.
     *
     * Example:
     * @code
     * sc_dsp* dsp = NULL;
     * sc_dsp_config lpfConfig = sc_dsp_config_init(SC_DSP_TYPE_LOWPASS);
     * sc_system_create_dsp(system, &lpfConfig, &dsp);
     * @endcode
     *
     * @see sc_dsp_config_init
     */
    sbk_status SC_API sc_system_create_dsp(sc_system* system, const sc_dsp_config* config, sc_dsp** dsp);

    sbk_status SC_API sc_sound_get_length(sc_sound* sound, float* lengthInSeconds);

    sbk_status SC_API sc_sound_release(sc_sound* sound);

    /**
     * @brief Checks whether the sound instance is playing or not.
     */
    sbk_status SC_API sc_sound_instance_is_playing(sc_sound_instance* instance, sc_bool* isPlaying);

    /**
     * @brief Plays a sound instance, if it is not already.
     * 
     * Does not create any new resources, unlike sc_system_play_sound.
     */
    sbk_status SC_API sc_sound_instance_start(sc_sound_instance* instance);

    /**
     * @brief Pause playback at the current time.
     */
    sbk_status SC_API sc_sound_instance_pause(sc_sound_instance* instance);

    sbk_status SC_API sc_sound_instance_get_cursor_in_seconds(sc_sound_instance* instance, float* seconds);
    sbk_status SC_API sc_sound_instance_set_cursor_in_seconds(sc_sound_instance* instance, float seconds);

    sbk_status SC_API sc_sound_instance_get_loop_position_in_seconds(sc_sound_instance* instance, float* seconds);
    sbk_status SC_API sc_sound_instance_set_loop_position_in_seconds(sc_sound_instance* instance, float loopStartSeconds, float loopEndSeconds);
    sbk_status SC_API sc_sound_instance_get_is_looping(
        sc_sound_instance* instance, sc_bool* looping);
    sbk_status SC_API sc_sound_instance_set_looping(sc_sound_instance* instance, sc_bool looping);

    /**
     * @brief Releases the sound instance's resources.
     * 
     * The raw sc_sound needs to be released as well, once used, to release all resources.
     */
    sbk_status SC_API sc_sound_instance_release(sc_sound_instance* instance);

    /**
     * @brief Returns a valid sc_dsp_config object for the DSP type.
     *
     * @warning Will not return a valid config if the type ==
     * SC_DSP_TYPE_UNKOWN.
     */
    sc_dsp_config SC_API sc_dsp_config_init(sc_dsp_type type);
    sc_dsp_config SC_API sc_dsp_config_init_clap(const clap_plugin_factory_t* pluginFactory);

    sbk_status SC_API sc_dsp_get_parameter_float(sc_dsp* dsp, int index, float* value);
    sbk_status SC_API sc_dsp_set_parameter_float(sc_dsp* dsp, int index, float value);
    sbk_status SC_API sc_dsp_get_metering_info(sc_dsp* dsp,
                                              ma_uint32 channelIndex,
                                              sc_dsp_meter meterType,
                                              float* value);
    sbk_status SC_API sc_dsp_release(sc_dsp* dsp);

    sbk_status SC_API sc_node_group_set_volume(sc_node_group* nodeGroup, float volume);
    sbk_status SC_API sc_node_group_get_volume(sc_node_group* nodeGroup, float* volume);

    sbk_status SC_API sc_node_group_set_parent(sc_node_group* nodeGroup, sc_node_group* parent);
    sbk_status SC_API sc_node_group_set_parent_endpoint(sc_node_group* nodeGroup);

    sbk_status SC_API sc_node_group_get_dsp(sc_node_group* nodeGroup, sc_dsp_type type, sc_dsp** dsp);

    sbk_status SC_API sc_node_group_add_dsp(sc_node_group* nodeGroup, sc_dsp* dsp, sc_dsp_index index);
    sbk_status SC_API sc_node_group_remove_dsp(sc_node_group* nodeGroup, sc_dsp* dsp);

    sbk_status SC_API sc_node_group_release(sc_node_group* nodeGroup);

    // CLAP

    sbk_status SC_API sc_system_clap_get_count(sc_system* system, ma_uint32* count);

    /**
     * @brief Get the CLAP plugin at a specified index.
     * 
     * Fills the plugin parameter with a pointer to a loaded CLAP plugin.
     * 
     * @warning Do not free the plugin pointer or unload any of the internals of the struct.
     * @warning Do not access the pointer after the system is closed.
     */
    sbk_status SC_API sc_system_clap_get_at(sc_system* system, ma_uint32 index, sc_clap** plugin);

#ifdef __cplusplus
}
#endif

#endif  // #ifndef SOUND_CHEF_H