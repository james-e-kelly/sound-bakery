#pragma once

#include "sound_bakery/pch.h"

namespace sbk
{
    /**
     * @brief Move-only RAII ownership of a coroutine frame -- the @c unique_ptr of coroutine handles.
     *
     * A raw @c std::coroutine_handle is a non-owning pointer to the heap-allocated frame; nothing
     * reclaims it unless someone calls @c destroy. Holding the handle in this wrapper makes the
     * ownership single: exactly one owner, destroyed on scope exit unless handed off via @r release.
     *
     * Templated on the promise so the same primitive serves a typed owner (@r task, which needs
     * @c promise()) and a type-erased owner (@r work_item). @c Promise defaults to @c void.
     */
    template <class Promise = void>
    class unique_coroutine
    {
    public:
        using handle_type = std::coroutine_handle<Promise>;

        unique_coroutine() noexcept = default;
        explicit unique_coroutine(handle_type handle) noexcept : m_handle(handle) {}

        unique_coroutine(unique_coroutine&& other) noexcept : m_handle(std::exchange(other.m_handle, {})) {}
        auto operator=(unique_coroutine&& other) noexcept -> unique_coroutine&
        {
            if (this != &other)
            {
                if (m_handle)
                {
                    m_handle.destroy();
                }
                m_handle = std::exchange(other.m_handle, {});
            }
            return *this;
        }

        unique_coroutine(const unique_coroutine&)                    = delete;
        auto operator=(const unique_coroutine&) -> unique_coroutine& = delete;

        ~unique_coroutine()
        {
            if (m_handle)
            {
                m_handle.destroy();
            }
        }

        [[nodiscard]] auto get() const noexcept -> handle_type { return m_handle; }
        [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(m_handle); }

        // Relinquish ownership without destroying the frame. The caller must eventually resume or destroy it.
        [[nodiscard]] auto release() noexcept -> handle_type { return std::exchange(m_handle, {}); }

    private:
        handle_type m_handle{};
    };
}  // namespace sbk
