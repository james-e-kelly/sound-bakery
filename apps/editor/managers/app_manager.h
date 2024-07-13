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
    void init(gluten::app* app) override;

    /*
     * Opens a file dialogue window to create the project and handles creating
     * files.
     */
    void CreateNewProject();

    /*
     * Opens a file dialogue and opens the project is found.
     */
    void open_project();
};
