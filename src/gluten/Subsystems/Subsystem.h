#pragma once

#include "pch.h"

namespace gluten
{
    class app;

    class subsystem
    {
    public:
        subsystem() = delete;
        subsystem(app* appOwner) : m_app(appOwner) {}

    public:
        /**
         * @brief Runs as early as possible and provides command line arguments
         *
         * @param ArgC Command line arguments count
         * @param ArgV Command line arguments array
         * @return int Returns for success and greater than 0 for error
         */
        virtual int pre_init(int ArgC, char* ArgV[]) { return 0; }

        /**
         * @brief Init the subsystem / start
         *
         * @return int Returns for success and greater than 0 for error
         */
        virtual int init() { return 0; }

        /**
         * @brief Runs before to tick to get if the app should close or set up a new frame
         *
         */
        virtual void pre_tick(double deltaTime) {}

        /**
         * @brief Called every frame regardless of if the app is closing
         *
         */
        virtual void tick(double deltaTime) {}

        /**
         * @brief Called every frame if the app is NOT closing
         *
         */
        virtual void tick_rendering(double deltaTime) {}

        /**
         * @brief Called when closing the app
         *
         */
        virtual void exit() {}

    public:
        app* get_app() const { return m_app; }

    protected:
        app* m_app = nullptr;
    };
}  // namespace