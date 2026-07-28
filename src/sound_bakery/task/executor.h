#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/error/result.h"

namespace sbk
{
    /**
     * @brief Abstract execution context: somewhere coroutines and work items can be posted to run.
     *
     * The idea behind the executor is to make async coroutines easy. The system holds its executors
     * by base pointer, so a domain (game, studio, loading, ...) can be backed by a real OS thread
     * (@r thread_executor), a deferred command channel (@r command_queue), or -- in single-threaded /
     * editor mode -- simply an alias of another domain, e.g. `m_studioThread = m_gameThread`.
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
         * @brief Enqueue a coroutine to be resumed on this executor.
         * @param h coroutine handle
         * @return SBK_SUCCESS if successfully enqueued
         */
        auto post(std::coroutine_handle<> h) -> sbk::result<>
        {
            return post_work(
                [h]
                {
                    ZoneScopedN("coroutine resume");
                    h.resume();
                });
        }

        /**
         * @brief Enqueue a generic function to run on this executor. The one operation a concrete executor must provide.
         * @param work function to execute
         * @return SBK_SUCCESS if successfully enqueued, otherwise a failure the caller can forward
         */
        virtual auto post_work(std::function<void()> work) -> sbk::result<> = 0;

        /**
         * @brief For executors like the @r command_queue, moves all tasks to the target executor.
         */
        virtual auto flush() -> sbk::result<> { return sbk::ok(); }

        /**
         * @brief Drain all tasks and run them now. Can do nothing on certain executors.
         */
        virtual auto drain() -> sbk::result<> { return sbk::ok(); }

        /**
         * @brief Finish and stop the executor. Overridden by executors that own work or a thread; by default there is nothing to shut down.
         *
         * Implementations must run every task still queued (including continuations those tasks post)
         * to completion rather than dropping them, and must be idempotent -- the system may call
         * shutdown() explicitly and again from the destructor, and aliased domains may share one object.
         */
        virtual auto shutdown() -> void {}

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
             *
             * Templated on the awaiting coroutine's promise so that, when Tracy is enabled and that
             * promise carries a fiber name (every @r task / @r detached_task does, via @r fiber_promise),
             * the resume is bracketed by @r TracyFiberEnter / @r TracyFiberLeave. The coroutine then
             * shows up in Tracy as its own fiber lane, staying continuous as it hops between executors.
             * Promises without a fiber_name(), and non-Tracy builds, take the plain @r post path.
             */
            template <class Promise>
            auto await_suspend(std::coroutine_handle<Promise> h) const -> bool
            {
#ifdef TRACY_ENABLE
                if constexpr (requires { h.promise().fiber_name(); })
                {
                    const char* const fiberName = h.promise().fiber_name();
                    return exec
                        ->post_work(
                            [h, fiberName]
                            {
                                TracyFiberEnter(fiberName);
                                h.resume();
                                TracyFiberLeave;
                            })
                        .has_value();
                }
#else
                return exec->post(h).has_value();
#endif
            }

            auto await_resume() const noexcept -> void {}
        };

        // Route onto this executor only if we are not already on it.
        [[nodiscard]] auto schedule() -> schedule_awaiter { return schedule_awaiter{.exec = this, .force = false}; }

        // Always bounce through the queue, even when already on this executor.
        [[nodiscard]] auto yield() -> schedule_awaiter { return schedule_awaiter{.exec = this, .force = true}; }

    protected:
        // The executor's name, used by concrete executors e.g. to name their OS thread in Tracy.
        [[nodiscard]] auto name() const noexcept -> const std::string& { return m_name; }

    private:
        std::string m_name;
    };
}  // namespace sbk
