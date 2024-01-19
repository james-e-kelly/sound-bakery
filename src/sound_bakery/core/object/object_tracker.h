#pragma once

#include "sound_bakery/core/core_fwd.h"
#include "sound_bakery/util/singleton.h"

namespace SB::Core
{
    class ObjectTracker : public Singleton<ObjectTracker>
    {
        using RawObjectPtr = Object*;

    public:
        void trackObject(RawObjectPtr object);
        void untrackObject(
            RawObjectPtr object,
            const rttr::type& typeOverride = rttr::type::get<void>());

        std::unordered_set<RawObjectPtr> getObjectsOfCategory(
            SB_OBJECT_CATEGORY category);
        std::unordered_set<RawObjectPtr> getObjectsOfType(rttr::type type);

    private:
        std::unordered_map<SB_OBJECT_CATEGORY, std::unordered_set<RawObjectPtr>>
            m_categoryToObjects;
        std::unordered_map<rttr::type, std::unordered_set<RawObjectPtr>>
            m_typeToObjects;
    };
}  // namespace SB::Core