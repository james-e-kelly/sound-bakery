#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "sound_bakery/system.h"

#include "sound_bakery/core/property.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/bus/aux_bus.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/profiling/remote_session.h"
#include "sound_bakery/profiling/remote_session_host.h"

#include <chrono>

namespace
{
    /**
     * @brief RAII helper that spins up a real Sound Bakery system for the
     * duration of a test and guarantees it is destroyed afterwards.
     *
     * Any test that needs reflection, the database, or object creation should
     * declare one of these at the top of its body.
     */
    struct scoped_engine
    {
        scoped_engine()
        {
            sbk_system_config config = sbk_system_config_init_default();
            config.logToConsole      = true;

            REQUIRE(sbk::engine::system::create().has_value());
            REQUIRE(sbk::engine::system::get() != nullptr);
            REQUIRE(sbk::engine::system::get()->init(config).has_value());
        }

        ~scoped_engine() { sbk::engine::system::destroy(); }

        [[nodiscard]] auto get() const -> sbk::engine::system* { return sbk::engine::system::get(); }
    };
}  // namespace

TEST_SUITE("System")
{
    TEST_CASE("System Creation Deletion")
    {
        sbk_system_config config = sbk_system_config_init_default();

        REQUIRE(sbk::engine::system::create().has_value());
        REQUIRE(sbk::engine::system::get()->init(config).has_value());
        REQUIRE(sbk::engine::system::get()->update().has_value());
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }

    TEST_CASE("Re-init")
    {
        sbk_system_config config = sbk_system_config_init_default();

        REQUIRE(sbk::engine::system::create().has_value());
        REQUIRE(sbk::engine::system::get()->init(config).has_value());
        REQUIRE(sbk::engine::system::get()->update().has_value());
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);

        REQUIRE(sbk::engine::system::create().has_value());
        REQUIRE(sbk::engine::system::get()->init(config).has_value());
        REQUIRE(sbk::engine::system::get()->update().has_value());
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }

    TEST_CASE("Get before create returns null")
    {
        // A fresh system should not exist until create() is called.
        REQUIRE(sbk::engine::system::get() == nullptr);
    }

    TEST_CASE("Runtime operating mode without a project")
    {
        scoped_engine engine;

        // With no project opened we should be in runtime mode and have no
        // editor project.
        CHECK(sbk::engine::system::get_operating_mode() == sbk::engine::system::operating_mode::runtime);
        CHECK(sbk::engine::system::get_project() == nullptr);
    }

    TEST_CASE("Listener game object exists after init")
    {
        scoped_engine engine;

        // The listener is created during init. The master bus, by contrast, is
        // only established once a project/soundbank is loaded, so it stays null
        // after a bare init.
        CHECK(engine.get()->get_listener_game_object() != nullptr);
        CHECK(engine.get()->get_master_bus() == nullptr);
    }

    TEST_CASE("System reports objects existing")
    {
        scoped_engine engine;

        // The listener should exist
        CHECK(engine.get()->get_objects_count() > 0);
    }

    TEST_CASE("System reports correct types")
    {
        scoped_engine engine;

        // The listener should exist
        CHECK(engine.get()->get_objects_of_type(sbk::engine::game_object::type()).size() > 0);
    }

    TEST_CASE("System reports correct categories")
    {
        scoped_engine engine;

        // The listener should exist
        CHECK(engine.get()->get_objects_of_category(SB_CATEGORY_RUNTIME_OBJECT).size() > 0);
    }

    TEST_CASE("Can set the master bus")
    {
        scoped_engine engine;

        auto bus = engine.get()->create_database_object<sbk::engine::bus>(false);
        CHECK(bus.has_value());

        auto busShared = bus.value();

        busShared->set_master_bus(true);
        CHECK(busShared->is_master_bus() == true);
        CHECK(engine.get()->get_master_bus() == busShared);
    }

    TEST_CASE("Creating objects increases object count correctly")
    {
        scoped_engine engine;

        const std::size_t countBefore = engine.get()->get_objects_count();

        auto bus = engine.get()->create_database_object<sbk::engine::bus>();
        CHECK(bus.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 1);

        auto bus2 = engine.get()->create_database_object<sbk::engine::bus>();
        CHECK(bus2.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 2);

        auto auxBus = engine.get()->create_database_object<sbk::engine::aux_bus>();
        CHECK(auxBus.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 3);
        
        auto auxBus2 = engine.get()->create_database_object<sbk::engine::aux_bus>();
        CHECK(auxBus2.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 4);
    }

    TEST_CASE("Spamming update works")
    {
        scoped_engine engine;

        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
        CHECK(engine.get()->update().has_value());
    }
}

