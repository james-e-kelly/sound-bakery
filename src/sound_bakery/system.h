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
            class voice_tracker;
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
            [[nodiscard]] auto get_game_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>;
            [[nodiscard]] auto get_system_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>;
            [[nodiscard]] auto get_background_thread_executer() const -> std::shared_ptr<concurrencpp::thread_pool_executor>;
            [[nodiscard]] auto get_listener_game_object() const -> sbk::engine::game_object*;
            [[nodiscard]] auto get_master_bus() const -> sbk::engine::bus*;
            [[nodiscard]] auto get_current_object_owner() -> sbk::core::object_owner*;  //< Either project for editor or system for runtime

            /**
             * @brief Creates an instance of Sound Bakery and opens the project.
             */
            [[nodiscard]] static auto open_project(const std::filesystem::path& projectFile, sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>;

            /**
             * @brief Creates a project and initializes Sound Bakery.
             */
            [[nodiscard]] static auto create_project(const std::filesystem::directory_entry& projectDirectory,
                                       std::string_view projectName) -> sbk::result<void>;

            auto set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus) -> void;

            friend class boost::serialization::access;

            template <class archive_class>
            auto serialize(archive_class& archive, const unsigned int version) -> void
            {
            }

        private:
            auto update_async() -> void;

            bool m_registeredReflection = false;

            std::shared_ptr<sbk::engine::game_object> m_listenerGameObject;
            std::shared_ptr<sbk::engine::bus> m_masterBus;
            std::unique_ptr<sbk::editor::project> m_project;
            std::unique_ptr<profiling::voice_tracker> m_voiceTracker;
            std::unique_ptr<concurrencpp::runtime> m_threadRuntime;
            std::shared_ptr<concurrencpp::manual_executor> m_gameThreadExecuter;
            std::shared_ptr<concurrencpp::manual_executor> m_studioThreadExecuter;
            std::shared_ptr<concurrencpp::worker_thread_executor> m_workerThread;
            concurrencpp::timer m_studioThreadTimer;
        };
    }  // namespace engine
}  // namespace sbk