#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/core/error/result.h"
#include "sound_bakery/core/memory/memory.h"
#include "sound_bakery/core/task/unique_coroutine.h"

namespace sbk
{
    template <class T>
    class task;

    class promise_base
    {
    public:
        promise_base()    = default;
        ~promise_base() = default;

        static void* operator new(std::size_t size)
        {
            return sbk::memory::malloc(size, sbk::memory::default_alignment, sbk::memory::object_category::data);
        }

        static void operator delete(void* pointer)
        {
            sbk::memory::free(pointer, sbk::memory::object_category::data);
        }
    };

    // ---------------------------------------------------------------------------
    // Tracy fibers
    //
    // A scheduled coroutine can migrate between executor threads across its
    // co_await hops, so ordinary per-thread zones can never show its true
    // timeline. Tracy models this with "fibers": each coroutine gets its own lane
    // and an executor marks when it enters (resumes) and leaves (suspends) that
    // lane -- see @r executor::schedule_awaiter.
    //
    // Tracy identifies a fiber by the *address* of its name and reads the text on
    // demand, so the name must have a stable, unique address for as long as the
    // coroutine lives. The coroutine frame is exactly that owner, so we hang the
    // name off the promise. Mixed into every task/detached promise; empty (and
    // free) when Tracy is compiled out.
    // ---------------------------------------------------------------------------
#ifdef TRACY_ENABLE
    struct fiber_promise
    {
        // Minted lazily the first time an executor schedules this coroutine, so a
        // task that never hops threads pays nothing. The counter only has to make
        // the lane unique; it is not tied to the source function.
        [[nodiscard]] auto fiber_name() -> const char*
        {
            if (m_fiberName.empty())
            {
                static std::atomic<std::uint64_t> counter{0};
                m_fiberName = fmt::format("task {}", counter.fetch_add(1, std::memory_order_relaxed));
            }
            return m_fiberName.c_str();
        }

    private:
        std::string m_fiberName;
    };
#else
    struct fiber_promise
    {
    };
#endif

    // When a task finishes, it resumes whoever awaited it (its "continuation"),
    // via symmetric transfer. If nobody is waiting, park on noop_coroutine so
    // resume() returns cleanly to the executor loop.
    struct final_awaiter
    {
        [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

        template <class Promise>
        [[nodiscard]] auto await_suspend(std::coroutine_handle<Promise> self) const noexcept -> std::coroutine_handle<>
        {
            auto cont = self.promise().continuation;
            return cont ? cont : std::noop_coroutine();
        }

        auto await_resume() const noexcept -> void {}
    };

    template <class T>
    struct task_promise : public promise_base, fiber_promise
    {
        result<T> outcome;                       // filled by co_return
        std::coroutine_handle<> continuation{};  // who awaited us

        auto get_return_object() noexcept -> task<T>;
        auto initial_suspend() noexcept -> std::suspend_always { return {}; }  // LAZY
        auto final_suspend() noexcept -> final_awaiter { return {}; }

        // Two ways to finish:
        //   co_return result<T>{...} / co_return T{...}  -- normal completion
        //   co_return some_error;                        -- error propagation
        // The second overload is what SBK_CO_TRY targets: on a failed await it
        // does `co_return tmp.error();`, so an inner failure flows outward with
        // no exception in sight.
        auto return_value(result<T> r) noexcept -> void { outcome = std::move(r); }
        auto return_value(error e) noexcept -> void { outcome = result<T>{e}; }

        // No exceptions cross this boundary, by construction. Under
        // -fno-exceptions the compiler never even calls this; we still declare
        // it (the coroutine machinery requires the name) and make it fatal.
        auto unhandled_exception() noexcept -> void { std::abort(); }
    };

    template <>
    struct task_promise<void> : public promise_base, fiber_promise
    {
        result<void> outcome;
        std::coroutine_handle<> continuation{};

        auto get_return_object() noexcept -> task<void>;
        auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept -> final_awaiter { return {}; }