TEST_SUITE("Property")
{
    using sbk::core::float_property;
    using sbk::core::int_property;

    TEST_CASE("Default construction")
    {
        float_property property;

        CHECK(property.get() == doctest::Approx(0.0F));
    }

    TEST_CASE("Min/max construction stores the range")
    {
        float_property property(0.5F, 0.0F, 2.0F);

        CHECK(property.get() == doctest::Approx(0.5F));

        const auto minMax = property.get_min_max_pair();
        CHECK(minMax.first == doctest::Approx(0.0F));
        CHECK(minMax.second == doctest::Approx(2.0F));
    }

    TEST_CASE("Set within range succeeds and updates the value")
    {
        float_property property(0.0F, 0.0F, 1.0F);

        CHECK(property.set(0.75F));
        CHECK(property.get() == doctest::Approx(0.75F));
    }

    TEST_CASE("Set out of range is rejected")
    {
        float_property property(0.0F, 0.0F, 1.0F);

        CHECK_FALSE(property.set(2.0F));    // above max
        CHECK_FALSE(property.set(-1.0F));   // below min
        CHECK(property.get() == doctest::Approx(0.0F));
    }

    TEST_CASE("Setting the same value is a no-op")
    {
        int_property property(5, 0, 10);

        // Value is unchanged, so set() reports that nothing happened.
        CHECK_FALSE(property.set(5));
        CHECK(property.get() == 5);
    }

    TEST_CASE("Changing the value broadcasts the delegate with old and new")
    {
        float_property property(0.0F, 0.0F, 1.0F);

        float broadcastOld = -1.0F;
        float broadcastNew = -1.0F;
        int   callCount    = 0;

        property.get_delegate().AddLambda(
            [&](float oldValue, float newValue)
            {
                broadcastOld = oldValue;
                broadcastNew = newValue;
                ++callCount;
            });

        REQUIRE(property.set(0.25F));

        CHECK(callCount == 1);
        CHECK(broadcastOld == doctest::Approx(0.0F));
        CHECK(broadcastNew == doctest::Approx(0.25F));

        // A rejected set must not fire the delegate.
        CHECK_FALSE(property.set(5.0F));
        CHECK(callCount == 1);
    }
}

TEST_SUITE("Parameter")
{
    using sbk::engine::float_parameter;
    using sbk::engine::int_parameter;

    TEST_CASE("Float parameter get/set within default range")
    {
        float_parameter parameter;

        parameter.set(0.5F);
        CHECK(parameter.get() == doctest::Approx(0.5F));
    }

    TEST_CASE("Float parameter default value round-trips")
    {
        float_parameter parameter;

        parameter.set_default(0.25F);
        CHECK(parameter.get_default() == doctest::Approx(0.25F));
    }

    TEST_CASE("Setting min and max")
    {
        float_parameter parameter;

        parameter.set_min(-100.0F);
        CHECK(parameter.get_min() == doctest::Approx(-100.0F));

        parameter.set_max(200.0f);
        CHECK(parameter.get_max() == doctest::Approx(200.0F));
    }

    TEST_CASE("Setting min or max clamps the value")
    {
        float_parameter parameter;

        parameter.set_min(0.0F);
        parameter.set_max(100.0F);
        parameter.set(100.0F);
        CHECK(parameter.get() == doctest::Approx(100.0F));

        parameter.set_max(1.0F);
        CHECK(parameter.get() == doctest::Approx(1.0F));
    }

    TEST_CASE("Int parameter rejects out-of-range set")
    {
        int_parameter parameter;

        parameter.set(1);
        CHECK(parameter.get() == 1);

        parameter.set_max(500);
        parameter.set(600);
        CHECK(parameter.get() == 1);
    }

    TEST_CASE("Parameter set forwards to the change delegate")
    {
        float_parameter parameter;

        int callCount = 0;
        parameter.get_delegate().AddLambda([&](float, float) { ++callCount; });

        parameter.set(0.5F);
        CHECK(callCount == 1);
    }

    TEST_CASE("create_local_parameter_from_this copies the current value")
    {
        float_parameter parameter;
        parameter.set(0.5F);

        const auto local = parameter.create_local_parameter_from_this();
        CHECK(local.second.get() == doctest::Approx(0.5F));
    }
}

