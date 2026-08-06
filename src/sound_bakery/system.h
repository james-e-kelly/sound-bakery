#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/core/memory/memory.h"
#include "sound_bakery/core/error/error.h"
#include "sound_bakery/core/task/command_queue.h"
#include "sound_bakery/core/task/executor.h"

namespace sbk
{
    class system_thread;

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
        class SB_CLASS system final : public sbk::core::object_owner,
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
            
            /**
             * @name Update functions.
             */
            /**@{*/

            /**
             * @brief Update called from the game thread.
             * 
             * Triggers the system thread to flush commands, if it exists.
             * If it doesn't, we are assumed to be in single-threaded mode, and this function flushes commands.
             */
            [[nodiscard]] auto update() -> sbk::result<void>;

            /**
             * @brief Flush all commands on the calling thread.
             * 
             * Normally, this is called by the system thread.
             */
            [[nodiscard]] auto flush_commands() -> sbk::result<void>;
            /**@}*/

            [[nodiscard]] auto get_project() const -> sbk::editor::project*;
            [[nodiscard]] auto get_runtime() const -> sbk::engine::runtime*;
            [[nodiscard]] auto get_voice_tracker() const -> sbk::engine::profiling::voice_tracker*;
            [[nodiscard]] auto get_current_object_owner() -> sbk::core::object_owner*;  //< Either an editor project or the runtime

            [[nodiscard]] auto get_command_queue() -> sbk::command_queue& { return m_commandQueue; }

            [[nodiscard]] auto get_game_executer() const -> sbk::executor*;
            [[nodiscard]] auto get_system_executor() const -> sbk::executor*;
            [[nodiscard]] auto get_worker_executor() const -> sbk::executor*;

            [[nodiscard]] auto get_default_memory_resource() noexcept -> sbk::memory::memory_resource* { return &m_generalResource; }

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


            bool m_registeredReflection = false;

            // Declared before any owned_ptr member below so their deleters (which call back
            // into these resources) run before the resources themselves are destroyed.
            sbk::memory::rpmalloc_resource m_generalResource{sbk::memory::object_category::system};
            sbk::memory::monotonic_buffer_resource m_systemArena{sbk::memory::object_category::system};

            sbk::command_queue m_commandQueue;                  //< Public API command queue

            sbk::owned_ptr<sbk::system_thread> m_systemThread;  //< The thread that calls @see flush_commands when triggered by @see update

            sbk::owned_ptr<sbk::editor::project> m_project;
            sbk::owned_ptr<sbk::engine::runtime> m_runtime;
            sbk::owned_ptr<profiling::voice_tracker> m_voiceTracker;

            sbk::owned_ptr<sbk::executor> m_gameExecutor;       //< Manual executor that runs during @r update
            sbk::owned_ptr<sbk::executor> m_systemExecutor;     //< Manual executor that runs off the system thread
            sbk::owned_ptr<sbk::executor> m_workerThread;       //< Worker thread for loading and decoding
        };
    }  // namespace engine
}  // namespace sbk