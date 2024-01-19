#pragma once

#include "Manager.h"

class Widget;

class AppManager : public Manager
{
public:
    AppManager(App* appOwner) : Manager(appOwner) {}

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
    void OnSplashWidgetDestroy(Widget* widget);
};
