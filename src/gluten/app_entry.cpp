#include "app/app.h"

extern gluten::app* CreateApplication();

#if defined(WIN32)
    #include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    gluten::app* app = CreateApplication();

    app->run(__argc, __argv);

    delete app;

    return 0;
}

#else

int main(int argc, char* argv[])
{
    app* app = CreateApplication();

    app->run(__argc, __argv);

    delete app;

    return 0;
}
#endif