#ifndef SOUND_BAKERY_H
#define SOUND_BAKERY_H

#include "sound_bakery_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * @brief Log a message to Sound Bakery.
	 * 
	 * There is no guarantee the message will reach a specific output, like the console or a file.
	 * It is possible in some configurations that the message will not go anywhere.
	 * The log destination all depends on the @r sbk_system_config options and how the user handles messages received in the logging callback.
	 * 
	 * @param level of the message, from MA_LOG_LEVEL_DEBUG to MA_LOG_LEVEL_ERROR. Use MA_LOG_LEVEL_INFO for regular messages
	 * @param message to log
	 * @return SBK_SUCCESS on successful logging
	 * @return SBK_ERR_INVALID_PARAMETER if the message is null
	 * @return SBK_ERR_BAKERY_UNINITIALIZED if the system object does not exist or is not initialized
	 */
	sbk_status SB_API sbk_log(ma_log_level level, const char* message);

	/**
	 * @brief Create a @r sbk_system_config object with default options.
	 * 
	 * A call to @r sbk_system_init with this config will result in a valid Sound Bakery System that can output audio to the default system output/device.
	 * 
	 * @return a sbk_system_config object with default options
	 */
	sbk_system_config SB_API sbk_system_config_init_default();

	/**
	 * @brief Create a @r sbk_system_config object that points at a specific directory for loading plugins from.
	 * 
	 * Similar to @r sbk_system_config_init_default, a Sound Bakery System will be valid when using this config and can output audio.
	 * 
	 * @note The function does not care about being passed a null pointer. If it is imperative Sound Bakery receives a plugin path, please check the pointer beforehand
	 * @param pluginPath a file path that contains plugins
	 * @return a sbk_system_config object that points at a plugin path
	 */
	sbk_system_config SB_API sbk_system_config_init(const char* pluginPath);

	/**
	 * @brief Create the global @r sbk::engine::system object and the underlying @r sc_system.
	 * 
	 * @warn @r sbk_system_create and @r sbk_system_destroy are not thread-safe. Calling either of these functions concurrently with any other API function may cause undefined behavior.
	 * @return SBK_SUCCESS if the system was successfully created or already exists
	 * @return SBK_ERR_OUT_OF_MEMORY if the system could not be created
	 */
	SBK_NODISCARD sbk_status SB_API sbk_system_create();

	/**
	 * @brief Initialize the system.
	 * 
	 * sc_system is also initialized at the same time.
	 * 
	 * @param config the object to configure the system's behaviour
	 * @return SBK_SUCCESS if the system was successfully initialized
	 */
	SBK_NODISCARD sbk_status SB_API sbk_system_init(sbk_system_config config);

	/**
	 * @brief Update the system.
	 * 
	 * If Sound Bakery is running in an asynchronous mode, this function submits all queued data (commands and variables) to the system and runtime thread.
	 * 
	 * @return SBK_SUCCESS if the system successfully updated
	 */
	SBK_NODISCARD sbk_status SB_API sbk_system_update();

	/**
	 * @brief Destroy the system object and release all memory.
	 * 
	 * Everything, including threads, sc_system, jobs, and data is destroyed and released.
	 * 
	 * @warn @r sbk_system_create and @r sbk_system_destroy are not thread-safe. Calling either of these functions concurrently with any other API function may cause undefined behavior.
	 * @return 
	 */
	sbk_status SB_API sbk_system_destroy();

	/**
	 * @brief Load a soundbank and all its objects into memory.
	 * 
	 * @todo Make soundbank loading happen on the system thread
	 * @param soundbankFilePath path to the soundbank file on disk
	 * @param outSoundbankID the ID of the soundbank that was loaded. This is not a handle. It is the ID of the object, as it was at build time
	 * @return SBK_SUCCESS if the load and deserialization was succesful
	 */
	SBK_NODISCARD sbk_status SB_API sbk_system_load_soundbank(const char* soundbankFilePath, sbk_id* outSoundbankID);

	/**
	 * @brief Post an event to the system by its name.
	 * 
	 * Call @r sbk_system_update to submit the command to the system thread for processing.
	 * 
	 * @param eventName name of the event. Used to look up its database ID
	 * @param gameObjectID game object to post the event on. If gameObjectID == 0, it is posted on the global object
	 * @return SBK_SUCCESS if the message was enqueued correctly
	 */
	SBK_NODISCARD sbk_status SB_API sbk_system_post_event(const char* eventName, sbk_id gameObjectID);

	/**
	 * @brief Stop all events and sounds on the game object.
	 * 
	 * @warn This function can stop all sounds if passed 0 to @r gameObjectID.
	 * @param gameObjectID game object to stop. If gameObjectID == 0, all sounds are stopped, across the entire system
	 * @return SBK_SUCCESS if the message was enqueued correctly
	 */
	SBK_NODISCARD sbk_status SB_API sbk_system_stop_all(sbk_id gameObjectID);

#ifdef __cplusplus
}
#endif

#endif