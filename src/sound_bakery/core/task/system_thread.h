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

        auto stop() -> void
        {
            m_abandon.store(true, std::memory_order_relaxed);
        }

        auto trigger_update() -> void
        {
            m_notify.store(true, std::memory_order_relaxed);
        }

    private:
        auto run_loop() -> void;

        std::thread m_thread;
        std::atomic_bool m_notify{};
        std::atomic_bool m_abandon{};
	};
}