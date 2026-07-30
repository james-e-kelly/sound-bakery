#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/task/executor.h"

namespace sbk
{
    /**
     * @brief Queues tasks until it is "flushed" onto another executor.
     *
     * For Sound Bakery, this means queuing all commands from the game thread, or any thread, then flushing it to the system thread.
     */
    class command_queue : public executor
    {
    public:
        command_queue(std::string name) : executor(name) {}

        auto enqueue(work_item item) -> sbk::result<> override
        {
            const std::lock_guard lock(m_mutex);
            SBK_CHECK_MSG(m_stopped == false, SBK_ERR_BAKERY, "Cannot enqueue command. Command queue shut down");
            m_staging.push_back(std::move(item));
            return sbk::ok();
        }

        /**
         * @brief Flush all tasks to the target executor.
         * @return SBK_SUCCESS if the command queue was empty, or successfully flushed
         */
        auto flush() -> sbk::result<> override
        {
            ZoneScopedN("command_queue flush");

            std::vector<work_item> batch;
            {
                const std::lock_guard lock(m_mutex);
                m_staging.swap(batch);
            }

            if (batch.empty())
            {
                return sbk::ok();
            }

            return m_target->enqueue(work_item{
                [commands = std::move(batch)]() mutable
                {
                    const sbk::core::scoped_thread_domain studioDomain(sbk::core::thread_domain::studio);
                    for (auto& command : commands)
                    {
                        command();
                    }
                }});
        }

        /**
         * @brief Drop all staged commands and refuse further work. Does not flush.
         */
        auto abandon() -> void override
        {
            const std::lock_guard lock(m_mutex);
            m_stopped = true;
            std::vector<work_item> dropped;
            m_staging.swap(dropped);
        }

    private:
        executor* m_target;
        std::mutex m_mutex;
        std::vector<work_item> m_staging;
        bool m_stopped = false;
        friend class ::sbk::engine::system;
    };
}  // namespace sbk
