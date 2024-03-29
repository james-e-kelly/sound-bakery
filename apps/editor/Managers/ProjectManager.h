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
    void Init(const ProjectConfiguration&& projectConfig);
    virtual void Tick(double deltaTime) override;
    virtual void Exit() override;

public:
    void SaveProject() const;
    void LoadProject();

public:
    Selection& GetSelection() { return m_selection; }
    SB::Engine::SoundContainer* GetPreviewSoundContainer() const
    {
        return m_previewSoundContainer;
    }

private:
    void SaveYAMLToFile(YAML::Emitter& yaml,
                        std::ofstream& stream,
                        std::filesystem::path file) const;
    void SaveObjectToFile(YAML::Emitter& yaml,
                          std::ofstream& stream,
                          SB_ID objectID,
                          std::filesystem::path file,
                          std::string_view fileExtension) const;

private:
    ProjectConfiguration m_projectConfiguration;

    Selection m_selection;

    SB::Engine::SoundContainer* m_previewSoundContainer = nullptr;
    std::shared_ptr<SB::Core::DatabaseObject> m_previewSoundContainerResource;
};
