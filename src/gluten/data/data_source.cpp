#include "data_source.h"

#include "platform_folders.h"

std::filesystem::path gluten::get_config_file(const rttr::type& type)
    {
        const std::string typeFileName = type.get_name().to_string() + ".xml";
        return std::filesystem::path(sago::getConfigHome()) / "JamesKelly" / "SoundBakery" / typeFileName;
    }