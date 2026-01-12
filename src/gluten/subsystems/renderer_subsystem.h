#pragma once

#include "gluten/subsystems/subsystem.h"

struct GLFWwindow;

typedef void (*GLFWglproc)(void);

namespace gluten
{
    class renderer_subsystem : public subsystem
    {
    public:
        renderer_subsystem(app* appOwner) : subsystem(appOwner) {}

    public:
        /** AppSubsystem Implementation */
        virtual int pre_init(const boost::program_options::variables_map& cliVariables) override;
        virtual int init() override;
        virtual void pre_tick(double deltaTime) override;
        virtual void tick(double deltaTime) override;
        virtual void tick_rendering(double deltaTime) override;
        virtual void exit() override;

        static auto glfw_get_proc_address(const char* procName) -> GLFWglproc;
        static auto glfw_get_proc_address_with_context(void* context, const char* procName) -> GLFWglproc;

        void set_window_title(const std::string& title);

        void toggle_minimised();
        void toggle_maximised();
        void set_maximised();

        bool is_minimised() const;
        bool is_maximized() const;

        auto get_glfw_window() const -> GLFWwindow*
        {
            return m_window.m_window;
        }

        int m_startWindowWidth = 1920;
        int m_startWindowHeight = 1080;

    private:
        int init_glfw();
        int init_imgui();

        void set_default_window_hints();

    private:
        struct window_guard
        {
            window_guard() : m_window(nullptr) {}
            window_guard(int width, int height, const std::string& windowName);
            ~window_guard();

            window_guard(const window_guard& other)            = delete;
            window_guard& operator=(const window_guard& other) = delete;

            window_guard(window_guard&& other) noexcept;
            window_guard& operator=(window_guard&& other) noexcept;

            void set_title(const std::string& title);

            operator GLFWwindow*() const { return m_window; }
            GLFWwindow* operator->() const { return m_window; }

            GLFWwindow* m_window;
        };

    private:
        window_guard m_window;
        std::string m_fontPath;
    };
}  // namespace gluten
