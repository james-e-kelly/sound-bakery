#pragma once

#include "sound_bakery/core/core_include.h"

namespace SB::Editor
{
    /**
     * Manages a project file and the objects contained within it.
    */
    class SB_CLASS Project : public SB::Core::Object
    {
        REGISTER_REFLECTION(Project, SB::Core::Object)
    };
}  // namespace SB::Engine