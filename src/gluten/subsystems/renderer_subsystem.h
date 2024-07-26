#pragma once

#include "gluten/subsystems/subsystem.h"

struct GLFWwindow;

namespace gluten
{
    class renderer_subsystem : public subsystem
    {
    public:
        renderer_subsystem(app* appOwner) : subsystem(appOwner) {}

    public:
        /** AppSubsystem Implementation */

        virtual int pre_init(int ArgC, char* ArgV[]) override;
        virtual int init() override;
        virtual void pre_tick(double deltaTime) override;
        virtual void tick(double deltaTime) override;
        virtual void tick_rendering(double deltaTime) override;
        virtual void exit() override;

        void set_window_title(const std::string& title);

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