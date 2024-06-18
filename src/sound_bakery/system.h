#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/util/fmod_pointers.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace sbk::editor
{
    class project;
}  // namespace sbk::editor

namespace sbk::engine
{
    namespace Profiling
    {
        class VoiceTracker;
    }

    /**
     * @brief Manager of the whole Sound Bakery.
     */
    class SB_CLASS system final : public sc_system,
                                  public concurrencpp::runtime,
                                  public sbk::core::object_tracker,
                                  public sbk::core::database
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
         * @brief Creates an instance of Sound Bakery and opens the project.
         */
        static sc_result open_project(const std::filesystem::path& projectFile);
        static sbk::editor::project* get_project();

        std::shared_ptr<concurrencpp::manual_executor> game_thread_executer() const { return m_gameThreadExecuter; }

    private:
        bool m_registeredReflection = false;

        std::unique_ptr<sbk::editor::project> m_project;
        std::unique_ptr<Profiling::VoiceTracker> m_voiceTracker;
        std::shared_ptr<concurrencpp::manual_executor> m_gameThreadExecuter;
        std::shared_ptr<spdlog::logger> m_logger;
    };
}  // namespace sbk::engine