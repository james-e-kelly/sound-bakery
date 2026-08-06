#include "system_thread.h"

#include "sound_bakery/system.h"

auto sbk::system_thread::run_loop() -> void
{
    for (;;)
    {
        if (m_abandon.load(std::memory_order_relaxed) == true)
        {
            return;
        }

        if (m_notify.load(std::memory_order_relaxed))
        {
            if (sbk::engine::system* const system = sbk::engine::system::get())
            {
                (void)system->flush_commands();
            }

            m_notify.store(false, std::memory_order_relaxed);
        }
    }
}
