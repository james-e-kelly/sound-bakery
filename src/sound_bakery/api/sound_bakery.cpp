#include "sound_bakery/sound_bakery.h"

#include "sound_bakery/system.h"

static sbk_id convert_pointer_to_id(void* ptr)
{
    return reinterpret_cast<sbk_id>(ptr);
}

template<class T>
static T* convert_id_to_pointer(sbk_id id)
{
    return reinterpret_cast<T*>(id);
}

sbk_system_config sbk_system_config_init_default()
{
    sbk_system_config config;
    std::memset(&config, 0, sizeof(sbk_system_config));
    config.soundChefConfig = sc_system_config_init_default();
    return config;
}

sbk_system_config sbk_system_config_init(const char* pluginPath)
{
    sbk_system_config config           = sbk_system_config_init_default();
    config.soundChefConfig.pluginPath = pluginPath;
    return config;
}

sbk_result sbk_system_create() 
{ 
    return sbk::engine::system::create(); 
}

sbk_result sbk_system_init(sbk_system_config config)
{ 
    return sbk::engine::system::init(config); 
}

sbk_result sbk_system_update()
{
    return sbk::engine::system::update();
}

sbk_result sbk_system_destroy()
{ 
    sbk::engine::system::destroy();
    return MA_SUCCESS;
}

sbk_result sbk_system_load_soundbank(const char* soundbankFilePath, sbk_soundbank** outSoundbank)
{
    SC_CHECK_ARG(soundbankFilePath != NULL);
    SC_CHECK_ARG(outSoundbank != NULL);

    sbk_id soundbankID = 0;
    const sbk_result loadResult = sbk::engine::system::load_soundbank(soundbankFilePath, soundbankID);
    SC_CHECK_RESULT(loadResult);

    *outSoundbank = convert_id_to_pointer<sbk_soundbank>(soundbankID);
    return loadResult;
}