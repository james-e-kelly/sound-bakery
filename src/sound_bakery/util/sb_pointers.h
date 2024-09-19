#pragma once

#include "sound_bakery/core/lazy_pointer.h"

namespace sbk
{
    namespace core
    {
        class object;
        class database_object;
    }  // namespace core

    using LObjectPtr = SB::Core::DatabasePtr<SB::Core::DatabaseObject>;
}  // namespace sbk