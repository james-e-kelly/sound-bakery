#pragma once

#include "sound_bakery/core/lazy_pointer.h"

namespace SB
{
    namespace Core
    {
        class Object;
        class DatabaseObject;
    }

    using LObjectPtr = SB::Core::DatabasePtr<SB::Core::DatabaseObject>;
}