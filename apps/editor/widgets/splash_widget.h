#pragma once

#include "gluten/widgets/widget.h"

class splash_widget : public gluten::widget
{
public:
    splash_widget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem, "Splash Widget") {}

public:
    void show_splash_screen();
    void close_splash_screen();

protected:
    virtual void start_implementation() override;
    virtual void tick_implementation(double deltaTime) override;
    virtual void render_implementation() override;

private:
    bool m_wantsToShow         = false;
    double m_timeShowingScreen = 0.0;
    std::string m_loadingText  = std::string("Loading");

    uint32_t m_splashImageID = 0;
    int m_splashImageWidth   = -1;
    int m_splashImageHeight  = -1;
};
