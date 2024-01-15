#pragma once

#include "Subsystem.h"

struct GLFWwindow;

class RendererSubsystem : public Subsystem
{
public:
    RendererSubsystem(App* appOwner) : Subsystem(appOwner) {}

public:
    /** AppSubsystem Implementation */

    virtual int PreInit(int ArgC, char* ArgV[]) override;
    virtual int Init() override;
    virtual void PreTick(double deltaTime) override;
    virtual void Tick(double deltaTime) override;
    virtual void TickRendering(double deltaTime) override;
    virtual void Exit() override;

private:
    int InitGLFW();
    int InitImGui();

    void SetDefaultWindowHints();

private:
    struct WindowGuard
    {
        WindowGuard() : m_window(nullptr) {}
        WindowGuard(int width, int height, const std::string& windowName);
        ~WindowGuard();

        // Add move operator
        // This class should always own the window
        WindowGuard(WindowGuard&& other) noexcept;
        WindowGuard& operator=(WindowGuard&& other) noexcept;

        // Delete the copy constructor and copy assignment operator to prevent
        // copying
        WindowGuard(const WindowGuard& other)            = delete;
        WindowGuard& operator=(const WindowGuard& other) = delete;

        operator GLFWwindow*() const { return m_window; }
        GLFWwindow* operator->() const { return m_window; }

        GLFWwindow* m_window;
    };

private:
    WindowGuard m_window;
    std::string m_fontPath;
};
