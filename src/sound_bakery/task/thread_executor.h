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
            if (m_thread.joinable())
            {
                m_thread.join();
            }
        }

        /**
         * @brief Enqueue an item on this executor's thread.
         * @return SBK_SUCCESS if successfully enqueued
         * @return SBK_ERR_BAKERY if the worker has finished exiting and can no longer run work
         */
        auto enqueue(work_item item) -> sbk::result<> override
        {
            {
                const std::lock_guard lock(m_mutex);
                SBK_CHECK_MSG(m_stopped == false, SBK_ERR_BAKERY, "Cannot enqueue work. Executor shut down");
                m_queue.push(std::move(item));
            }
            m_cv.notify_one();
            return sbk::ok();
        }

        /**
         * @brief Finish the item already running, drop the rest of the queue, then join.
         */
        auto abandon() -> void override
        {
            {
                const std::lock_guard lock(m_mutex);
                if (m_stop)
                {
                    return;
                }
                m_stop    = true;
                m_abandon = true;
            }
            m_cv.notify_one();
            if (m_thread.joinable())
            {
                m_thread.join();
            }

            const std::lock_guard lock(m_mutex);
            std::queue<work_item> dropped;
            m_queue.swap(dropped);
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
                work_item work;
                {
                    std::unique_lock lock(m_mutex);
                    m_cv.wait(lock, [this]
                              { return m_stop || !m_queue.empty(); });
                    if (m_abandon)
                    {
                        m_stopped = true;
                        break;
                    }
                    if (m_queue.empty())
                    {
                        if (m_stop)
                        {
                            m_stopped = true;
                            break;
                        }
                        continue;
                    }
                    work = std::move(m_queue.front());
                    m_queue.pop();
                }

                ZoneScopedN("thread_executor dispatch");
                work();
            }

            sbk::memory::thread_end(name());
        }

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::queue<work_item> m_queue;
        std::thread::id m_id;
        bool m_started = false;
        bool m_stop    = false;  // stop requested (shutdown or abandon)
        bool m_abandon = false;  // stop requested via abandon: drop the backlog rather than draining it
        bool m_stopped = false;  // worker has exited; no longer accepts work
        std::thread m_thread;
    };
}  // namespace sbk
