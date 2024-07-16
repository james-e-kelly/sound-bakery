#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/util/fmod_pointers.h"
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
        namespace Profiling
        {
            class VoiceTracker;
        }

        class Bus;
        class GameObject;

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

        public:
            system();
            ~system();

            static system* get();

            static system* create();
            static void destroy();

            static sc_result init();
            static sc_result update();

            /**
             * @brief Returns the current object owner. Either a project or soundbank runtime.
             */
            sbk::core::object_owner* current_object_owner();

            /**
             * @brief Creates an instance of Sound Bakery and opens the project.
             */
            static sc_result open_project(const std::filesystem::path& project_file);
            static sbk::editor::project* get_project();

            std::shared_ptr<concurrencpp::manual_executor> game_thread_executer() const { return m_gameThreadExecuter; }

            [[nodiscard]] sbk::engine::GameObject* get_listener_game_object() const { return m_listenerGameObject.get(); }
            [[nodiscard]] sbk::engine::Bus* get_master_bus() const { return m_masterBus.get(); }

            void set_master_bus(const std::shared_ptr<sbk::engine::Bus>& masterBus);

        private:
            bool m_registeredReflection = false;

            std::shared_ptr<sbk::engine::GameObject> m_listenerGameObject;
            std::shared_ptr<sbk::engine::Bus> m_masterBus;
            std::unique_ptr<sbk::editor::project> m_project;
            std::unique_ptr<Profiling::VoiceTracker> m_voiceTracker;
            std::shared_ptr<concurrencpp::manual_executor> m_gameThreadExecuter;
            std::shared_ptr<spdlog::logger> m_logger;
        };
    }  // namespace engine
}  // namespace sbk