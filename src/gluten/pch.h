#pragma once

#include "core/leak_detector.h"
#include "Delegates.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imspinner.h"
#include "imgui_internal.h"
#include "imspinner_demo.h"
#include "tracy/Tracy.hpp"
#include "utils/boost_serialization_extensions.h"

#include <fmt/core.h>

#include <rttr/type>
#define BYTESIZE_FMTLIB_FORMATTER
#include <bytesize.hh>
#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <cmath>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>