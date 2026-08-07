#pragma once

#include "pch.h"

class editor_app final : public gluten::app
{
public:
    void open_project(const std::filesystem::path& project_file);
    void create_and_open_project(const std::filesystem::directory_entry& projectFolder, const std::string_view& projectName);

    auto cli_setup(boost::program_options::options_description& options) -> void override;
    auto pre_init(const boost::program_options::variables_map& cliVariables) -> void override;
    void post_init() override;

    auto on_file_drop(const std::vector<std::string>& paths) -> void override;

private:
    std::optional<std::string> m_projectFile;
};
