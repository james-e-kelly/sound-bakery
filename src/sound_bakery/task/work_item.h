#pragma once

#include "sound_bakery/pch.h"

#include "sound_bakery/task/unique_coroutine.h"

namespace sbk
{
    /**
     * @brief True when @p Promise declares `static constexpr bool self_owns_frame = true`, meaning a
     *        queue parking its coroutine must own the frame outright (nothing else holds it). Promises
     *        without the marker are owned by an awaiter, so the queue only borrows the handle.
     */
    template <class Promise, class = void>
    inline constexpr bool promise_self_owns_frame_v = false;

    template <class Promise>
    inline constexpr bool promise_self_owns_frame_v<Promise, std::void_t<decltype(Promise::self_owns_frame)>> =
        Promise::self_owns_frame;

    /**
     * @brief A move-only unit of work queued on an @r executor.
     *
     * Type-erased over any move-only invocable, so one queue element type carries a plain callable, a
     * batch of other work_items (see @r command_queue), or a coroutine resume. An *owning* resume item
     * holds its suspended frame, so a queue dropped at shutdown reclaims every parked frame it held.
     */
    class work_item
    {
    public:
        work_item() = default;
        ~work_item() = default;

        template <class Fn> requires std::is_invocable_r_v<void, std::decay_t<Fn>&> && (!std::is_same_v<std::decay_t<Fn>, work_item>)
        work_item(Fn&& fn) : m_impl(std::make_unique<invocable_impl<std::decay_t<Fn>>>(std::forward<Fn>(fn)))
        {
        }

        work_item(work_item&&) noexcept                    = default;
        auto operator=(work_item&&) noexcept -> work_item& = default;
        work_item(const work_item&)                        = delete;
        auto operator=(const work_item&) -> work_item&     = delete;

        [[nodiscard]] explicit operator bool() const noexcept { return m_impl != nullptr; }

        auto operator()() -> void
        {
            BOOST_ASSERT_MSG(m_impl != nullptr, "invoking a moved-from or default-constructed work_item");
            m_impl->invoke();
        }

        /**
         * @brief Resume item that owns the suspended coroutine frame. For self-owning coroutines
         *        (@r detached_task): if the item is dropped before running, the frame is destroyed.
         */
        [[nodiscard]] static auto resume_owning(std::coroutine_handle<> handle, const char* fiberName) -> work_item
        {
            // Ownership handoff: while the lambda lives, the captured unique_coroutine owns the frame,
            // so dropping the item unrun destroys it. Running the lambda transfers the raw handle to
            // resume(), which either completes the frame (self-destroys) or hands it to whatever
            // executor the resume suspends onto next.
            return work_item{[coro = unique_coroutine<>{handle}, fiberName]() mutable
                             { resume(coro.release(), fiberName); }};
        }

        /**
         * @brief Resume item that only borrows the handle; the frame is owned elsewhere (an awaiting
         *        @r task). Running resumes; dropping simply forgets the borrow.
         */
        [[nodiscard]] static auto resume_borrowed(std::coroutine_handle<> handle, const char* fiberName) -> work_item
        {
            return work_item{[handle, fiberName] { resume(handle, fiberName); }};
        }

    private:
        // Sean Parent-style type erasure: invocable_base is the polymorphic interface, invocable_impl<Fn>
        // is the concrete holder. Lets any move-only callable live in the same queue element.
        struct invocable_base
        {
            virtual ~invocable_base()     = default;
            virtual auto invoke() -> void = 0;
        };

        template <class Fn>
        struct invocable_impl final : invocable_base
        {
            template <class G>
            explicit invocable_impl(G&& fn) : m_fn(std::forward<G>(fn))
            {
            }
            auto invoke() -> void override { m_fn(); }
            Fn m_fn;
        };

        static auto resume(std::coroutine_handle<> handle, [[maybe_unused]] const char* fiberName) -> void
        {
            if (!handle)
            {
                return;
            }
            ZoneScopedN("coroutine resume");
#ifdef TRACY_ENABLE
            if (fiberName)
            {
                TracyFiberEnter(fiberName);
                handle.resume();
                TracyFiberLeave;
                return;
            }
#endif
            handle.resume();
        }

        std::unique_ptr<invocable_base> m_impl;
    };
}  // namespace sbk
