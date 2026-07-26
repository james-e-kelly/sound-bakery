#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/error/error.h"
#include "sound_bakery/task/executor.h"

#include "core/logger.h"

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
                unkown,  //< Unkown/unset
                editor,  //< We have a project
                runtime  //< We are loading soundbanks
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
            [[nodiscard]] auto get_operating_mode() -> operating_mode;  //< @todo Remove this. Users should just try and get the objects they want
            [[nodiscard]] auto get_project() const -> sbk::editor::project*;
            [[nodiscard]] auto get_voice_tracker() const -> sbk::engine::profiling::voice_tracker*;
            [[nodiscard]] auto get_game_executer() const -> std::shared_ptr<sbk::executor>;
            [[nodiscard]] auto get_system_executer() const -> std::shared_ptr<sbk::executor>;
            [[nodiscard]] auto get_worker_executer() const -> std::shared_ptr<sbk::executor>;
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

            friend class boost::serialization::access;

            template <class archive_class>
            auto serialize(archive_class& archive, const unsigned int version) -> void
            {
            }

        private:
            auto update_async() -> void;

            bool m_registeredReflection = false;
            bool m_initSoundChef        = false;

            std::weak_ptr<sbk::engine::game_object> m_listenerGameObject;
            std::weak_ptr<sbk::engine::bus> m_masterBus;
            std::unique_ptr<sbk::editor::project> m_project;
            std::unique_ptr<profiling::voice_tracker> m_voiceTracker;

            std::shared_ptr<sbk::executor> m_gameExecutor;    //< Manual executor that runs during @r update
            std::shared_ptr<sbk::executor> m_systemExecutor;  //< Command queue that either flushes to a worker thread or the game thread for single threaded mode
            std::shared_ptr<sbk::executor> m_systemThread;    //< System worker thread. Can be null if in single threaded mode
            std::shared_ptr<sbk::executor> m_workerThread;    //< Worker thread for loading and decoding
        };
    }  // namespace engine
}  // namespace sbk