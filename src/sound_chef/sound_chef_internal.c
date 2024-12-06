#include "sound_chef_internal.h"

#define CLAP_ENTRY "clap_entry"

#if defined(MA_WIN32)
    #include <windows.h>
#endif

#ifdef MA_POSIX
    /* No need for dlfcn.h if we're not using runtime linking. */
    #ifndef MA_NO_RUNTIME_LINKING
        #include <dlfcn.h>
    #endif
#endif

ma_handle sc_dlopen(ma_log* pLog, const char* filename)
{
#ifndef MA_NO_RUNTIME_LINKING
    ma_handle handle;

    ma_log_postf(pLog, MA_LOG_LEVEL_DEBUG, "Loading library: %s\n", filename);

    #ifdef MA_WIN32
        /* From MSDN: Desktop applications cannot use LoadPackagedLibrary; if a desktop application calls this function
         * it fails with APPMODEL_ERROR_NO_PACKAGE.*/
        #if !defined(MA_WIN32_UWP) ||   \
            !(defined(WINAPI_FAMILY) && \
              ((defined(WINAPI_FAMILY_PHONE_APP) && WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP)))
    handle = (ma_handle)LoadLibraryA(filename);
        #else
    /* *sigh* It appears there is no ANSI version of LoadPackagedLibrary()... */
    WCHAR filenameW[4096];
    if (MultiByteToWideChar(CP_UTF8, 0, filename, -1, filenameW, sizeof(filenameW)) == 0)
    {
        handle = NULL;
    }
    else
    {
        handle = (ma_handle)LoadPackagedLibrary(filenameW, 0);
    }
        #endif
    #else
    handle = (ma_handle)dlopen(filename, RTLD_NOW);
    #endif

    /*
    I'm not considering failure to load a library an error nor a warning because seamlessly falling through to a
    lower-priority backend is a deliberate design choice. Instead I'm logging it as an informational message.
    */
    if (handle == NULL)
    {
        ma_log_postf(pLog, MA_LOG_LEVEL_INFO, "Failed to load library: %s\n", filename);
    }

    return handle;
#else
    /* Runtime linking is disabled. */
    (void)pLog;
    (void)filename;
    return NULL;
#endif
}

void sc_dlclose(ma_log* pLog, ma_handle handle)
{
#ifndef MA_NO_RUNTIME_LINKING
    #ifdef MA_WIN32
    FreeLibrary((HMODULE)handle);
    #else
    dlclose((void*)handle);
    #endif

    (void)pLog;
#else
    /* Runtime linking is disabled. */
    (void)pLog;
    (void)handle;
#endif
}

ma_proc sc_dlsym(ma_log* pLog, ma_handle handle, const char* symbol)
{
#ifndef MA_NO_RUNTIME_LINKING
    ma_proc proc;

    ma_log_postf(pLog, MA_LOG_LEVEL_DEBUG, "Loading symbol: %s\n", symbol);

    #ifdef _WIN32
    proc = (ma_proc)GetProcAddress((HMODULE)handle, symbol);
    #else
        #if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpedantic"
        #endif
    proc = (ma_proc)dlsym((void*)handle, symbol);
        #if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8))
            #pragma GCC diagnostic pop
        #endif
    #endif

    if (proc == NULL)
    {
        ma_log_postf(pLog, MA_LOG_LEVEL_WARNING, "Failed to load symbol: %s\n", symbol);
    }

    (void)pLog; /* It's possible for pContext to be unused. */
    return proc;
#else
    /* Runtime linking is disabled. */
    (void)pLog;
    (void)handle;
    (void)symbol;
    return NULL;
#endif
}

const char* SC_API sc_filename_get_ext(const char* filename)
{
    if (filename != NULL)
    {
        const char* dot = strrchr(filename, '.');
        if (!dot || dot == filename)
        {
            return "";
        }
        return dot + 1;
    }
    return "";
}

sc_result sc_clap_load(const char* clapFilePath, sc_clap* clapPlugin)
{
    SC_CHECK_ARG(clapFilePath != NULL);
    SC_CHECK_ARG(clapPlugin != NULL);

    SC_ZERO_OBJECT(clapPlugin);

    ma_handle pluginHandle = sc_dlopen(NULL, clapFilePath);
    SC_CHECK(pluginHandle != NULL, MA_ERROR);

    clap_plugin_entry_t* const clapEntry = (clap_plugin_entry_t*)sc_dlsym(NULL, pluginHandle, CLAP_ENTRY);
    SC_CHECK_AND_GOTO(clapEntry != NULL, error_dll);

    if (clapEntry->init(clapFilePath))
    {
        const clap_plugin_factory_t* pluginFactory =
            (const clap_plugin_factory_t*)clapEntry->get_factory(CLAP_PLUGIN_FACTORY_ID);
        SC_CHECK_AND_GOTO(pluginFactory != NULL, error_clap);

        const ma_uint32 pluginCount = pluginFactory->get_plugin_count(pluginFactory);
        SC_CHECK_AND_GOTO(pluginCount > 0, error_clap);

        clapPlugin->dynamicLibraryHandle = pluginHandle;
        clapPlugin->clapEntry            = clapEntry;
        clapPlugin->pluginFactory        = pluginFactory;

        return MA_SUCCESS;
    }
    else
    {
        goto error_dll;
    }

error_clap:
    clapEntry->deinit();
error_dll:
    sc_dlclose(NULL, pluginHandle);

    return MA_ERROR;
}

sc_result sc_clap_unload(sc_clap* clapPlugin)
{
    SC_CHECK_ARG(clapPlugin != NULL);
    SC_CHECK_ARG(clapPlugin->dynamicLibraryHandle != NULL);
    SC_CHECK_ARG(clapPlugin->clapEntry != NULL);
    SC_CHECK_ARG(clapPlugin->pluginFactory != NULL);

    clapPlugin->clapEntry->deinit();
    sc_dlclose(NULL, clapPlugin->dynamicLibraryHandle);

    SC_ZERO_OBJECT(clapPlugin);

    return MA_SUCCESS;
}

sc_result sc_system_release_clap_plugins(sc_system* system)
{
    SC_CHECK_ARG(system != NULL);
    SC_CHECK_ARG(system->clapPlugins != NULL);

    for (int index = 0; index < arrlen(system->clapPlugins); ++index)
    {
        const sc_result unloadResult = sc_clap_unload(&system->clapPlugins[index]);
        assert(unloadResult == MA_SUCCESS);
    }

    arrfree(system->clapPlugins);
    system->clapPlugins = NULL;

    return MA_SUCCESS;
}