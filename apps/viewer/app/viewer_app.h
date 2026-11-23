#pragma once

#include "gluten/app/app.h"

namespace gluten
{
    class audio_subsystem;
}

class viewer_app final : public gluten::app
{
public:
    static auto get() -> viewer_app*;

protected:
    auto cli_setup(boost::program_options::options_description& options) -> void override;
    auto pre_init(const boost::program_options::variables_map& cliVariables) -> void override;
    auto post_init() -> void override;
    auto exit() -> void override;
    auto tick_implementation() -> void override;

private:
    std::shared_ptr<gluten::audio_subsystem> m_audioSubsystem;
};