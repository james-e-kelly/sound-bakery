#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace sbk
{
    namespace core
    {
        class object_owner;
    }

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

        void* malloc(std::size_t size, SB_OBJECT_CATEGORY category);
        void* realloc(void* pointer, std::size_t size);
        void free(void* pointer, SB_OBJECT_CATEGORY category);

        /**
         * @brief Manager of the whole Sound Bakery.
         *
         * The system tracks all objects created during Sound Bakery's lifetime.
         *
         * It owns all loaded Soundbanks, listener game object, and busses.
         */
        class SB_CLASS system final : public sc_system,
                                      public sbk::core::object_owner,
                                      public sbk::core::object_tracker,
                                      public sbk::core::database,
                                      public boost::noncopyable
        {
            REGISTER_REFLECTION(system)
            LEAK_DETECTOR(system)

        public:
            enum class operating_mode
            {
                unkown, //< Unkown/unset
                editor, //< We have a project
                runtime //< We are loading soundbanks
            };

        public:
            system();
            ~system();

            static auto create() -> system*;
            static auto init(const sb_system_config& config) -> sc_result;
            static auto update() -> sc_result;
            static auto destroy() -> void;

            static auto post_event(const char* eventName, sbk_id gameObjectID) -> sb_result;
            static auto post_container(sbk_id containerID, sbk_id gameObjectID) -> sb_result;
            
            static auto stop_all(sbk_id gameObjectID) -> sb_result;

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
            static auto open_project(const std::filesystem::path& project_file) -> sc_result;

            /**
             * @brief Creates a project and initializes Sound Bakery.
             */
            static auto create_project(const std::filesystem::directory_entry& projectDirectory,
                                       const std::string& projectName) -> sc_result;

            static auto load_soundbank(const std::filesystem::path& file) -> sb_result;

            auto set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus) -> void;

            friend class boost::serialization::access;

            template <class archive_class>
            void serialize(archive_class& archive, const unsigned int version)
            {
            }

        private:
            auto update_async() -> void;

            static auto get_game_object(sbk_id gameObjectID) -> std::weak_ptr<sbk::core::database_object>;

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
            std::shared_ptr<spdlog::logger> m_logger;
        };
    }  // namespace engine
}  // namespace sbk