#pragma once

#include "sound_bakery/core/lazy_pointer.h"

namespace SB
{
    namespace Core
    {
        class object;
        class database_object;
    }  // namespace Core

    using LObjectPtr = SB::Core::DatabasePtr<SB::Core::DatabaseObject>;
}  // namespace SB