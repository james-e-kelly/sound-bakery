#pragma once

#include "gluten/managers/manager.h"
#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database_object.h"

#include "yaml-cpp/yaml.h"

namespace sbk::engine
{
    class SoundContainer;
}

struct Selection
{
    Selection() = default;

    void SelectObject(sbk::core::object* object) { m_selected = object; }

    std::optional<rttr::type> SelectedType() const
    {
        if (m_selected)
        {
            return m_selected->getType();
        }
        return std::nullopt;
    }

    sbk::core::object* GetSelected() const { return m_selected; }

private:
    sbk::core::object* m_selected = nullptr;
};

class ProjectManager : public gluten::manager
{
public:
    ProjectManager(gluten::app* appOwner) : gluten::manager(appOwner) {}

public:
    void init_project(const std::filesystem::path& projectFile);
    virtual void tick(double deltaTime) override;
    virtual void exit() override;

    void SaveProject() const;

    Selection& GetSelection() { return m_selection; }
    sbk::engine::SoundContainer* GetPreviewSoundContainer() const;

private:
    Selection m_selection;
};
