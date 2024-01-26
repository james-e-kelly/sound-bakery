#pragma once

#include "sound_bakery/pch.h"

namespace SB::Engine
{
    class Factory final
    {
    public:
        /**
         * @brief Create an object from its type
         * @param objectType rttr::type of the object to create
         * @param id optional id of the object
         * @return the created object represented as the base SB::Core::Object
         * type
         */
        static SB::Core::Object* createObjectFromType(rttr::type objectType, SB_ID id = 0);

        static SB::Core::DatabaseObject* createDatabaseObjectFromType(rttr::type objectType, SB_ID id = 0);
    };
}  // namespace SB::Engine