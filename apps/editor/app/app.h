#pragma once

#include "pch.h"

class editor_app final : public gluten::app
{
public:
    void open_project(const std::filesystem::path& project_file);
    void create_and_open_project(const std::filesystem::directory_entry& projectFolder, const std::string_view& projectName);

    void post_init() override;
};
