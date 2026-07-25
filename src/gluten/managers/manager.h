#pragma once

#include "gluten/pch.h"

#include "core/leak_detector.h"

namespace gluten
{
    class app;

    /**
     * @brief Manager class that can handle application operations or project operations.
     * For example, the app subclass can handle creating the root widget, file browsers and such.
     * The project subclass can handle loading project files and widgets needed only for the project.
     *
     */
    class manager
    {
        LEAK_DETECTOR(manager)

    public:
        manager() = delete;
        manager(app* appOwner) : m_app(appOwner) {}
        virtual ~manager() = default;

    public:
        virtual void init(app* app) {}

        virtual void start() {}

        /**
         * @brief Called every frame regardless of if the app is closing
         *
         */
        virtual void tick(double deltaTime) {}

        /**
         * @brief Called when closing the app
         *
         */
        virtual void exit() {}

    public:
        app* get_app() const;

    protected:
        app* m_app = nullptr;
    };
}  // namespace gluten