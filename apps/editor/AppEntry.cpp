#include "App/App.h"

#if defined(WIN32)
#include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
    App app;

    return app.Run(__argc, __argv);
}

#else

int main(int argc, char* argv[])
{
    App app;

    return app.Run(argc, argv);
}
#endif