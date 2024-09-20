#include "app/app.h"

/**
 * @brief Create a new application class.
 * 
 * All consuming libraries should implement this function.
 * 
 * @warn Ensure the object is created on the heap as gluten will delete the pointer upon close.
*/
extern gluten::app* create_application();

#if defined(WIN32)
    #include <windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PSTR pCmdLine, _In_ int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)pCmdLine;
    (void)nCmdShow;

    gluten::app* app = create_application();

    app->run(__argc, __argv);

    delete app;

    return 0;
}

#else

int main(int argc, char* argv[])
{
    gluten::app* app = create_application();

    app->run(argc, argv);

    delete app;

    return 0;
}
#endif
