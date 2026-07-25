#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/memory.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/task/executor.h"

namespace sbk
{
    /**
     * @brief An @r executor that owns one OS thread and runs posted work as soon as it arrives.
     *
     * There is no load balancing, only a single thread.
     *
     * The executor knows its thread id and can run coroutines synchronously, if it is already on the
     * executor's thread. Otherwise, @r schedule (on the base) suspends the coroutine and enqueues it.
     */
    class thread_executor : public executor
    {
    public:
        thread_executor(std::string name) : executor(name)
        {
            m_thread = std::thread([this]
                                   { run_loop(); });

            // Block until the worker has recorded its own thread id, so that
            // on_this_thread() is reliable the instant the constructor returns.
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this]
                      { return m_started; });
        }

        ~thread_executor() override
        {
            shutdown();
        }

        /**
         * @brief Enqueue a generic function on this executor's thread.
         * @param work function to execute
         * @return SBK_SUCCESS if successfully enqueued
         * @return SBK_ERR_BAKERY if the executor has finished shutting down and can no longer run work
         *
         * Note the guard is @r m_stopped, not @r m_stop: while a shutdown() is draining the queue,
         * tasks are still allowed to post their continuations so they can finish. Only once the worker
         * has actually exited (queue empty) does enqueuing fail.
         */
        auto post_work(std::function<void()> work) -> sbk::result<> override
        {
            {
                const std::lock_guard lock(m_mutex);
                SBK_CHECK_MSG(m_stopped == false, SBK_ERR_BAKERY, "Cannot enqueue work. Executor shut down");
                m_queue.push(std::move(work));
            }
            m_cv.notify_one();
            return sbk::ok();
        }

        /**
         * @brief Finish all queued work, then stop the thread.
         *
         * Sets the drain flag and lets the worker process everything still in the queue -- including
         * continuations posted by those tasks -- before joining. Nothing queued is dropped. Idempotent.
         */
        auto shutdown() -> void override
        {
            {
                const std::lock_guard lock(m_mutex);
                if (m_stop)
                {
                    return;  // already draining / shut down
                }
                m_stop = true;
            }
            m_cv.notify_one();
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        [[nodiscard]] auto on_this_thread() const noexcept -> bool override
        {
            return std::this_thread::get_id() == m_id;
        }

    private:
        auto run_loop() -> void
        {
            sbk::memory::thread_start(name());
            tracy::SetThreadName(name().c_str());

            {
                const std::lock_guard lock(m_mutex);
                m_id      = std::this_thread::get_id();
                m_started = true;
            }
            m_cv.notify_all();  // release the constructor waiting on m_started

            for (;;)
            {
                std::function<void()> work;
                {
                    std::unique_lock lock(m_mutex);
                    m_cv.wait(lock, [this]
                              { return m_stop || !m_queue.empty(); });
                    if (m_queue.empty())
                    {
                        if (m_stop)
                        {
                            m_stopped = true;  // set under lock: any later post_work is rejected
                            break;             // lock released by unique_lock's destructor as we leave the scope
                        }
                        continue;
                    }
                    work = std::move(m_queue.front());
                    m_queue.pop();
                }

                // A busy slice on this named thread lane per work item. Any task fiber the item
                // resumes enters and leaves within work(), so this zone opens and closes in the
                // thread's own context -- it nests cleanly around the fiber excursion.
                ZoneScopedN("thread_executor dispatch");
                work();
            }

            // Worker is exiting: tear down rpmalloc's per-thread heap. Runs with no lock held.
            sbk::memory::thread_end(name());
        }

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<std::function<void()>> m_queue;
        std::thread::id m_id;
        bool m_started = false;
        bool m_stop    = false;  // drain requested (shutdown called)
        bool m_stopped = false;  // worker has exited; no longer accepts work
        std::thread m_thread;
    };
}  // namespace sbk
