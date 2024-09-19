#pragma once

#include "gluten/managers/manager.h"
#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/system.h"

#include "yaml-cpp/yaml.h"

namespace sbk::engine
{
    class sound_container;
}

struct selection
{
    selection() = default;

    void selected_object(sbk::core::object* object) { m_selected = object; }

    std::optional<rttr::type> selected_type() const
    {
        if (m_selected)
        {
            return m_selected->getType();
        }
        return std::nullopt;
    }

    sbk::core::object* get_selected() const { return m_selected; }

private:
    sbk::core::object* m_selected = nullptr;
};

class project_manager : public gluten::manager
{
public:
    project_manager(gluten::app* appOwner) : gluten::manager(appOwner) {}

public:
    void init_project(const std::filesystem::path& project_file);

    virtual void tick(double deltaTime) override;
    virtual void exit() override;

    void save_project() const;

    selection& get_selection() { return m_selection; }
    sbk::engine::sound_container* get_preview_sound_container() const;

private:
    selection m_selection;
};