TEST_SUITE("Named Parameter")
{
    TEST_CASE("Adding values and selecting them")
    {
        scoped_engine engine;

        auto parameterResult = engine.get()->create_database_object<sbk::engine::named_parameter>();
        REQUIRE(parameterResult.has_value());

        auto& parameter = parameterResult.value();

        auto walkValue = parameter->add_new_value("Walk");
        auto runValue  = parameter->add_new_value("Run");

        REQUIRE(walkValue.valid());
        REQUIRE(runValue.valid());

        parameter->set_selected_value(runValue);
        CHECK(parameter->get() == runValue.id());

        auto selected = parameter->get_selected_value();
        CHECK(selected.id() == runValue.id());

        parameter->set_selected_value(walkValue);
        CHECK(parameter->get() == walkValue.id());

        selected = parameter->get_selected_value();
        CHECK(selected.id() == walkValue.id());
    }

    TEST_CASE("Empty parameter lazily creates a 'None' value")
    {
        scoped_engine engine;

        auto parameterResult = engine.get()->create_database_object<sbk::engine::named_parameter>();
        REQUIRE(parameterResult.has_value());

        auto& parameter = parameterResult.value();

        // get_values() on an empty parameter should populate a default entry.
        const auto values = parameter->get_values();
        CHECK_FALSE(values.empty());
    }

    TEST_CASE("Empty name is ignored")
    {
        scoped_engine engine;

        auto parameterResult = engine.get()->create_database_object<sbk::engine::named_parameter>();
        REQUIRE(parameterResult.has_value());

        auto& parameter = parameterResult.value();

        const auto invalid = parameter->add_new_value("");
        CHECK_FALSE(invalid.valid());
    }
}

TEST_SUITE("Game Object")
{
    TEST_CASE("Local float parameter set/get round-trips")
    {
        scoped_engine engine;

        sbk::engine::game_object gameObject;

        constexpr sbk_id parameterId = 1234;
        gameObject.set_float_parameter({parameterId, 0.75F});

        const sbk::core::database_ptr<sbk::engine::float_parameter> parameterPtr(parameterId);
        CHECK(gameObject.get_float_parameter_value(parameterPtr) == doctest::Approx(0.75F));
    }

    TEST_CASE("Local parameters are stored on the game object")
    {
        scoped_engine engine;

        sbk::engine::game_object gameObject;
        gameObject.set_float_parameter({4321, 0.5F});

        const auto locals = gameObject.get_local_parameters();
        CHECK(locals.floatParameters.size() == 1);
    }

    TEST_CASE("Fresh game object is not playing")
    {
        scoped_engine engine;

        sbk::engine::game_object gameObject;
        CHECK_FALSE(gameObject.is_playing());
    }

    TEST_CASE("Unknown float parameter falls back to zero")
    {
        scoped_engine engine;

        sbk::engine::game_object gameObject;

        // No local value and no such parameter in the database - expect the
        // documented default of 0.
        const sbk::core::database_ptr<sbk::engine::float_parameter> unknown(999999);
        CHECK(gameObject.get_float_parameter_value(unknown) == doctest::Approx(0.0F));
    }
}

