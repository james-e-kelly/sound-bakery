#pragma once

#include "Manager.h"
#include "sound_bakery/system.h"
#include "sound_bakery/core/database/database_object.h"

#include "yaml-cpp/yaml.h"

namespace SB::Engine
{
    class SoundContainer;
}

struct Selection
{
    Selection() = default;

    void SelectObject(SB::Core::Object* object) { m_selected = object; }

    std::optional<rttr::type> SelectedType() const
    {
        if (m_selected)
        {
            return m_selected->getType();
        }
        return std::nullopt;
    }

    SB::Core::Object* GetSelected() const { return m_selected; }

private:
    SB::Core::Object* m_selected = nullptr;
};

class ProjectManager : public Manager
{
public:
    ProjectManager(App* appOwner) : Manager(appOwner) {}

public:
    void Init(const std::filesystem::path& projectFile);
    virtual void Tick(double deltaTime) override;
    virtual void Exit() override;

    void SaveProject() const;

    void ConvertAllFilesTest() const;

    Selection& GetSelection() { return m_selection; }
    SB::Engine::SoundContainer* GetPreviewSoundContainer() const
    {
        return m_previewSoundContainer;
    }

private:
    Selection m_selection;

    SB::Engine::SoundContainer* m_previewSoundContainer = nullptr;
};
