#pragma once

#include "boost/program_options.hpp"
#include "concurrencpp/concurrencpp.h"
#include "core/leak_detector.h"
#include "core/logger.h"
#include "gluten/managers/manager.h"
#include "gluten/subsystems/subsystem.h"
#include "gluten/theme/theme.h"
#include "imgui.h"

#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include <rttr/type>

namespace gluten
{
    static inline constexpr const char* g_serializedEntryName = "data";

    /**
     * @brief Manages application lifetime and object owning.
     *
     * The app class is intended to pass application behaviour to manager and subsystem classes.
     */
    class app : public concurrencpp::runtime, public sbk::core::logger
    {
        LEAK_DETECTOR(app)

    public:
        app();
        virtual ~app() = default;

        static app* get();

        int run(int argc, char** argv);
        void request_exit();

        template <class T>
        std::shared_ptr<T> add_unique_subsystem_class();

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

        template <typename T>
        static auto save_data_to_disk(const std::filesystem::path& file, const T& data) -> void
        {
            std::filesystem::create_directories(file.parent_path());

            std::ofstream outputStream(file, std::ios_base::out);
            boost::archive::xml_oarchive archive(outputStream);

            archive& boost::serialization::make_nvp(g_serializedEntryName, data);
        }

        template <typename T>
        static auto load_data_from_disk(const std::filesystem::path& file, T& data) -> void
        {
            std::ifstream inputStream(file, std::ios_base::in);
            boost::archive::xml_iarchive archive(inputStream);

            archive& boost::serialization::make_nvp(g_serializedEntryName, data);
        }

        auto get_tick_executor() const -> std::shared_ptr<concurrencpp::manual_executor>
        {
            return m_tickExecutor;
        }

        auto get_executable_location() const -> std::string
        {
            return m_executableLocation;
        }

    protected:
        /**
         * @brief Runs at the earliest possible time and before the parsing of command line arguments.
         *
         * Use this function to set up command line arguments. Read the parsed values in @see pre_init.
         */
        virtual auto cli_setup(boost::program_options::options_description& options) -> void {}

        /**
         * @brief Runs after subsystems are created and before any init functions are called.
         *
         * Use this function to create the root widget, managers, more subsystems or general initialization.
         */
        virtual auto pre_init(const boost::program_options::variables_map& cliVariables) -> void {}

        /**
         * @brief Runs after all init functions were called and before start.
         */
        virtual auto post_init() -> void {}

        /**
         * @brief Runs before everything has exited and when the app is about to exit.
         *
         * The subsystems and managers will still be valid at this point.
         */
        virtual auto exit() -> void {}

        virtual auto tick_implementation() -> void {}

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
    std::shared_ptr<T> app::add_unique_subsystem_class()
    {
        // Should only have one subsystem of each class. Therefore, return the existing
        // one, if found
        if (std::shared_ptr<T> foundSubsystem = get_subsystem_by_class<T>())
        {
            return foundSubsystem;
        }

        get_logger()->info(fmt::format("Creating a subsystem of type {}", rttr::type::get<T>().get_name().data()));

        std::shared_ptr<T> subsystemPtr = std::make_shared<T>(this);
        m_subsystems.push_back(subsystemPtr);
        assert(subsystemPtr);

        if (m_hasInit)
        {
            subsystemPtr->pre_init({});
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
        return {};
    }

    template <class T>
    void app::remove_subsystem_by_class()
    {
        for (int index = m_subsystems.size() - 1; index >= 0; --index)
        {
            if (T* castedSubsytem = dynamic_cast<T*>(m_subsystems[index].get()))
            {
                get_logger()->info(fmt::format("Removing a subsystem of type {}", rttr::type::get<T>().get_name().data()));

                castedSubsytem->exit();
                m_subsystems.erase(m_subsystems.begin() + index);
                return;
            }
        }
    }

    template <class T, typename... Args>
    std::shared_ptr<T> app::add_manager_class(Args&&... args)
    {
        get_logger()->info(fmt::format("Creating a manager of type {}", rttr::type::get<T>().get_name().data()));

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
                get_logger()->info(fmt::format("Removing a manager of type {}", rttr::type::get<T>().get_name().data()));

                castedManager->exit();
                m_managers.erase(m_managers.begin() + index);
                return;
            }
        }
    }
}  // namespace gluten
