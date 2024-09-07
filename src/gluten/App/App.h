#pragma once

#include "gluten/managers/manager.h"
#include "gluten/subsystems/subsystem.h"

#include "imgui.h"

namespace gluten
{
    class app
    {
    public:
        static app* get();

        int run(int argc, char** argv);
        void request_exit();

        virtual void post_init() = 0;

        template <class T>
        T* add_subsystem_class();

        template <class T>
        T* get_subsystem_by_class();

        template <class T>
        T* add_manager_class();

        template <class T>
        T* get_manager_by_class();

        bool is_maximized();

        void set_application_display_title(const std::string& title);
        std::string_view get_application_display_title() const { return m_applicationDisplayTitle; }

        ImFont* get_font(const fonts& font) { return m_fonts[font]; }

    private:
        void load_fonts();

        std::vector<std::unique_ptr<subsystem>> m_subsystems;
        std::vector<std::unique_ptr<manager>> m_managers;

        std::chrono::high_resolution_clock::time_point m_currentTime;
        std::chrono::high_resolution_clock::time_point m_previousTime;

        std::string m_executableLocation;
        std::string m_applicationDisplayTitle;

        std::unordered_map<fonts, ImFont*> m_fonts;

        bool m_hasInit          = false;
        bool m_isRequestingExit = false;
    };

    template <class T>
    T* app::add_subsystem_class()
    {
        m_subsystems.push_back(std::make_unique<T>(this));
        T* subsystem = dynamic_cast<T*>(m_subsystems.back().get());
        assert(subsystem);

        if (m_hasInit)
        {
            subsystem->pre_init(0, NULL);
            subsystem->init();
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

    template <class T>
    T* app::add_manager_class()
    {
        m_managers.push_back(std::make_unique<T>(this));
        T* manager = dynamic_cast<T*>(m_managers.back().get());

        if (manager)
        {
            manager->init(this);
        }

        return manager;
    }

    template <class T>
    T* app::get_manager_by_class()
    {
        for (std::unique_ptr<manager>& manager : m_managers)
        {
            if (T* castedManager = dynamic_cast<T*>(manager.get()))
            {
                return castedManager;
            }
        }
        return nullptr;
    }
}  // namespace gluten