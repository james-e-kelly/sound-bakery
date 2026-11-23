#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/object/object_owner.h"

namespace sbk
{
	class system_thread final : public sbk::core::object_owner
	{
    public:
        system_thread()
        {
            m_thread = std::thread([this]
                                   { run_loop(); });
        }

        ~system_thread() override
        {
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        auto stop() -> void;
        auto trigger_update() -> void;

    private:
        auto run_loop() -> void;

        std::mutex m_mutex;
        std::condition_variable m_condition;
        bool m_notify  = false;
        bool m_abandon = false;
        std::thread m_thread;
	};
}