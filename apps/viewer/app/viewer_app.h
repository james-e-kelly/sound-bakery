#pragma once

#include "gluten/app/app.h"

class viewer_widget;

class viewer_app final : public gluten::app
{
public:
    static auto get() -> viewer_app*;

protected:
    auto cli_setup(boost::program_options::options_description& options) -> void override;
    auto cli_setup_positional(boost::program_options::positional_options_description& positionalOptions) -> void override;
    auto pre_init(const boost::program_options::variables_map& cliVariables) -> void override;
    auto post_init() -> void override;
    auto exit() -> void override;

private:
    std::vector<std::filesystem::path> m_launchFiles;
    std::shared_ptr<viewer_widget> m_viewerWidget;
};
