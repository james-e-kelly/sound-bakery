#pragma once

#include "pch.h"

class editor_app final : public gluten::app
{
    void OpenProject(const std::filesystem::path& projectFile);
};
