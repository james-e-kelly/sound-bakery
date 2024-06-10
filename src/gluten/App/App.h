#pragma once

#include "Managers/Manager.h"
#include "Subsystems/Subsystem.h"

class App final
{
public:
    int Run(int argc, char** argv);
    void RequestExit();

public:
    template <class T>
    T* AddSubsystemClass();

    template <class T>
    T* GetSubsystemByClass();

    template <class T>
    T* AddManagerClass();

    template <class T>
    T* GetManagerByClass();

private:
    std::vector<std::unique_ptr<Subsystem>> m_subsystems;
    std::vector<std::unique_ptr<Manager>> m_managers;

    std::chrono::high_resolution_clock::time_point m_currentTime;
    std::chrono::high_resolution_clock::time_point m_previousTime;
    
    std::string m_executableLocation;

    bool m_hasInit          = false;
    bool m_isRequestingExit = false;
};

template <class T>
T* App::AddSubsystemClass()
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
T* App::GetSubsystemByClass()
{
    for (std::unique_ptr<Subsystem>& subsystem : m_subsystems)
    {
        if (T* castedSubsystem = dynamic_cast<T*>(subsystem.get()))
        {
            return castedSubsystem;
        }
    }
    assert(false);
    return nullptr;
}

template <class T>
T* App::AddManagerClass()
{
    m_managers.push_back(std::make_unique<T>(this));
    T* manager = dynamic_cast<T*>(m_managers.back().get());

    if (manager)
    {
        manager->Init(this);
    }

    return manager;
}

template <class T>
T* App::GetManagerByClass()
{
    for (std::unique_ptr<Manager>& manager : m_managers)
    {
        if (T* castedManager = dynamic_cast<T*>(manager.get()))
        {
            return castedManager;
        }
    }
    return nullptr;
}