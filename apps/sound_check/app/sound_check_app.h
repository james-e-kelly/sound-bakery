#pragma once

#include "gluten/app/app.h"

class workspace_manager;

class sound_check_app final : public gluten::app
{
protected:
    virtual auto pre_init() -> void;
    virtual auto post_init() -> void;

private:
    std::shared_ptr<workspace_manager> m_workspaceManager;
};