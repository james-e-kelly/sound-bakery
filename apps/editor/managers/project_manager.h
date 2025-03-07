#pragma once

#include "gluten/managers/manager.h"
#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/system.h"

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
            return m_selected->get_object_type();
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
    auto create_project(const std::filesystem::directory_entry& projectDirectory,
                        const std::string& projectName) -> void;

    virtual void tick(double deltaTime) override;
    virtual void exit() override;

    void save_project() const;

    selection& get_selection() { return m_selection; }
    sbk::engine::sound_container* get_preview_sound_container() const;

private:
    void setup_project();

    selection m_selection;

    std::shared_ptr<gluten::widget> m_projectExplorerWidget;
    std::shared_ptr<gluten::widget> m_profilerWidget;
    std::shared_ptr<gluten::widget> m_playerWidget;
    std::shared_ptr<gluten::widget> m_detailsWidget;
    std::shared_ptr<gluten::widget> m_audioMeterWidget;
};