        auto return_value(result<void> r) noexcept -> void { outcome = r; }
        auto return_value(error e) noexcept -> void { outcome = tl::make_unexpected(e); }
        auto unhandled_exception() noexcept -> void { std::abort(); }
    };

    template <class T>
    class task
    {
    public:
        using promise_type = task_promise<T>;
        using handle_type  = std::coroutine_handle<promise_type>;

        task() = default;
        explicit task(handle_type h) : m_coro(h) {}

        // Awaiting a task starts it (symmetric transfer into its body) and, when
        // it finishes, hands back its result<T>. This is where one coroutine
        // plugs into another.
        struct awaiter
        {
            handle_type handle;

            [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }

            [[nodiscard]] auto await_suspend(std::coroutine_handle<> caller) const noexcept -> std::coroutine_handle<>
            {
                handle.promise().continuation = caller;
                return handle;  // tail-call into the task body
            }

            [[nodiscard]] auto await_resume() const -> result<T>
            {
                return std::move(handle.promise().outcome);
            }
        };

        auto operator co_await() && noexcept -> awaiter { return awaiter{m_coro.get()}; }

        [[nodiscard]] auto handle() const noexcept -> handle_type { return m_coro.get(); }
        [[nodiscard]] auto release() noexcept -> handle_type { return m_coro.release(); }

    private:
        unique_coroutine<promise_type> m_coro;
    };

    template <class T>
    auto task_promise<T>::get_return_object() noexcept -> task<T>
    {
        return task<T>{std::coroutine_handle<task_promise<T>>::from_promise(*this)};
    }

    inline auto task_promise<void>::get_return_object() noexcept -> task<void>
    {
        return task<void>{std::coroutine_handle<task_promise<void>>::from_promise(*this)};
    }

    template <class U>
    struct ready_result_awaiter
    {
        result<U> value;
        [[nodiscard]] auto await_ready() const noexcept -> bool { return true; }
        auto await_suspend(std::coroutine_handle<>) const noexcept -> void {}
        [[nodiscard]] auto await_resume() -> result<U> { return std::move(value); }
    };

    template <class U>
    [[nodiscard]] auto operator co_await(result<U> r) noexcept -> ready_result_awaiter<U>
    {
        return ready_result_awaiter<U>{std::move(r)};
    }

    /**
     * @brief An asynchronous result of a fallible operation that can be co_await'd.
     *
     * This is a @r concurrencpp::result object that is used to make a function a coroutine.
     * Coroutines can use co_await to suspend execution and await a result.
     */
    template <class T = void>
    using async_result = task<T>;

#if SBK_CONFIG_DEBUG
    namespace detail
    {
        // Count of live @r detached_task frames. Teardown asserts this reaches zero once every executor
        // is abandoned, catching any frame that escaped reclamation (e.g. a hop into a dead executor).
        [[nodiscard]] inline auto live_detached_tasks() noexcept -> std::atomic<int>&
        {
            static std::atomic<int> counter{0};
            return counter;
        }
    }  // namespace detail
#endif

    /**
     * @brief A fire-and-forget coroutine. Starts running immediately and destroys its own frame when
     *        it finishes. Nobody awaits it, so it returns nothing -- any failure is logged at its
     *        origin (via @r sbk::log_error / @r sbk::make_error) rather than propagated.
     *
     * Use this to launch detached background work, e.g. `co_await workerExecutor->schedule()` at the
     * top to move onto a worker thread. Write it as a NAMED function whose inputs are passed BY VALUE:
     * coroutine parameters are copied into the frame and stay alive across every co_await, unlike the
     * captures of a temporary lambda (which dangle after the first suspension). Do not launch a
     * detached_task from a lambda that captures state it uses after a suspend.
     *
     * The frame is owned by the resuming thread while running and by the executor slot holding it
     * while suspended, so an executor torn down mid-flight reclaims its parked frames as it clears
     * its queue -- see @r work_item.
     *
     * @code
     *   auto encode_sound(engine::sound* sound, sc_encoder_config cfg, std::filesystem::path src,
     *                     std::filesystem::path dst, executor* worker) -> detached_task
     *   {
     *       co_await worker->schedule();
     *       // ... heavy work using the by-value parameters ...
     *   }
     * @endcode
     */
    struct detached_task
    {
        struct promise_type : public promise_base, fiber_promise
        {
            // No return object holds this frame, so an executor parking it must take ownership.
            static constexpr bool self_owns_frame = true;

#if SBK_CONFIG_DEBUG
            promise_type() noexcept { detail::live_detached_tasks().fetch_add(1, std::memory_order_relaxed); }
            ~promise_type() noexcept { detail::live_detached_tasks().fetch_sub(1, std::memory_order_relaxed); }
#endif

