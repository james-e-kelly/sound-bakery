#pragma once

#include "gluten/managers/manager.h"

namespace gluten
{
    class widget;
}

class AppManager : public gluten::manager
{
public:
    AppManager(gluten::app* appOwner) : manager(appOwner) {}

public:
    void Init(const std::string& executablePath);
    virtual void Tick(double deltaTime) {}
    virtual void Exit() {}

public:
    double GetDeltaTime() const;

public:
    /*
     * Opens a file dialogue window to create the project and handles creating
     * files.
     */
    void CreateNewProject();

    /*
     * Opens a file dialogue and opens the project is found.
     */
    void OpenProject();

private:
    void OnSplashWidgetDestroy(gluten::widget* widget);
};
