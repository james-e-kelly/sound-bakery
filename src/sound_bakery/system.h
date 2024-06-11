#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/util/fmod_pointers.h"
#include "spdlog/sinks/basic_file_sink.h"

namespace SB::Core
{
    class database;
    class ObjectTracker;
}  // namespace SB::Core

namespace SB::Editor
{
    class Project;
}  // namespace SB::Editor

namespace SB::Engine
{
    class Bus;
    class GameObject;
    class System;

    namespace Profiling
    {
        class VoiceTracker;
    }

    /**
     * @brief Manager of the whole Sound Bakery.
     */
    class SB_CLASS System final
    {
        REGISTER_REFLECTION(System)

    public:
        System();
        ~System();

    public:
        static System* get();
        static sc_system* getChef();

        static System* create();
        static void destroy();

        static sc_result init();
        static sc_result update();

        /**
         * @brief Creates an instance of Sound Bakery and opens the project.
         */
        static sc_result openProject(const std::filesystem::path& projectFile);

        static SB::Core::ObjectTracker* getObjectTracker();
        static SB::Core::database* getDatabase();
        static SB::Editor::Project* getProject();

        void onLoaded();

        void createMasterBus();
        const SB::Core::DatabasePtr<Bus>& getMasterBus() const { return m_masterBus; }
        void setMasterBus(const SB::Core::DatabasePtr<Bus>& bus);

        GameObject* getListenerGameObject() const;

        std::shared_ptr<concurrencpp::thread_pool_executor> getBackgroundExecuter() const
        {
            return concurrenRuntime.background_executor();
        }

        std::shared_ptr<concurrencpp::manual_executor> getMainThreadExecutuer() const { return mainThreadExecuter; }

    private:
        SB::SystemPtr m_chefSystem;

        std::unique_ptr<GameObject> m_listenerGameObject;
        SB::Core::DatabasePtr<Bus> m_masterBus;

        bool m_registeredReflection = false;

        std::unique_ptr<SB::Editor::Project> m_project;
        std::unique_ptr<SB::Core::database> m_database;
        std::unique_ptr<SB::Core::ObjectTracker> m_objectTracker;
        std::unique_ptr<Profiling::VoiceTracker> m_voiceTracker;

        concurrencpp::runtime concurrenRuntime;
        std::shared_ptr<concurrencpp::manual_executor> mainThreadExecuter;

        std::shared_ptr<spdlog::logger> logger;
    };
}  // namespace SB::Engine