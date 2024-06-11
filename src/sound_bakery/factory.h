#pragma once

#include "sound_bakery/pch.h"

namespace SB::Engine
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
        static SB::Core::object* createObjectFromType(rttr::type objectType, sb_id id = 0);

        static SB::Core::database_object* createDatabaseObjectFromType(rttr::type objectType, sb_id id = 0);
    };
}  // namespace SB::Engine