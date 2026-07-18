#pragma once

#include "sound_bakery/pch.h"
#include "sound_bakery/core/core_fwd.h"

namespace sbk::core
{
    struct SB_CLASS object_ptr_comparator
    {
        auto operator()(const object* lhs, const object* rhs) const -> bool;
    };

    /**
     * @brief Tracks object categories and types.
     *
     * The object_tracker allows for searching of object types and categories.
     *
     * Objects are tracked with their memory address. This means objects in sound Bakery cannot be moved around in
     * memory.
     *
     * @see database
     */
    class SB_CLASS object_tracker
    {
    public:
        auto track_object(object* object) -> void;
        auto untrack_object(object* object, std::optional<rttr::type> typeOverride = std::nullopt) -> void;

        [[nodiscard]] auto get_objects_of_category(const SB_OBJECT_CATEGORY& category) const -> std::unordered_set<object*>;
        [[nodiscard]] auto get_objects_of_type(const rttr::type& type) const -> std::unordered_set<object*>;
        [[nodiscard]] auto get_objects_count() const -> size_t;
        [[nodiscard]] auto get_all_category_to_objects() const -> const std::unordered_map<SB_OBJECT_CATEGORY, std::unordered_set<object*>>&;
        [[nodiscard]] auto get_all_type_to_objects() const -> const std::unordered_map<rttr::type, std::unordered_set<object*>>&;

        [[nodiscard]] auto convert_to_ordered(const std::unordered_set<object*>& unordered) const
            -> std::set<object*, object_ptr_comparator>;

    private:
        auto on_object_destroyed(object* object) -> void;

        std::unordered_map<SB_OBJECT_CATEGORY, std::unordered_set<object*>> m_categoryToObjects;
        std::unordered_map<rttr::type, std::unordered_set<object*>> m_typeToObjects;
    };
}  // namespace sbk::core