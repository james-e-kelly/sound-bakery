#pragma once

#include "core/leak_detector.h"
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include "gluten/managers/manager.h"
#include "gluten/subsystems/subsystem.h"
#include "concurrencpp/concurrencpp.h"
#include "imgui.h"

namespace gluten
{
    static inline constexpr const char* g_serializedEntryName = "data";
    static inline constexpr float g_baseFontSize              = 16.0f;
    static inline constexpr float g_baseIconFontSize         = g_baseFontSize * (2.0f / 3.0f);

    /**
     * @brief Manages application lifetime and object owning.
     * 
     * The app class is intended to pass application behaviour to manager and subsystem classes.
     */
    class app : public concurrencpp::runtime
    {
        LEAK_DETECTOR(app)

    public:
        app()          = default;
        virtual ~app() = default;

        static app* get();

        int run(int argc, char** argv);
        void request_exit();

        template <class T>
        std::shared_ptr<T> add_subsystem_class();

        template <class T>
        std::shared_ptr<T> get_subsystem_by_class();

        template <class T>
        void remove_subsystem_by_class();

        template <class T, typename... Args>
        std::shared_ptr<T> add_manager_class(Args&&... args);

        template <class T>
        std::shared_ptr<T> get_manager_by_class();

        template <class T>
        void remove_manager_by_class();

        bool is_maximized();

        void set_application_display_title(const std::string& title);
        std::string_view get_application_display_title() const { return m_applicationDisplayTitle; }

        ImFont* get_font(const fonts& font) { return m_fonts[font]; }

        virtual auto on_file_drop(const std::vector<std::string>& paths) -> void {}

        static auto open_select_folder_dialog() -> std::filesystem::path;
        static auto open_select_file_dialog(const std::string& name, const std::string& fileExtensionNoDots) -> std::filesystem::path;

        template<typename T>
        static auto save_data_to_disk(const std::filesystem::path& file, const T& data) -> void
        {
            std::filesystem::create_directories(file.parent_path());

            std::ofstream outputStream(file, std::ios_base::out);
            boost::archive::xml_oarchive archive(outputStream);

            archive & boost::serialization::make_nvp(g_serializedEntryName, data);
        }

        template <typename T>
        static auto load_data_from_disk(const std::filesystem::path& file, T& data) -> void
        {
            std::ifstream inputStream(file, std::ios_base::in);
            boost::archive::xml_iarchive archive(inputStream);

            archive & boost::serialization::make_nvp(g_serializedEntryName, data);
        }

        auto get_tick_executor() const -> std::shared_ptr<concurrencpp::manual_executor>
        {
            return m_tickExecutor;
        }

    protected:
        /**
         * @brief Runs after subsystems are created and before any init functions are called.
         * 
         * Use this function to create the root widget, managers, more subsystems or general initialization.
         */
        virtual void pre_init() {}

        /**
         * @brief Runs after all init functions were called and before start.
         */
        virtual void post_init() {}

        /**
         * @brief Runs before everything has exited and when the app is about to exit.
         * 
         * The subsystems and managers will still be valid at this point.
         */
        virtual void exit() {}

        virtual void tick_implementation() {}

    private:
        auto start() -> void;
        auto tick() -> void;
        auto tick_begin() -> void;
        auto tick_end() -> void;
        void load_fonts();

        std::vector<std::shared_ptr<subsystem>> m_subsystems;
        std::vector<std::shared_ptr<manager>> m_managers;

        std::chrono::high_resolution_clock::time_point m_currentTime;
        std::chrono::high_resolution_clock::time_point m_previousTime;

        std::string m_executableLocation;
        std::string m_applicationDisplayTitle;

        std::unordered_map<fonts, ImFont*> m_fonts;

        std::shared_ptr<concurrencpp::manual_executor> m_tickExecutor;

        bool m_hasInit          = false;
        bool m_hasStarted       = false;
        bool m_isRequestingExit = false;

        double m_deltaTime = 0.0;
    };

    template <class T>
    std::shared_ptr<T> app::add_subsystem_class()
    {
        std::shared_ptr<T> subsystemPtr = std::make_shared<T>(this);
        m_subsystems.push_back(subsystemPtr);
        assert(subsystemPtr);

        if (m_hasInit)
        {
            subsystemPtr->pre_init(0, NULL);
            subsystemPtr->init();
        }

        return subsystemPtr;
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

    template <class T, typename... Args>
    std::shared_ptr<T> app::add_manager_class(Args&&... args)
    {
        m_managers.push_back(std::make_shared<T>(this, std::forward<Args>(args)...));
        std::shared_ptr<manager> managerPtr = m_managers.back();

        if (managerPtr)
        {
            managerPtr->init(this);

            if (m_hasStarted)
            {
                managerPtr->start();
            }
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
