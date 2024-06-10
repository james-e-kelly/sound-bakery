#pragma once

#include "pch.h"

class App;

class Subsystem
{
public:
    Subsystem() = delete;
    Subsystem(App* appOwner)
    : m_app(appOwner) { }

public:
    /**
     * @brief Runs as early as possible and provides command line arguments
     * 
     * @param ArgC Command line arguments count
     * @param ArgV Command line arguments array
     * @return int Returns for success and greater than 0 for error
     */
    virtual int PreInit(int ArgC, char* ArgV[]) { return 0; }

    /**
     * @brief Init the subsystem / start
     * 
     * @return int Returns for success and greater than 0 for error
     */
    virtual int Init() { return 0; }

    /**
     * @brief Runs before to tick to get if the app should close or set up a new frame
     * 
     */
    virtual void PreTick(double deltaTime) {}

    /**
     * @brief Called every frame regardless of if the app is closing
     * 
     */
    virtual void Tick(double deltaTime) {}

    /**
     * @brief Called every frame if the app is NOT closing
     * 
     */
    virtual void TickRendering(double deltaTime) {}

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