TEST_SUITE("Database")
{
    TEST_CASE("Created object is findable by id")
    {
        scoped_engine engine;

        auto objectResult = engine.get()->create_database_object<sbk::engine::float_parameter>();
        REQUIRE(objectResult.has_value());

        const sbk_id id = objectResult.value()->get_database_id();
        REQUIRE(id != 0);

        const auto found = engine.get()->try_find_database_object(id);
        CHECK_FALSE(found.expired());
    }

    TEST_CASE("Objects not added to the database cannot be found")
    {
        scoped_engine engine;

        auto objectResult = engine.get()->create_database_object<sbk::engine::float_parameter>(false);
        REQUIRE(objectResult.has_value());

        const sbk_id id = objectResult.value()->get_database_id();
        REQUIRE(id == 0); // IDs should be null. This is so serialization can set it later. Or the user can

        const auto found = engine.get()->try_find_database_object(id);
        CHECK(found.expired());
    }

    TEST_CASE("Object count grows when creating objects")
    {
        scoped_engine engine;

        const std::size_t before = engine.get()->get_database_object_count();

        auto objectResult = engine.get()->create_database_object<sbk::engine::float_parameter>();
        REQUIRE(objectResult.has_value());

        CHECK(engine.get()->get_database_object_count() == before + 1);
    }

    TEST_CASE("Removing an object from the database makes it unfindable")
    {
        scoped_engine engine;

        auto objectResult = engine.get()->create_database_object<sbk::engine::float_parameter>();
        REQUIRE(objectResult.has_value());

        const sbk_id id = objectResult.value()->get_database_id();

        const auto validFound = engine.get()->try_find_database_object(id);
        CHECK_FALSE(validFound.expired());

        engine.get()->remove_object_from_database(id);

        const auto invalidFound = engine.get()->try_find_database_object(id);
        CHECK(invalidFound.expired());
    }
}

TEST_SUITE("Event")
{
    TEST_CASE("Default action is a play action")
    {
        sbk::engine::action action;
        CHECK(action.m_type == sbk::engine::action_type::play);
    }

    TEST_CASE("Event can hold multiple actions")
    {
        scoped_engine engine;

        auto eventResult = engine.get()->create_database_object<sbk::engine::event>();
        REQUIRE(eventResult.has_value());

        auto& event = eventResult.value();

        event->m_actions.push_back(sbk::engine::action{sbk::engine::action_type::play, {}});
        event->m_actions.push_back(sbk::engine::action{sbk::engine::action_type::stop, {}});

        REQUIRE(event->m_actions.size() == 2);
        CHECK(event->m_actions[0].m_type == sbk::engine::action_type::play);
        CHECK(event->m_actions[1].m_type == sbk::engine::action_type::stop);
    }
}

