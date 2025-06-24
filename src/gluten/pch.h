#pragma once

#include "utils/boost_serialization_extensions.h"

#include <rttr/type>
#include "Delegates.h"
#include "imgui.h"
#include "tracy/Tracy.hpp"

#include <fmt/core.h>
#define BYTESIZE_FMTLIB_FORMATTER
#include <bytesize.hh>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace gluten
{
    enum class fonts
    {
        regular,
        regular_font_awesome,
        regular_audio_icons,
        regular_lucide_icons,
        light,
        title
    };
}