#include "sound_chef_internal.h"

#define CLAP_ENTRY "clap_entry"

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

	ma_handle pluginHandle = ma_dlopen(NULL, clapFilePath);
    SC_CHECK(pluginHandle != NULL, MA_ERROR);

    clap_plugin_entry_t* const clapEntry = (clap_plugin_entry_t*)ma_dlsym(NULL, pluginHandle, CLAP_ENTRY);
    SC_CHECK_AND_GOTO(clapEntry != NULL, error_dll);

    if (clapEntry->init(clapFilePath))
    {
        const clap_plugin_factory_t* pluginFactory = (const clap_plugin_factory_t*)clapEntry->get_factory(CLAP_PLUGIN_FACTORY_ID);
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
    ma_dlclose(NULL, pluginHandle);

    return MA_ERROR;
}

sc_result sc_clap_unload(sc_clap* clapPlugin) 
{ 
    SC_CHECK_ARG(clapPlugin != NULL); 
    SC_CHECK_ARG(clapPlugin->dynamicLibraryHandle != NULL);
    SC_CHECK_ARG(clapPlugin->clapEntry != NULL);
    SC_CHECK_ARG(clapPlugin->pluginFactory != NULL);

    clapPlugin->clapEntry->deinit();
    ma_dlclose(NULL, clapPlugin->dynamicLibraryHandle);

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