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
        class runtime;

        /**
         * @brief Manager of the whole Sound Bakery.
         *
         * The system tracks all objects created during Sound Bakery's lifetime.
         *
         * It owns all loaded Soundbanks, listener game object, and busses.
         */
        class SB_CLASS system final : public sc_system,
                                      public sbk::core::object_owner,
                                      public sbk::core::logger,
                                      public sbk::core::object_tracker,
                                      public sbk::core::database,
                                      public boost::noncopyable
        {
            REGISTER_REFLECTION(system)
            LEAK_DETECTOR(system)

        public:
            /**
             * @name Constructors for the system object.
             * 
             * These are not intended to be called by users. Sound Bakery currently assumes there is a single system object.
             */
            /**@{*/
            system();
            system(const std::filesystem::path& logFile);
            system(sbk::core::sbk_log_callback_proc logCallback);
            ~system();
            /**@}*/

            /**
             * @name The static system functions.
             * 
             * Users should call these functions to create, get, and destroy the static system object.
             * 
             * It is expected that users use get() whenever they need to access the system.
             */
            /**@{*/
            [[nodiscard]] static auto create() -> sbk::result<void>;
            [[nodiscard]] static auto create(const std::filesystem::path& logFile) -> sbk::result<void>;
            [[nodiscard]] static auto create(sbk::core::sbk_log_callback_proc logCallback) -> sbk::result<void>;
            [[nodiscard]] static auto get() -> system*;
            static auto destroy() -> void;
            /**@}*/

            [[nodiscard]] auto init(const sbk_system_config& config) -> sbk::result<void>;
            [[nodiscard]] auto update() -> sbk::result<void>;

            [[nodiscard]] auto get_project() const -> sbk::editor::project*;
            [[nodiscard]] auto get_runtime() const -> sbk::engine::runtime*;
            [[nodiscard]] auto get_voice_tracker() const -> sbk::engine::profiling::voice_tracker*;
            [[nodiscard]] auto get_current_object_owner() -> sbk::core::object_owner*;  //< Either an editor project or the runtime

            [[nodiscard]] auto get_game_executer() const -> std::shared_ptr<sbk::executor>;
            [[nodiscard]] auto get_system_executer() const -> std::shared_ptr<sbk::executor>;
            [[nodiscard]] auto get_worker_executer() const -> std::shared_ptr<sbk::executor>;

            [[nodiscard]] auto create_project() -> sbk::result<sbk::editor::project*>;

            friend class boost::serialization::access;

            template <class archive_class>
            auto serialize(archive_class& archive, const unsigned int version) -> void
            {
                (void)archive;
                (void)version;
            }

        private:
            // Ideally, no one should use the system publically to create objects.
            // The system owns objects it creates itself and is used as the top-most owner of objects.
            using object_owner::create_raw_object;
            using object_owner::create_runtime_object;
            using object_owner::create_database_object;

            auto update_async() -> void;

            bool m_registeredReflection = false;

            sbk::owned_ptr<sbk::editor::project> m_project;
            sbk::owned_ptr<sbk::engine::runtime> m_runtime;
            sbk::owned_ptr<profiling::voice_tracker> m_voiceTracker;

            std::shared_ptr<sbk::executor> m_gameExecutor;    //< Manual executor that runs during @r update
            std::shared_ptr<sbk::executor> m_systemExecutor;  //< Command queue that either flushes to a worker thread or the game thread for single threaded mode
            std::shared_ptr<sbk::executor> m_systemThread;    //< System worker thread. Can be null if in single threaded mode
            std::shared_ptr<sbk::executor> m_workerThread;    //< Worker thread for loading and decoding
        };
    }  // namespace engine
}  // namespace sbk