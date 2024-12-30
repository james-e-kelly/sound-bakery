#pragma once

#include "Delegates.h"

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
        light,
        title
    };
}