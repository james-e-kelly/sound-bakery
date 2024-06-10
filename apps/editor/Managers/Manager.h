#pragma once

#include "pch.h"

class app;

/**
 * @brief Manager class that can handle application operations or project operations.
 * For example, the app subclass can handle creating the root widget, file browsers and such.
 * The project subclass can handle loading project files and widgets needed only for the project.
 * 
 */
class Manager
{
public:
    Manager() = delete;
    Manager(app* appOwner)
    : m_app(appOwner) { }

public:

    /**
     * @brief Called every frame regardless of if the app is closing
     * 
     */
    virtual void Tick(double deltaTime) {}

    /**
     * @brief Called when closing the app
     * 
     */
    virtual void Exit() {}
    
public:
    app* GetApp() const { return m_app; }

protected:
    app* m_app = nullptr;
};
