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
                                      public concurrencpp::runtime
        {
            REGISTER_REFLECTION(system)
            NOT_COPYABLE(system)
            LEAK_DETECTOR(system)

        public:
            system();
            ~system();

            static auto create() -> system*;
            static auto init(const sb_system_config& config) -> sc_result;
            static auto update() -> sc_result;
            static auto destroy() -> void;

            [[nodiscard]] static auto get() -> system*;
            [[nodiscard]] static auto get_project() -> sbk::editor::project*;
            [[nodiscard]] static auto get_voice_tracker() -> sbk::engine::profiling::voice_tracker*;
            [[nodiscard]] auto get_game_thread_executer() const -> std::shared_ptr<concurrencpp::manual_executor>;
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

            auto set_master_bus(const std::shared_ptr<sbk::engine::bus>& masterBus) -> void;

        private:
            bool m_registeredReflection = false;

            std::shared_ptr<sbk::engine::game_object> m_listenerGameObject;
            std::shared_ptr<sbk::engine::bus> m_masterBus;
            std::unique_ptr<sbk::editor::project> m_project;
            std::unique_ptr<profiling::voice_tracker> m_voiceTracker;
            std::shared_ptr<concurrencpp::manual_executor> m_gameThreadExecuter;
            std::shared_ptr<spdlog::logger> m_logger;
        };
    }  // namespace engine
}  // namespace sbk