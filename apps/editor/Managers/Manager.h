#pragma once

#include "pch.h"

class App;

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
    Manager(App* appOwner)
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
    App* GetApp() const { return m_app; }

protected:
    App* m_app = nullptr;
};
