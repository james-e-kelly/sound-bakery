#pragma once

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "Delegates.h"

#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <format>
#include <fstream>
#include <variant>

#include <stdio.h>
#include <stdlib.h>

using int8 = signed char;
using uint8 = unsigned char;
using int16 = signed short;
using uint16 = unsigned short;
using int32 = signed int;
using uint32 = unsigned int;
using int64 = long long;
using uint64 = unsigned long long;

struct ProjectConfiguration
{
    std::string m_projectName;
    std::filesystem::path m_projectFile;
    std::filesystem::path m_projectFolder;
    std::filesystem::path m_sourceFolder;
    std::filesystem::path m_sourceSoundFolder;
    std::filesystem::path m_sourceDialogueFolder;
    std::filesystem::path m_sourceMusicFolder;
    std::filesystem::path m_objectsFolder;
    std::filesystem::path m_eventsFolder;
    std::filesystem::path m_soundbanksFolder;
};
