#include "system_thread.h"

#include "sound_bakery/core/memory/memory.h"
#include "sound_bakery/system.h"

auto sbk::system_thread::stop() -> void
{
    {
        const std::lock_guard lock(m_mutex);
        m_abandon = true;
    }
    m_condition.notify_one();
}

auto sbk::system_thread::trigger_update() -> void
{
    {
        const std::lock_guard lock(m_mutex);
        m_notify = true;
    }
    m_condition.notify_one();
}

auto sbk::system_thread::run_loop() -> void
{
    sbk::memory::thread_start("Sound Bakery System Thread");

    for (;;)
    {
        {
            std::unique_lock lock(m_mutex);
            m_condition.wait(lock, [this] { return m_notify || m_abandon; });

            if (m_abandon)
            {
                break;
            }

            m_notify = false;
        }

        if (sbk::engine::system* const system = sbk::engine::system::get())
        {
            (void)system->flush_commands();
        }
    }

    sbk::memory::thread_end("Sound Bakery System Thread");
}
