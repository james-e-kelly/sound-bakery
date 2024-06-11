#pragma once

#include "sound_bakery/pch.h"

namespace SB::Engine
{
    class NodeBase;
    class Node;
}  // namespace SB::Engine

namespace SB::Util
{
    class SB_CLASS TypeHelper final
    {
    public:
        static SB_OBJECT_CATEGORY getCategoryFromType(rttr::type type);

        static std::unordered_set<rttr::type> getTypesFromCategory(SB_OBJECT_CATEGORY category);

        static std::string_view getDisplayNameFromType(rttr::type type);

        static std::string getFolderNameForObjectType(rttr::type type);

        static std::string_view getFileExtensionOfObjectCategory(SB_OBJECT_CATEGORY category);

        static std::string_view getPayloadFromType(rttr::type type);

        static bool isTypePlayable(const rttr::type& type);

        static rttr::enumeration getObjectCategoryEnum();

        static rttr::string_view getObjectCategoryName(const SB_OBJECT_CATEGORY& objectCategory);

        static SB::Core::object* getObjectFromInstance(const rttr::instance& instance);

        static SB::Core::database_object* getDatabaseObjectFromInstance(const rttr::instance& instance);

        static SB::Engine::Node* getNodeFromInstance(const rttr::instance& instance);

        static SB::Engine::NodeBase* getNodeBaseFromInstance(const rttr::instance& instance);
    };
}  // namespace SB::Util