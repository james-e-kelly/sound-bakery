#pragma once

#include "sound_bakery/core/core_fwd.h"

namespace sbk::core
{
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
        void trackObject(object* object);
        void untrackObject(object* object, std::optional<rttr::type> typeOverride = std::nullopt);

        [[nodiscard]] std::unordered_set<object*> getObjectsOfCategory(const SB_OBJECT_CATEGORY& category);
        [[nodiscard]] std::unordered_set<object*> getObjectsOfType(const rttr::type& type);

    private:
        std::unordered_map<SB_OBJECT_CATEGORY, std::unordered_set<object*>> m_categoryToObjects;
        std::unordered_map<rttr::type, std::unordered_set<object*>> m_typeToObjects;
    };
}  // namespace sbk::core