#include "sound_bakery/sound_bakery.h"

#include "sound_bakery/system.h"

sb_system_config sb_system_config_init_default()
{
    sb_system_config config;
    std::memset(&config, 0, sizeof(sb_system_config));
    config.soundChefConfig = sc_system_config_init_default();
    return config;
}

sb_system_config sb_system_config_init(const char* pluginPath)
{
    sb_system_config config           = sb_system_config_init_default();
    config.soundChefConfig.pluginPath = pluginPath;
    return config;
}

sb_result sb_system_create() 
{ 
    return sbk::engine::system::create(); 
}

sb_result sb_system_init(sb_system_config config)
{ 
    return sbk::engine::system::init(config); 
}

sb_result sb_system_update()
{
    return sbk::engine::system::update();
}

sb_result sb_system_destroy()
{ 
    sbk::engine::system::destroy();
    return MA_SUCCESS;
}