#pragma once

#include "Managers/AppManager.h"
#include "Managers/ProjectManager.h"
#include "Subsystems/Subsystem.h"
#include "Widgets/Widget.h"
#include "pch.h"

class app final
{
public:
    int run(int argc, char** argv);
    void request_exit();

public:
    template <class T>
    T* add_subsystem_class();

    template <class T>
    T* get_subsystem_by_class();

    AppManager* GetAppManager() const { return m_appManager.get(); }
    ProjectManager* GetProjectManager() const { return m_projectManager.get(); }

    void OpenProject(const std::filesystem::path& projectFile);

private:
    std::vector<std::unique_ptr<subsystem>> m_subsystems;
    std::chrono::high_resolution_clock::time_point m_previousTime;
    std::string m_executableLocation;
    bool m_hasInit          = false;
    bool m_isRequestingExit = false;

    std::unique_ptr<AppManager> m_appManager;
    std::unique_ptr<ProjectManager> m_projectManager;
};

template <class T>
T* app::add_subsystem_class()
{
    m_subsystems.push_back(std::make_unique<T>(this));
    T* subsystem = dynamic_cast<T*>(m_subsystems.back().get());
    assert(subsystem);

    if (m_hasInit)
    {
        subsystem->PreInit(0, NULL);
        subsystem->Init();
    }

    return subsystem;
}

template <class T>
T* app::get_subsystem_by_class()
{
    for (std::unique_ptr<subsystem>& subsystem : m_subsystems)
    {
        if (T* castedSubsystem = dynamic_cast<T*>(subsystem.get()))
        {
            return castedSubsystem;
        }
    }
    assert(false);
    return nullptr;
}