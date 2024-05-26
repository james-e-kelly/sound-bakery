#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/util/fmod_pointers.h"

namespace SB::Core
{
    class Database;
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

        static SB_RESULT init();
        static SB_RESULT update();

        static SB_RESULT openProject(const std::filesystem::path& projectFile);

        static SB::Core::ObjectTracker* getObjectTracker();
        static SB::Core::Database* getDatabase();
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

    private:
        SB::SystemPtr m_chefSystem;

        std::unique_ptr<GameObject> m_listenerGameObject;
        SB::Core::DatabasePtr<Bus> m_masterBus;

        bool m_registeredReflection = false;

        std::unique_ptr<SB::Editor::Project> m_project;
        std::unique_ptr<SB::Core::Database> m_database;
        std::unique_ptr<SB::Core::ObjectTracker> m_objectTracker;
        std::unique_ptr<Profiling::VoiceTracker> m_voiceTracker;

        concurrencpp::runtime concurrenRuntime;
    };
}  // namespace SB::Engine