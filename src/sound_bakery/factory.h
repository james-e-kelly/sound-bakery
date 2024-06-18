#pragma once

#include "sound_bakery/pch.h"

namespace sbk::engine
{
    class SB_CLASS Factory final
    {
    public:
        /**
         * @brief Create an object from its type
         * @param objectType rttr::type of the object to create
         * @param id optional id of the object
         * @return the created object represented as the base SB::Core::Object
         * type
         */
        static sbk::core::object* createObjectFromType(rttr::type objectType, sbk_id id = 0);

        static sbk::core::database_object* createDatabaseObjectFromType(rttr::type objectType, sbk_id id = 0);
    };
}  // namespace sbk::engine