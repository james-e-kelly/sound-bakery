#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/error/result.h"
#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/core/task/work_item.h"

namespace sbk
{
    /**
     * @brief Abstract executor.
     *
     * Sound Bakery uses coroutines to make async programming easy to think about.
     * It also uses coroutines so thread pools are easy to design and for us to utilise all system resources.
     * 
     * Executors are what lets coroutines uses threads in an easy way.
     * Most coroutines eventually end up on a @r thread_executor. Thread executors are simple executors that just run all tasks.
     * Other executors, like the @r command_queue, allow work to be queued up (likely from the game) and moved to a @r thread_executor at a defined time.
     *
     * A concrete executor implements one thing: @r post_work for a generic work item. Posting a
     * coroutine, and the schedule()/yield() thread-domain guards, are built on top of it.
     *
     * From this, most functions are expected to return @r sbk::async_result and set themselves up to
     * run on their desired thread via `co_await executor->schedule()`.
     */
    class executor : public sbk::core::object_owner // Object owner so the system can create it as an owned object
    {
    public:
        executor() = delete;
        executor(std::string name) : m_name(std::move(name)) {}
        virtual ~executor() = default;

        executor(const executor&)                    = delete;
        executor(executor&&)                         = delete;
        auto operator=(const executor&) -> executor& = delete;
        auto operator=(executor&&) -> executor&      = delete;

        /**
         * @brief Take ownership of a work item and arrange for it to run. The one operation a concrete executor MUST provide.
         * @return SBK_SUCCESS if successfully enqueued, otherwise a failure the caller can forward
         */
        virtual auto enqueue(work_item item) -> sbk::result<> = 0;

        /**
         * @brief Enqueue a generic callable to run on this executor.
         * @return SBK_SUCCESS if successfully enqueued
         */
        auto post_work(std::function<void()> work) -> sbk::result<> { return enqueue(work_item{std::move(work)}); }

        /**
         * @brief Enqueue a coroutine to be resumed on this executor.
         * @return SBK_SUCCESS if successfully enqueued
         */
        template <class Promise>
        auto post(std::coroutine_handle<Promise> h) -> sbk::result<>
        {
            [[maybe_unused]] const char* fiberName = nullptr;
#ifdef TRACY_ENABLE
            if constexpr (requires { h.promise().fiber_name(); })
            {
                fiberName = h.promise().fiber_name();
            }
#endif
            if constexpr (promise_self_owns_frame_v<Promise>)
            {
                return enqueue(work_item::resume_owning(h, fiberName));
            }
            else
            {
                return enqueue(work_item::resume_borrowed(h, fiberName));
            }
        }

        /**
         * @brief For executors like the @r command_queue, moves all tasks to the target executor.
         */
        virtual auto flush() -> sbk::result<> { return sbk::ok(); }

        /**
         * @brief For executors like the @r manual_executor, drain all tasks and run them now.
         */
        virtual auto drain() -> sbk::result<> { return sbk::ok(); }

        /**
         * @brief Stop the executor. Drop everything still queued and refuse further work.
         */
        virtual auto abandon() -> void {}

        /**
         * @brief Whether the caller is already running "on" this executor.
         * @return true if the calling code is on this executor's thread
         * @return false by default -- executors with no affine thread (e.g. @r command_queue) never
         *         short-circuit, so scheduling always routes through @r post_work
         */
        [[nodiscard]] virtual auto on_this_thread() const noexcept -> bool { return false; }

        /**
         * @brief Awaitable that puts a coroutine on this executor.
         *
         * Use @r schedule to move a coroutine onto this executor, if it is not already on it. On a
         * @r thread_executor that is already the current thread this does not suspend, so the
         * coroutine runs on synchronously and inline. On a deferred @r command_queue,
         * on_this_thread() is always false, so schedule() means "resume on the target at the next
         * flush()".
         *
         * Use @r yield to always suspend and re-post, even when already on this executor. This is to
         * split up long work into fair chunks, where schedule() would no-op.
         */
        struct schedule_awaiter
        {
            executor* exec = nullptr;
            bool force     = false;  // true = always reschedule (yield())

            /**
             * @return true if the coroutine should run on the current thread without suspending
             * @return false if the coroutine should suspend and be re-posted
             */
            [[nodiscard]] auto await_ready() const noexcept -> bool
            {
                return !force && exec->on_this_thread();
            }

            /**
             * @return true if the job was enqueued (suspend)
             * @return false if the job failed to enqueue and should run synchronously instead of
             *         hanging forever on a thread that will never run it
             */
            template <class Promise>
            auto await_suspend(std::coroutine_handle<Promise> h) const -> bool
            {
                return exec->post(h).has_value();
            }

            auto await_resume() const noexcept -> void {}
        };

        /**
         * @brief Schedule a coroutine to run on this executor by co_awaiting the result.
         * 
         * If the caller is already on the executor's thread, the coroutine does not suspend.
         */
        [[nodiscard]] auto schedule() -> schedule_awaiter { return schedule_awaiter{.exec = this, .force = false}; }

        // Always bounce through the queue, even when already on this executor.

        /**
         * @brief Suspend and yield the current coroutine. The coroutine is queued to run on this executor at a later date.
         */
        [[nodiscard]] auto yield() -> schedule_awaiter { return schedule_awaiter{.exec = this, .force = true}; }

    protected:
        [[nodiscard]] auto name() const noexcept -> const std::string& { return m_name; }

    private:
        std::string m_name;
    };
}  // namespace sbk
