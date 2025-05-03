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
    return SBK_SUCCESS;
}

sbk_result sbk_system_get_object_count(uint64_t* count)
{
    SC_CHECK_ARG(count != NULL);
    sbk::engine::system* const system = sbk::engine::system::get();
    SC_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

    *count = system->get_database_object_count();
    return SBK_SUCCESS;
}

sbk_result sbk_system_get_object_info(uint64_t index, sbk_id* id, char* name, uint64_t nameSize, uint64_t* actualNameSize)
{
    SC_CHECK_ARG(index != NULL);
    SC_CHECK_ARG(name != NULL);
    SC_CHECK_ARG(nameSize > 0);
    SC_CHECK_ARG(actualNameSize != NULL);

    sbk::engine::system* const system = sbk::engine::system::get();
    SC_CHECK(system != NULL, SBK_ERR_BAKERY_UNINITIALIZED);

    if (const std::shared_ptr<sbk::core::database_object> object = system->get_database_object_at(index).lock())
    {
        const sbk_id objectID        = object->get_database_id();
        const std::string objectName = object->get_database_name();

        *id = objectID;
        *actualNameSize = objectName.copy(name, nameSize);
        return SBK_SUCCESS;
    }
    return SBK_ERR_BAKERY_OBJECT_NOT_FOUND;
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