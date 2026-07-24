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

        auto post_work(std::function<void()> work) -> sbk::result<> override
        {
            const std::lock_guard lock(m_mutex);
            SBK_CHECK_MSG(m_stopped == false, SBK_ERR_BAKERY, "Cannot enqueue command. Command queue shut down");
            m_staging.push_back(std::move(work));
            return sbk::ok();
        }

        /**
         * @brief Flush all tasks to the target executor.
         * @return SBK_SUCCESS if the command queue was empty, or successfully flushed
         */
        auto flush() -> sbk::result<> override
        {
            std::vector<std::function<void()>> batch;
            {
                const std::lock_guard lock(m_mutex);
                m_staging.swap(batch);
            }

            if (batch.empty())
            {
                return sbk::ok();
            }

            return m_target->post_work(
                [commands = std::move(batch)]() mutable
                {
                    const sbk::core::scoped_thread_domain studioDomain(sbk::core::thread_domain::studio);
                    for (auto& command : commands)
                    {
                        command();
                    }
                });
        }

        /**
         * @brief Moves all tasks to the target executor. That executor can then handle draining all tasks.
         */
        auto drain() -> sbk::result<> override
        {
            return flush();
        }

        /**
         * @brief Hand any staged commands to the target so none are dropped, then refuse further work.
         *
         * The target executor must still be alive: shut down command queues before the executors they
         * flush into.
         */
        auto shutdown() -> void override
        {
            (void)flush();
            const std::lock_guard lock(m_mutex);
            m_stopped = true;
        }

    private:
        executor*                          m_target;
        std::mutex                         m_mutex;
        std::vector<std::function<void()>> m_staging;
        bool                               m_stopped = false;
        friend class ::sbk::engine::system;
    };
}
