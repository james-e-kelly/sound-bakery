#include "App/App.h"

extern App* CreateApplication();

#if defined(WIN32)
    #include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE hPrevInstance,
                   _In_ PSTR pCmdLine,
                   _In_ int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    App* app = CreateApplication();

    app->Run(__argc, __argv);

    delete app;

    return 0;
}

#else

int main(int argc, char* argv[])
{
    App* app = CreateApplication();

    app->Run(__argc, __argv);

    delete app;

    return 0;
}
#endif