            auto get_return_object() noexcept -> detached_task { return {}; }
            auto initial_suspend() noexcept -> std::suspend_never { return {}; }  // eager: start now
            auto final_suspend() noexcept -> std::suspend_never { return {}; }    // self-destroy when done

            // Nobody awaits a detached task, so any error is at the top of its own stack. We still
            // want the SBK_CO_* macros to work here (they co_return a result/error), so the promise
            // accepts both. The value is dropped -- errors were already logged where they were born
            // (make_error / log_error / the SBK_ macros), per the log-once-at-origin discipline.
            // Natural completion: `co_return sbk::ok();`.
            auto return_value(result<void>) noexcept -> void {}
            auto return_value(error) noexcept -> void {}
            auto unhandled_exception() noexcept -> void { std::abort(); }
        };
    };
}  // namespace sbk

// ===========================================================================
// Coroutine control-flow macros -- the co_return form of SBK_TRY / SBK_TRYV.
//
// The macros in error/result.h use `return`, which is illegal inside a coroutine
// body. Inside a task<T> / async_result<T>, use these instead: they co_await the
// fallible call, bind its value (or forward the error via co_return). A plain
// sbk::result<U> also works as the operand -- operator co_await above makes it an
// immediately-ready awaitable, so the same macro covers async and sync callees.
// ===========================================================================

/**
 * @brief Co_awaits @p expr, binds its value via @p decl, else co_returns the error.
 *
 * @code
 *   SBK_CO_TRY(auto object, create_database_object(...));   // awaits/hops, unwraps, or forwards
 * @endcode
 */
#define SBK_CO_TRY(decl, expr)                                                               \
    auto&& SBK_DETAIL_UNIQUE(sbkCoResult_) = co_await (expr);                                \
    if (!SBK_DETAIL_UNIQUE(sbkCoResult_).has_value()) [[unlikely]]                           \
        co_return ::tl::make_unexpected(std::move(SBK_DETAIL_UNIQUE(sbkCoResult_)).error()); \
    decl = std::move(SBK_DETAIL_UNIQUE(sbkCoResult_)).value()

/**
 * @brief Co_awaits @p expr (value ignored, e.g. a task<void>); co_returns its error on failure.
 */
#define SBK_CO_TRYV(expr)                                                                        \
    do                                                                                           \
    {                                                                                            \
        auto&& SBK_DETAIL_UNIQUE(sbkCoResult_) = co_await (expr);                                \
        if (!SBK_DETAIL_UNIQUE(sbkCoResult_).has_value()) [[unlikely]]                           \
            co_return ::tl::make_unexpected(std::move(SBK_DETAIL_UNIQUE(sbkCoResult_)).error()); \
    } while (0)

/**
 * @brief Guards @p cond; if false, logs and returns @p code as an error (message defaults to the condition).
 */
#define SBK_CO_CHECK(cond, code)                                            \
    do                                                                      \
    {                                                                       \
        if (!(cond)) [[unlikely]]                                           \
            co_return ::sbk::make_error((code), "check failed: " #cond);    \
    } while (0)

/**
 * @brief Guards @p cond; if false, logs a fmt-formatted @p message and returns @p code.
 */
#define SBK_CO_CHECK_MSG(cond, code, ...)                                       \
    do                                                                          \
    {                                                                           \
        if (!(cond)) [[unlikely]]                                               \
        {                                                                       \
            co_return ::sbk::make_error((code), ::fmt::format(__VA_ARGS__));    \
        }                                                                       \
    } while (0)