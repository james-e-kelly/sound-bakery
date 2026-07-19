#pragma once

#include "core/logger.h"
#include "sound_bakery/error/error.h"
#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/object/object_tracker.h"

namespace sbk
{
    namespace editor
    {
        class project;
    }  // namespace editor

    namespace engine
    {
        namespace profiling
        {
            class property_broadcaster;
            class remote_session;
            class remote_session_host;
            class voice_tracker;
            struct set_property_command;
        }

        class bus;
        class game_object;

        auto malloc(std::size_t size, SB_OBJECT_CATEGORY category) -> void*;
        auto realloc(void* pointer, std::size_t size) -> void*;
        auto free(void* pointer, SB_OBJECT_CATEGORY category) -> void;

        /**
         * @brief Manager of the whole Sound Bakery.
         *
         * The system tracks all objects created during Sound Bakery's lifetime.
         *
         * It owns all loaded Soundbanks, listener game object, and busses.
         */
        class SB_CLASS system final : public sc_system,
                                      public sbk::core::logger,  
                                      public sbk::core::object_owner,
                                      public sbk::core::object_tracker,
                                      public sbk::core::database,
                                      public boost::noncopyable
        {
            REGISTER_REFLECTION(system)
            LEAK_DETECTOR(system)

        public:
            enum class operating_mode : uint8_t
            {
                unkown, //< Unkown/unset
                editor, //< We have a project
                runtime //< We are loading soundbanks
            };

        public:
            system();
            system(const std::filesystem::path& logFile);
            system(sbk::core::sbk_log_callback_proc logCallback);
            ~system();

            [[nodiscard]] static auto create() -> sbk::result<void>;
            [[nodiscard]] static auto create(const std::filesystem::path& logFile) -> sbk::result<void>;
            static auto destroy() -> void;

            [[nodiscard]] auto init(const sbk_system_config& config) -> sbk::result<void>;
            [[nodiscard]] auto update() -> sbk::result<void>;

            [[nodiscard]] static auto get() -> system*;
            [[nodiscard]] static auto get_operating_mode() -> operating_mode;
            [[nodiscard]] static auto get_project() -> sbk::editor::project*;
            [[nodiscard]] static auto get_voice_tracker() -> sbk::engine::profiling::voice_tracker*;
            [[nodiscard]] static auto get_remote_session_host() -> sbk::engine::profiling::remote_session_host*;
            [[nodiscard]] auto get_game_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>;
            [[nodiscard]] auto get_system_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>;
            [[nodiscard]] auto get_background_thread_executer() const -> std::shared_ptr<concurrencpp::thread_pool_executor>;
            [[nodiscard]] auto get_listener_game_object() const -> std::shared_ptr<sbk::engine::game_object>;
            [[nodiscard]] auto get_master_bus() const -> std::shared_ptr<sbk::engine::bus>;
            [[nodiscard]] auto get_current_object_owner() -> sbk::core::object_owner*;  //< Either project for editor or system for runtime

            /**
             * @brief Creates an instance of Sound Bakery and opens the project.
             */
            [[nodiscard]] static auto open_project(const std::filesystem::path& projectFile, sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>;

            /**
             * @brief Creates a project and initializes Sound Bakery.
             */
            [[nodiscard]] static auto create_project(const std::filesystem::directory_entry& projectDirectory, std::string_view projectName) -> sbk::result<void>;

            auto set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus) -> void;

            /**
             * @brief Starts the remote session host so external tools can connect, receive telemetry, and push live edits.
             *
             * Telemetry is published from update(). Pass 0 for an OS-chosen port.
             */
            [[nodiscard]] auto host_remote_session(uint16_t port) -> sbk::result<void>;

            /**
             * @brief Stops the remote session host and disconnects every tool.
             */
            auto stop_hosting_remote_session() -> void;

            /**
             * @brief Connects this (authoring) instance to a remote runtime hosting a session.
             *
             * Once connected, every object property exposed via
             * gather_synced_properties syncs automatically: current values are
             * pushed on connect (so pre-connection edits arrive like Wwise's
             * remote sessions), and later edits broadcast the moment they
             * happen. No editor code is involved.
             */
            [[nodiscard]] auto connect_remote_session(std::string_view host, uint16_t port) -> sbk::result<void>;

            auto disconnect_remote_session() -> void;

            [[nodiscard]] auto get_remote_session() -> profiling::remote_session*;

            friend class boost::serialization::access;

            template <class archive_class>
            auto serialize(archive_class& archive, const unsigned int version) -> void
            {
            }

        private:
            auto update_async() -> void;
            auto apply_remote_property_command(const profiling::set_property_command& command) -> void;

            bool m_registeredReflection = false;
            bool m_initSoundChef        = false;

            std::weak_ptr<sbk::engine::game_object> m_listenerGameObject;
            std::weak_ptr<sbk::engine::bus> m_masterBus;
            std::unique_ptr<sbk::editor::project> m_project;
            std::unique_ptr<profiling::voice_tracker> m_voiceTracker;
            std::unique_ptr<profiling::remote_session_host> m_remoteSessionHost;
            std::unique_ptr<profiling::remote_session> m_remoteSession;
            std::unique_ptr<profiling::property_broadcaster> m_propertyBroadcaster;
            std::unique_ptr<concurrencpp::runtime> m_threadRuntime;
            std::shared_ptr<concurrencpp::manual_executor> m_gameThreadExecuter;
            std::shared_ptr<concurrencpp::manual_executor> m_studioThreadExecuter;
            std::shared_ptr<concurrencpp::worker_thread_executor> m_workerThread;
            concurrencpp::timer m_studioThreadTimer;
        };
    }  // namespace engine
}  // namespace sbk