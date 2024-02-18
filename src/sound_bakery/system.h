#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/util/fmod_pointers.h"

namespace SB::Engine
{
    class GameObject;
    class Bus;

    namespace Profiling
    {
        class VoiceTracker;
    }

    /**
     * @brief Manager of the whole Sound Bakery.
     */
    class System final
    {
    public:
        System();

    public:
        static System* get();
        static SC_SYSTEM* getChef();
        static System* create();
        static void destroy();

    public:
        SB_RESULT init();
        SB_RESULT update();

        void onLoaded();

        void createMasterBus();

    public:
        const SB::Core::DatabasePtr<Bus>& getMasterBus() const { return m_masterBus; }

        void setMasterBus(const SB::Core::DatabasePtr<Bus>& bus);

        GameObject* getListenerGameObject() const;

    private:
        SB::SystemPtr m_chefSystem;
        std::unique_ptr<GameObject> m_listenerGameObject;
        SB::Core::DatabasePtr<Bus> m_masterBus;

        // Profiling

    private:
        std::unique_ptr<Profiling::VoiceTracker> m_voiceTracker;

        RTTR_ENABLE()
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine