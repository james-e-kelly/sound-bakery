#pragma once

#include "gluten/managers/manager.h"
#include "widgets/splash_widget.h"

namespace gluten
{
    class widget;
}

class app_manager : public gluten::manager
{
public:
    app_manager(gluten::app* appOwner) : manager(appOwner) {}

public:
    void init(gluten::app* app) override;

    /*
     * Opens a file dialogue window to create the project and handles creating
     * files.
     */
    void create_new_project();

    /*
     * Opens a file dialogue and opens the project is found.
     */
    void open_project();

private:
    std::shared_ptr<splash_widget> m_splashWidget;
};
