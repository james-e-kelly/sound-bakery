#pragma once

#include "sound_bakery/pch.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/task/executor.h"

namespace sbk
{
    /**
     * @brief An @r executor with no thread of its own: work is queued, then run all at once by whoever calls @r drain.
     *
     * This is for domains whose thread Sound Bakery does not own -- above all the game thread. The
     * game posts commands and callbacks throughout its frame, then calls drain() inside update() to
     * run through all of them "there and then", synchronously, on the game thread.
     *
     * While draining, on_this_thread() reports true for the draining thread, so a game-thread-affine
     * function reached from inside the pump (`co_await gameExecutor->schedule()`) runs inline instead
     * of deferring to the next drain.
     */
    class manual_executor : public executor
    {
    public:
        manual_executor(std::string name) : executor(name) {}

        /**
         * @brief Queue a function to run at the next @r drain.
         * @param work function to execute
         * @return SBK_SUCCESS if queued
         * @return SBK_ERR_BAKERY if the executor has been shut down
         */
        auto post_work(std::function<void()> work) -> sbk::result<> override
        {
            const std::lock_guard lock(m_mutex);
            SBK_CHECK_MSG(m_stopped == false, SBK_ERR_BAKERY, "Cannot enqueue work. Executor shut down");
            m_queue.push(std::move(work));
            return sbk::ok();
        }

        [[nodiscard]] auto on_this_thread() const noexcept -> bool override
        {
            return std::this_thread::get_id() == m_drainThread.load(std::memory_order_relaxed);
        }

        /**
         * @brief Run all queued work on the calling thread. Call this from the game's update().
         *
         * Work queued during the drain is included (the loop runs until the queue is empty), so a
         * task's continuations finish within the same pump rather than waiting for the next one.
         *
         * @return the number of work items run
         */
        auto drain() -> sbk::result<> override
        {
            m_drainThread.store(std::this_thread::get_id(), std::memory_order_relaxed);

            for (;;)
            {
                std::function<void()> work;
                {
                    const std::lock_guard lock(m_mutex);
                    if (m_queue.empty())
                    {
                        break;
                    }
                    work = std::move(m_queue.front());
                    m_queue.pop();
                }
                work();
            }

            m_drainThread.store(std::thread::id{}, std::memory_order_relaxed);
            return sbk::ok();
        }

        /**
         * @brief Finish everything still queued (a final @r drain on the calling thread), then refuse
         *        further work. Idempotent.
         */
        auto shutdown() -> void override
        {
            (void)drain();
            const std::lock_guard lock(m_mutex);
            m_stopped = true;
        }

    private:
        std::mutex                        m_mutex;
        std::queue<std::function<void()>> m_queue;
        std::atomic<std::thread::id>      m_drainThread{};
        bool                              m_stopped = false;
    };
}
