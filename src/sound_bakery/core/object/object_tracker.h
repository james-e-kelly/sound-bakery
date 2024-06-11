#pragma once

#include "sound_bakery/core/core_fwd.h"

namespace SB::Core
{
    class SB_CLASS ObjectTracker
    {
        using RawObjectPtr = object*;

    public:
        void trackObject(RawObjectPtr object);
        void untrackObject(RawObjectPtr object, std::optional<rttr::type> typeOverride = std::nullopt);

        std::unordered_set<RawObjectPtr> getObjectsOfCategory(SB_OBJECT_CATEGORY category);
        std::unordered_set<RawObjectPtr> getObjectsOfType(rttr::type type);

    private:
        std::unordered_map<SB_OBJECT_CATEGORY, std::unordered_set<RawObjectPtr>> m_categoryToObjects;
        std::unordered_map<rttr::type, std::unordered_set<RawObjectPtr>> m_typeToObjects;
    };
}  // namespace SB::Core