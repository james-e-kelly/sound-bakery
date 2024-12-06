#include "sound_bakery/sound_bakery.h"

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