#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/util/fmod_pointers.h"

namespace SB::Core
{
    class Database;
    class ObjectTracker;
}  // namespace SB::Core

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
        static SC_SYSTEM* getChef();
        static System* create();
        static void destroy();

        static SB::Core::ObjectTracker* getObjectTracker();
        static SB::Core::Database* getDatabase();

    public:
        SB_RESULT init();
        SB_RESULT update();

        void onLoaded();

        void createMasterBus();
        const SB::Core::DatabasePtr<Bus>& getMasterBus() const { return m_masterBus; }
        void setMasterBus(const SB::Core::DatabasePtr<Bus>& bus);

        GameObject* getListenerGameObject() const;

    private:
        void registerReflectionTypes();
        void unregisterReflectionTypes();

    private:
        SB::SystemPtr m_chefSystem;

        std::unique_ptr<GameObject> m_listenerGameObject;
        SB::Core::DatabasePtr<Bus> m_masterBus;

        bool m_registeredReflection = false;

        std::unique_ptr<SB::Core::Database> m_database;
        std::unique_ptr<SB::Core::ObjectTracker> m_objectTracker;
        std::unique_ptr<Profiling::VoiceTracker> m_voiceTracker;
    };
}  // namespace SB::Engine