#pragma once

#include "gluten/managers/manager.h"
#include "gluten/subsystems/subsystem.h"
#include "imgui.h"

namespace gluten
{
    /**
     * @brief Manages application lifetime and object owning.
     * 
     * The app class is intended to pass application behaviour to manager and subsystem classes.
     */
    class app
    {
    public:
        app()          = default;
        virtual ~app() = default;

        static app* get();

        int run(int argc, char** argv);
        void request_exit();

        virtual void post_init() = 0;

        template <class T>
        std::shared_ptr<T> add_subsystem_class();

        template <class T>
        std::shared_ptr<T> get_subsystem_by_class();

        template <class T>
        void remove_subsystem_by_class();

        template <class T>
        std::shared_ptr<T> add_manager_class();

        template <class T>
        std::shared_ptr<T> get_manager_by_class();

        template <class T>
        void remove_manager_by_class();

        bool is_maximized();

        void set_application_display_title(const std::string& title);
        std::string_view get_application_display_title() const { return m_applicationDisplayTitle; }

        ImFont* get_font(const fonts& font) { return m_fonts[font]; }

    private:
        void load_fonts();

        std::vector<std::shared_ptr<subsystem>> m_subsystems;
        std::vector<std::shared_ptr<manager>> m_managers;

        std::chrono::high_resolution_clock::time_point m_currentTime;
        std::chrono::high_resolution_clock::time_point m_previousTime;

        std::string m_executableLocation;
        std::string m_applicationDisplayTitle;

        std::unordered_map<fonts, ImFont*> m_fonts;

        bool m_hasInit          = false;
        bool m_isRequestingExit = false;
    };

    template <class T>
    std::shared_ptr<T> app::add_subsystem_class()
    {
        m_subsystems.push_back(std::make_shared<T>(this));
        std::shared_ptr<subsystem> subsystemPtr = m_subsystems.back();
        assert(subsystemPtr);

        if (m_hasInit)
        {
            subsystemPtr->pre_init(0, NULL);
            subsystemPtr->init();
        }

        return std::static_pointer_cast<T>(subsystemPtr);
    }

    template <class T>
    std::shared_ptr<T> app::get_subsystem_by_class()
    {
        for (std::shared_ptr<subsystem>& subsystemPtr : m_subsystems)
        {
            if (std::shared_ptr<T> castedSubsystem = std::static_pointer_cast<T>(subsystemPtr))
            {
                if (dynamic_cast<T*>(subsystemPtr.get()) != nullptr)
                {
                    return castedSubsystem;
                }
            }
        }
        assert(false);
        return nullptr;
    }

    template <class T>
    void app::remove_subsystem_by_class()
    {
        for (int index = m_subsystems.size() - 1; index >= 0; --index)
        {
            if (T* castedSubsytem = dynamic_cast<T*>(m_subsystems[index].get()))
            {
                castedSubsytem->exit();
                m_subsystems.erase(m_subsystems.begin() + index);
                return;
            }
        }
    }

    template <class T>
    std::shared_ptr<T> app::add_manager_class()
    {
        m_managers.push_back(std::make_shared<T>(this));
        std::shared_ptr<manager> managerPtr = m_managers.back();

        if (managerPtr)
        {
            managerPtr->init(this);
        }

        return std::static_pointer_cast<T>(managerPtr);
    }

    template <class T>
    std::shared_ptr<T> app::get_manager_by_class()
    {
        for (std::shared_ptr<manager>& managerPtr : m_managers)
        {
            if (std::shared_ptr<T> castedManager = std::static_pointer_cast<T>(managerPtr))
            {
                if (dynamic_cast<T*>(managerPtr.get()) != nullptr)
                {
                    return castedManager;
                }
            }
        }
        return {};
    }

    template <class T>
    void app::remove_manager_by_class()
    {
        for (int index = m_managers.size() - 1; index >= 0; --index)
        {
            if (T* castedManager = dynamic_cast<T*>(m_managers[index].get()))
            {
                castedManager->exit();
                m_managers.erase(m_managers.begin() + index);
                return;
            }
        }
    }
}  // namespace gluten