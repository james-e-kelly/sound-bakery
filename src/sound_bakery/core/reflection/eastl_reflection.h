#pragma once

#include <EASTL/vector.h>

#include <rttr/detail/impl/sequential_mapper_impl.h>

#include <cstddef>

// rttr ships sequential_container_mapper specializations for std::vector / std::list /
// std::deque / std::array only -- eastl::vector is a different type so
// rttr::type::is_sequential_container() returns false for it. Without this specialization
// the serializer picks the wrong branch (writes/reads Child instead of SeqContainer) and
// bakery files fail to load with input-stream errors.
//
// Not simply inheriting from detail::sequential_container_base_dynamic_direct_access:
// eastl::vector's iterator is `T*` and const_iterator is `const T*`, so the two overloads
// of `get_data(const itr_t&)` / `get_data(const const_itr_t&)` in that base are both
// viable when rttr's SFINAE probes with an rvalue `T*` (T* binds to both const T*& and
// const const T*&), yielding an ambiguous-call error. We provide a single templated
// get_data instead, which shadows the base overloads and resolves unambiguously.
//
// This must be visible in every TU that registers reflection for a class containing an
// eastl::vector property, so include from pch.h.

namespace rttr
{
    template <typename T, typename Allocator>
    struct sequential_container_mapper<eastl::vector<T, Allocator>>
    {
        using container_t = eastl::vector<T, Allocator>;
        using value_t     = typename container_t::value_type;
        using itr_t       = typename container_t::iterator;
        using const_itr_t = typename container_t::const_iterator;

        static bool is_dynamic() { return true; }

        template <typename It>
        static auto get_data(const It& itr) -> decltype(*itr)& { return *itr; }

        static itr_t       begin(container_t& c)       { return c.begin(); }
        static const_itr_t begin(const container_t& c) { return c.begin(); }
        static itr_t       end(container_t& c)         { return c.end(); }
        static const_itr_t end(const container_t& c)   { return c.end(); }

        static void        clear(container_t& c)         { c.clear(); }
        static bool        is_empty(const container_t& c){ return c.empty(); }
        static std::size_t get_size(const container_t& c){ return c.size(); }
        static bool        set_size(container_t& c, std::size_t s) { c.resize(static_cast<typename container_t::size_type>(s)); return true; }

        static itr_t erase(container_t& c, const itr_t& it)       { return c.erase(it); }
        static itr_t erase(container_t& c, const const_itr_t& it) { return c.erase(it); }

        static itr_t insert(container_t& c, const value_t& v, const itr_t& pos)       { return c.insert(pos, v); }
        static itr_t insert(container_t& c, const value_t& v, const const_itr_t& pos) { return c.insert(pos, v); }

        static value_t&       get_value(container_t& c, std::size_t i)       { return c[static_cast<typename container_t::size_type>(i)]; }
        static const value_t& get_value(const container_t& c, std::size_t i) { return c[static_cast<typename container_t::size_type>(i)]; }
    };
}  // namespace rttr
