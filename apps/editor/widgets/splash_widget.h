#pragma once

#include "gluten/widgets/widget.h"

class SplashWidget : public gluten::widget
{
public:
    SplashWidget(gluten::widget_subsystem* parentSubsystem) : widget(parentSubsystem) {}

public:
    void ShowSplashScreen();
    void CloseSplashScreen();

public:
    virtual void tick(double deltaTime) override;
    virtual void render() override;

protected:
    virtual void start() override;

private:
    bool m_wantsToShow         = false;
    double m_timeShowingScreen = 0.0;
    std::string m_loadingText  = std::string("Loading");

    uint32_t m_splashImageID = 0;
    int m_splashImageWidth   = -1;
    int m_splashImageHeight  = -1;
};
