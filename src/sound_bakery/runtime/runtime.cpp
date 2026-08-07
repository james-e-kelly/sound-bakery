#include "runtime.h"

#include "sound_bakery/gameobject/gameobject.h"

namespace
{
    auto miniaudio_log_callback(void* pUserData, ma_uint32 level, const char* pMessage) -> void
    {
        (void)pUserData;

        switch (level)
        {
            case MA_LOG_LEVEL_DEBUG:
                // Purposefully drop debug messages for now
                break;
            case MA_LOG_LEVEL_INFO:
                SBK_INFO("{}", pMessage);
                break;
            case MA_LOG_LEVEL_WARNING:
                SBK_WARN("{}", pMessage);
                break;
            case MA_LOG_LEVEL_ERROR:
                SBK_ERROR("{}", pMessage);
                break;
            default:
                break;
        }
    }
}

sbk::engine::runtime::runtime() : sc_system()
{
    const sbk_status initLogResult = sc_system_log_init(this, miniaudio_log_callback);
    sbk::log_error(initLogResult, "sc_system_log_init");
}

sbk::engine::runtime::~runtime()
{
    m_masterBus.reset();
    m_listenerGameObject.reset();

    remove_all();

    const sbk_status closeResult = sc_system_close(this);
    sbk::log_error(closeResult, "sc_system_close");
    m_initSoundChef = false;
}

auto sbk::engine::runtime::init(const sc_system_config& config) -> sbk::result<>
{
    SBK_CHECK(m_initSoundChef == false, SBK_ERR_BAKERY);
    SBK_TRY_C(sc_system_init(this, &config));
    m_initSoundChef = true;

    SBK_TRY(auto listener, create_database_object<sbk::engine::game_object>());
    listener->set_object_name("Listener");
    listener->set_editor_hidden(true);
    m_listenerGameObject = listener;

    return sbk::ok();
}

auto sbk::engine::runtime::get_listener_game_object() const -> std::shared_ptr<sbk::engine::game_object>
{
    return m_listenerGameObject.lock();
}

auto sbk::engine::runtime::get_master_bus() const -> std::shared_ptr<sbk::engine::bus>
{
    return m_masterBus.lock();
}