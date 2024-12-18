#pragma once

#include "sound_bakery/pch.h"

namespace sbk::engine
{
    class node_base;
    class node;
}  // namespace sbk::engine

namespace sbk::util
{
    class SB_CLASS type_helper final
    {
    public:
        static SB_OBJECT_CATEGORY getCategoryFromType(rttr::type type);

        static std::unordered_set<rttr::type> getTypesFromCategory(SB_OBJECT_CATEGORY category);

        static rttr::string_view get_display_name_from_type(rttr::type type);

        static std::string getFolderNameForObjectType(rttr::type type);

        static std::string_view getFileExtensionOfObjectCategory(SB_OBJECT_CATEGORY category);

        static std::string_view getPayloadFromType(rttr::type type);

        static bool isTypePlayable(const rttr::type& type);

        static rttr::enumeration getObjectCategoryEnum();

        static rttr::string_view getObjectCategoryName(const SB_OBJECT_CATEGORY& objectCategory);

        static sbk::core::object* getObjectFromInstance(const rttr::instance& instance);

        static sbk::core::database_object* getDatabaseObjectFromInstance(const rttr::instance& instance);

        static sbk::engine::node* getNodeFromInstance(const rttr::instance& instance);

        static sbk::engine::node_base* getNodeBaseFromInstance(const rttr::instance& instance);
    };
}  // namespace sbk::util