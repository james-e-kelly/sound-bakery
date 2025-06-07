#pragma once

#include "sound_bakery/core/core_fwd.h"

namespace sbk::core
{
    struct SB_CLASS object_ptr_comparator
    {
        bool operator()(const object* lhs, const object* rhs) const;
    };

    /**
     * @brief Tracks object categories and types.
     *
     * The object_tracker allows for searching of object types and categories.
     *
     * Objects are tracked with their memory address. This means objects in Sound Bakery cannot be moved around in
     * memory.
     *
     * @see database
     */
    class SB_CLASS object_tracker
    {
    public:
        auto track_object(object* object) -> concurrencpp::result<void>;
        auto untrack_object(object* object, std::optional<rttr::type> typeOverride = std::nullopt) -> concurrencpp::result<void>;

        [[nodiscard]] auto get_objects_of_category(const SB_OBJECT_CATEGORY& category) const -> concurrencpp::result<std::vector<object*>>;
        [[nodiscard]] auto get_objects_of_type(const rttr::type& type) const -> concurrencpp::result<std::vector<object*>>;

        [[nodiscard]] auto get_objects_of_type_size(const rttr::type& type) const -> concurrencpp::result<std::size_t>;
        [[nodiscard]] auto get_objects_size() const -> concurrencpp::result<size_t>;

        /**
         * @remark Makes a copy of the data. May be memory inefficient
         */
        [[nodiscard]] auto get_all_tracked_objects() const -> concurrencpp::result<std::vector<object*>>;

        [[nodiscard]] static auto convert_to_ordered(const std::unordered_set<object*>& unordered) -> std::set<object*, object_ptr_comparator>;

    private:
        void on_object_destroyed(object* object);

        mutable concurrencpp::async_lock m_categoryToObjectsMapLock;
        mutable concurrencpp::async_lock m_typeToObjectsMapLock;

        std::unordered_map<SB_OBJECT_CATEGORY, std::unordered_set<object*>> m_categoryToObjects;
        std::unordered_map<rttr::type, std::unordered_set<object*>> m_typeToObjects;
    };
}  // namespace sbk::core