TEST_SUITE("Remote Profiling")
{
    TEST_CASE("Session receives telemetry over loopback")
    {
        scoped_engine engine;

        // Port 0 asks the OS for a free port so the test can never collide.
        REQUIRE(engine.get()->host_remote_session(0).has_value());

        sbk::engine::profiling::remote_session_host* const host = sbk::engine::system::get_remote_session_host();
        REQUIRE(host != nullptr);
        REQUIRE(host->is_open());
        REQUIRE(host->get_port() != 0);

        sbk::engine::profiling::remote_session session;
        REQUIRE(session.connect("127.0.0.1", host->get_port()).has_value());

        // Both ends are polled manually (no threads), so pump both sides until
        // telemetry lands or we give up.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!session.get_latest_telemetry().has_value() && std::chrono::steady_clock::now() < deadline)
        {
            REQUIRE(engine.get()->update().has_value());  //< Publishes telemetry and pumps the host.
            session.update();
        }

        CHECK(session.is_connected());
        CHECK(host->get_connection_count() == 1);
        REQUIRE(session.get_latest_telemetry().has_value());
        CHECK(session.get_telemetry_count() > 0);

        session.disconnect();
        engine.get()->stop_hosting_remote_session();
        CHECK(sbk::engine::system::get_remote_session_host() == nullptr);
    }

    TEST_CASE("Session live-updates a bus volume on the runtime")
    {
        constexpr float defaultVolume = 1.0F;
        constexpr float editedVolume  = 0.25F;

        scoped_engine engine;

        auto bus = engine.get()->create_database_object<sbk::engine::bus>();
        REQUIRE(bus.has_value());
        REQUIRE(bus.value()->m_volume.get() == doctest::Approx(defaultVolume));

        REQUIRE(engine.get()->host_remote_session(0).has_value());
        const uint16_t port = sbk::engine::system::get_remote_session_host()->get_port();

        sbk::engine::profiling::remote_session session;
        REQUIRE(session.connect("127.0.0.1", port).has_value());

        // Sending immediately is legal: commands queue until the connect lands.
        REQUIRE(session
                    .send_set_property(bus.value()->get_database_id(), sbk::core::synced_property_id("Volume"), editedVolume)
                    .has_value());

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (bus.value()->m_volume.get() != doctest::Approx(editedVolume) && std::chrono::steady_clock::now() < deadline)
        {
            session.update();
            REQUIRE(engine.get()->update().has_value());  //< Pumps the host and applies received edits.
        }

        CHECK(bus.value()->m_volume.get() == doctest::Approx(editedVolume));

        session.disconnect();
        engine.get()->stop_hosting_remote_session();
    }

    TEST_CASE("Properties sync automatically, including edits made before connecting")
    {
        using sbk::engine::profiling::set_property_command;

        scoped_engine engine;

        auto busResult = engine.get()->create_database_object<sbk::engine::bus>();
        REQUIRE(busResult.has_value());
        auto bus           = busResult.value();
        const sbk_id busID = bus->get_database_id();

        // Edit made BEFORE connecting - must arrive via the initial sync.
        REQUIRE(bus->m_volume.set(0.6F));

        // Standalone host plays the "game runtime" role so the test can
        // inspect the raw commands it receives.
        sbk::engine::profiling::remote_session_host host;
        REQUIRE(host.open(0).has_value());

        REQUIRE(engine.get()->connect_remote_session("127.0.0.1", host.get_port()).has_value());

        std::vector<set_property_command> received;

        const auto pumpUntilVolumeCommand = [&](const float expectedValue) -> bool
        {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

            while (std::chrono::steady_clock::now() < deadline)
            {
                REQUIRE(engine.get()->update().has_value());  //< Pumps the remote session + broadcaster.
                host.update();

                for (const set_property_command& command : host.consume_property_commands())
                {
                    received.push_back(command);
                }

                for (const set_property_command& command : received)
                {
                    if (command.objectID == busID &&
                        command.property == sbk::core::synced_property_id("Volume") &&
                        command.value == doctest::Approx(expectedValue))
                    {
                        return true;
                    }
                }
            }

            return false;
        };

        // The pre-connect edit arrives without anyone calling send.
        CHECK(pumpUntilVolumeCommand(0.6F));

        // A live edit broadcasts automatically via the property's delegate.
        received.clear();
        REQUIRE(bus->m_volume.set(0.35F));
        CHECK(pumpUntilVolumeCommand(0.35F));

        engine.get()->disconnect_remote_session();
        host.close();
    }

    TEST_CASE("Host rebinds after close")
    {
        scoped_engine engine;

        REQUIRE(engine.get()->host_remote_session(0).has_value());
        const uint16_t firstPort = sbk::engine::system::get_remote_session_host()->get_port();
        CHECK(firstPort != 0);

        // Re-opening must drop the old acceptor and bind cleanly again.
        REQUIRE(engine.get()->host_remote_session(0).has_value());
        CHECK(sbk::engine::system::get_remote_session_host()->is_open());

        engine.get()->stop_hosting_remote_session();
    }
}
