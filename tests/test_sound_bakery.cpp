#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "sound_bakery/system.h"

#include "sound_bakery/core/containers/message_queue.h"
#include "sound_bakery/core/containers/ring_buffer.h"
#include "sound_bakery/core/property.h"
#include "sound_bakery/core/memory/memory.h"
#include "sound_bakery/core/task/command_queue.h"
#include "sound_bakery/core/thread_domain.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/bus/aux_bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/container/random_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/runtime/runtime.h"
#include "sound_bakery/sound/sound.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

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
        scoped_engine(bool singleThreaded = true)
        {
            sbk_system_config config = sbk_system_config_init_default();
            config.logToConsole      = true;
            config.singleThreadedUpdate = singleThreaded;

            REQUIRE(sbk::engine::system::create().has_value());
            REQUIRE(sbk::engine::system::get() != nullptr);
            REQUIRE(sbk::engine::system::get()->init(config).has_value());
        }

        ~scoped_engine() { sbk::engine::system::destroy(); }

        [[nodiscard]] auto get() const -> sbk::engine::system* { return sbk::engine::system::get(); }
    };

    /**
     * @brief For when running tests without the engine but need memory (rpmalloc) to be initialized.
     */
    struct scoped_memory
    {
        scoped_memory()
        {
            sbk::memory::init();
        }

        ~scoped_memory()
        {
            sbk::memory::shutdown();
        }
    };
}  // namespace

TEST_SUITE("System")
{
    TEST_CASE("System Creation Deletion")
    {
        sbk_system_config config = sbk_system_config_init_default();
        config.logToConsole      = true;

        REQUIRE(sbk::engine::system::create().has_value());
        REQUIRE(sbk::engine::system::get()->init(config).has_value());
        REQUIRE(sbk::engine::system::get()->update().has_value());
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }

    TEST_CASE("Re-init")
    {
        sbk_system_config config = sbk_system_config_init_default();
        config.logToConsole      = true;

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

    TEST_CASE("Runtime exists after creation")
    {
        scoped_engine engine;
        REQUIRE(engine.get()->get_runtime() != nullptr);
    }

    TEST_CASE("Listener game object exists after init")
    {
        scoped_engine engine;

        // The listener is created during init. The master bus, by contrast, is
        // only established once a project/soundbank is loaded, so it stays null
        // after a bare init.
        CHECK(engine.get()->get_runtime()->get_listener_game_object() != nullptr);
        CHECK(engine.get()->get_runtime()->get_master_bus() == nullptr);
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
        CHECK(engine.get()->get_objects_of_category(sbk::memory::object_category::runtime_object).size() > 0);
    }

    TEST_CASE("Creating objects increases object count correctly")
    {
        scoped_engine engine;

        const std::size_t countBefore = engine.get()->get_objects_count();

        auto bus = engine.get()->get_runtime()->create_database_object<sbk::engine::bus>();
        CHECK(bus.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 1);

        auto bus2 = engine.get()->get_runtime()->create_database_object<sbk::engine::bus>();
        CHECK(bus2.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 2);

        auto auxBus = engine.get()->get_runtime()->create_database_object<sbk::engine::aux_bus>();
        CHECK(auxBus.has_value());
        CHECK(engine.get()->get_objects_count() == countBefore + 3);
        
        auto auxBus2 = engine.get()->get_runtime()->create_database_object<sbk::engine::aux_bus>();
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

TEST_SUITE("Object")
{
    TEST_CASE("Objects can find the system through their owners")
    {
        scoped_engine engine;

        auto createdObject = engine.get()->get_runtime()->create_runtime_object<sbk::core::object>();
        REQUIRE(createdObject.has_value());

        CHECK(createdObject.value()->get_system() == sbk::engine::system::get());
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

        auto parameterResult = engine.get()->get_runtime()->create_database_object<sbk::engine::named_parameter>();
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

        auto parameterResult = engine.get()->get_runtime()->create_database_object<sbk::engine::named_parameter>();
        REQUIRE(parameterResult.has_value());

        auto& parameter = parameterResult.value();

        // get_values() on an empty parameter should populate a default entry.
        const auto values = parameter->get_values();
        CHECK_FALSE(values.empty());
    }

    TEST_CASE("Empty name is ignored")
    {
        scoped_engine engine;

        auto parameterResult = engine.get()->get_runtime()->create_database_object<sbk::engine::named_parameter>();
        REQUIRE(parameterResult.has_value());

        auto& parameter = parameterResult.value();

        const auto invalid = parameter->add_new_value("");
        CHECK_FALSE(invalid.valid());
    }
}

TEST_SUITE("Node Containers")
{
    TEST_CASE("Can create nested containers")
    {
        scoped_engine engine;

        auto parent = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();
        REQUIRE(parent.has_value());

        auto child = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();
        REQUIRE(child.has_value());

        parent.value()->add_child(sbk::core::database_ptr<sbk::engine::node_base>(child.value()));
        auto returnedParent = child.value()->get_parent();
        REQUIRE(returnedParent);
        CHECK(returnedParent->get_database_id() == parent.value()->get_database_id());

        REQUIRE(parent.value()->get_child_count() > 0);
        REQUIRE(parent.value()->get_children()[0]->get_database_id() == child.value()->get_database_id());
    }

    TEST_CASE("Cannot assign to self")
    {
        scoped_engine engine;

        auto parent = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();

        parent.value()->add_child(sbk::core::database_ptr<sbk::engine::node_base>(parent.value()));
        REQUIRE(parent.value()->get_child_count() == 0);
    }

    TEST_CASE("Deep hierarchy tears down cleanly via remove_all")
    {
        scoped_engine engine;

        using node_ptr = std::shared_ptr<sbk::engine::node>;

        const auto make_node = [&]() -> node_ptr
        {
            auto created = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();
            REQUIRE(created.has_value());
            return created.value();
        };

        node_ptr root = make_node();

        std::vector<node_ptr> currentLevel{root};

        constexpr int depth        = 5;
        constexpr int childrenEach = 4;

        for (int level = 0; level < depth; ++level)
        {
            std::vector<node_ptr> nextLevel;
            for (const node_ptr& parent : currentLevel)
            {
                for (int i = 0; i < childrenEach; ++i)
                {
                    node_ptr child = make_node();
                    parent->add_child(sbk::core::database_ptr<sbk::engine::node_base>(child));
                    nextLevel.push_back(child);
                }
            }
            currentLevel = std::move(nextLevel);
        }

        // A leaf carries a multi-segment path name built by walking its parents, and it
        // resolves by ID while alive. We capture identity by value and deliberately keep no
        // shared_ptr to it, so remove_all() is free to destroy it.
        const sbk::core::database_name leafName = currentLevel.front()->get_database_name();
        const sbk_id leafID                     = currentLevel.front()->get_database_id();

        CHECK(std::string_view(static_cast<const char*>(leafName)).find('/') != std::string_view::npos);
        CHECK_FALSE(engine.get()->try_find_database_object(leafID).expired());

        // Release our own references so the system owns the only strong refs, then tear everything down. 
        currentLevel.clear();
        root.reset();

        engine.get()->get_runtime()->remove_all();

        CHECK(engine.get()->try_find_database_object(leafID).expired());
    }

    TEST_CASE("Reparenting updates the derived name with no bookkeeping")
    {
        scoped_engine engine;

        auto parentA = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();
        auto parentB = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();
        auto child   = engine.get()->get_runtime()->create_database_object<sbk::engine::node>();
        REQUIRE(parentA.has_value());
        REQUIRE(parentB.has_value());
        REQUIRE(child.has_value());

        const auto childPtr = sbk::core::database_ptr<sbk::engine::node_base>(child.value());

        parentA.value()->add_child(childPtr);
        const sbk::core::database_name underA = child.value()->get_database_name();

        parentB.value()->add_child(childPtr);
        const sbk::core::database_name underB = child.value()->get_database_name();

        CHECK_FALSE(underA == underB);

        CHECK_FALSE(engine.get()->try_find_database_object(underB).expired());
        CHECK(engine.get()->try_find_database_object(underA).expired());
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

        auto objectResult = engine.get()->get_runtime()->create_database_object<sbk::engine::float_parameter>();
        REQUIRE(objectResult.has_value());

        const sbk_id id = objectResult.value()->get_database_id();
        REQUIRE(id != 0);

        const auto found = engine.get()->try_find_database_object(id);
        CHECK_FALSE(found.expired());
    }

    TEST_CASE("Objects not added to the database cannot be found")
    {
        scoped_engine engine;

        auto objectResult = engine.get()->get_runtime()->create_database_object<sbk::engine::float_parameter>(false);
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

        auto objectResult = engine.get()->get_runtime()->create_database_object<sbk::engine::float_parameter>();
        REQUIRE(objectResult.has_value());

        CHECK(engine.get()->get_database_object_count() == before + 1);
    }

    TEST_CASE("Removing an object from the database makes it unfindable")
    {
        scoped_engine engine;

        auto objectResult = engine.get()->get_runtime()->create_database_object<sbk::engine::float_parameter>();
        REQUIRE(objectResult.has_value());

        const sbk_id id = objectResult.value()->get_database_id();

        const auto validFound = engine.get()->try_find_database_object(id);
        CHECK_FALSE(validFound.expired());

        REQUIRE(engine.get()->remove_object_from_database(id).has_value());

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

        auto eventResult = engine.get()->get_runtime()->create_database_object<sbk::engine::event>();
        REQUIRE(eventResult.has_value());

        auto& event = eventResult.value();

        event->m_actions.push_back(sbk::engine::action{sbk::engine::action_type::play, {}});
        event->m_actions.push_back(sbk::engine::action{sbk::engine::action_type::stop, {}});

        REQUIRE(event->m_actions.size() == 2);
        CHECK(event->m_actions[0].m_type == sbk::engine::action_type::play);
        CHECK(event->m_actions[1].m_type == sbk::engine::action_type::stop);
    }
}

// ---------------------------------------------------------------------------
// Placeholder tests for APIs that don't exist yet.
//
// These are intentionally skipped (`* doctest::skip()`) so the suite stays
// green while still documenting the behaviour we expect once the underlying
// API is implemented. Remove the skip decorator and flesh out the body as
// each feature lands.
// ---------------------------------------------------------------------------
TEST_SUITE("Future API")
{
    TEST_CASE("game_object::set_parameter unified setter" * doctest::skip())
    {
        // TODO: game_object currently exposes separate set_float_parameter and
        // set_int_parameter_value. A single templated/overloaded
        // set_parameter(parameter, value) would be nicer to use.
        //
        //   game_object gameObject;
        //   gameObject.set_parameter(floatParameter, 0.5F);
        //   CHECK(gameObject.get_float_parameter_value(floatParameter) == 0.5F);
    }

    TEST_CASE("game_object::reset_parameters clears local overrides" * doctest::skip())
    {
        // TODO: no way to clear local parameter overrides so a game object
        // falls back to global values again.
    }

    TEST_CASE("voice::set_parameter applies to a playing voice" * doctest::skip())
    {
        // TODO: voice has no public set_parameter. Setting a parameter on a
        // live voice should update the sounds it is currently playing.
    }

    TEST_CASE("voice::stop halts playback" * doctest::skip())
    {
        // TODO: voice exposes play_container and is_playing but no explicit
        // stop(). Needed to test that a played voice can be stopped on demand.
    }

    TEST_CASE("named_parameter::remove_value deletes a discrete value" * doctest::skip())
    {
        // TODO: named_parameter can add_new_value but there is no remove_value.
        // Removing the currently-selected value should reset the selection.
    }

    TEST_CASE("event executes its actions on a game object" * doctest::skip())
    {
        // TODO: event only stores a list of actions; there is no post/execute
        // entry point that runs them against a game object.
        //
        //   event->post(gameObject);
        //   CHECK(gameObject.is_playing());
    }

    TEST_CASE("container gather_children_for_play selects sounds" * doctest::skip())
    {
        // TODO: exercise gather_children_for_play on the concrete container
        // types (random/sequence/blend/switch) once fixtures exist to build a
        // small node graph for a test.
    }
}

TEST_SUITE("Thread Domain")
{
    TEST_CASE("Scopes mark the game and studio domains")
    {
        scoped_engine engine(false);

        // Outside any pump, the calling thread belongs to no domain.
        CHECK(sbk::core::get_current_thread_domain() == sbk::core::thread_domain::unknown);

        // Tasks drained by update() run inside the game domain.
        std::atomic<sbk::core::thread_domain> observedGame{sbk::core::thread_domain::unknown};
        engine.get()->get_game_executer()->post_work( [&observedGame] { observedGame = sbk::core::get_current_thread_domain(); });

        REQUIRE(engine.get()->update().has_value());
        CHECK(observedGame.load() == sbk::core::thread_domain::game);

        // Tasks posted to the studio executor drain on the studio timer
        // inside update_async, which marks the studio domain.
        std::atomic<sbk::core::thread_domain> observedStudio{sbk::core::thread_domain::unknown};
        std::atomic<bool> studioTaskRan{false};

        engine.get()->get_system_executer()->post_work(
            [&observedStudio, &studioTaskRan]
            {
                observedStudio = sbk::core::get_current_thread_domain();
                studioTaskRan  = true;
            });

        (void)engine.get()->update();

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!studioTaskRan.load() && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::yield();
        }

        REQUIRE(studioTaskRan.load());
        CHECK(observedStudio.load() == sbk::core::thread_domain::studio);
    }

    TEST_CASE("Nested scopes restore the previous domain")
    {
        CHECK(sbk::core::get_current_thread_domain() == sbk::core::thread_domain::unknown);

        {
            const sbk::core::scoped_thread_domain gameScope(sbk::core::thread_domain::game);
            CHECK(sbk::core::get_current_thread_domain() == sbk::core::thread_domain::game);

            {
                const sbk::core::scoped_thread_domain studioScope(sbk::core::thread_domain::studio);
                CHECK(sbk::core::get_current_thread_domain() == sbk::core::thread_domain::studio);
            }

            CHECK(sbk::core::get_current_thread_domain() == sbk::core::thread_domain::game);
        }

        CHECK(sbk::core::get_current_thread_domain() == sbk::core::thread_domain::unknown);
    }
}

TEST_SUITE("Stress Tests")
{
    TEST_CASE("Create huge database")
    {
        scoped_engine engine;

        constexpr std::size_t numberOfObjectsToCreate = 100000;
        const std::initializer_list<rttr::type> objectTypesToCreate =
            {
                sbk::engine::sound::type(),
                sbk::engine::sound_container::type(),
                sbk::engine::random_container::type(),
                sbk::engine::float_parameter::type()
            };

        for (const rttr::type& type : objectTypesToCreate)
        {
            for (std::size_t index = 0; index < numberOfObjectsToCreate; ++index)
            {
                auto creationResult = engine.get()->get_runtime()->create_database_object(type);
                REQUIRE(creationResult.has_value());
            }
        }

        engine.get()->get_runtime()->remove_all();
    }
}

TEST_SUITE("Tasks")
{
    static auto test_detatched_task(std::reference_wrapper<std::atomic<bool>> ranStudio, std::reference_wrapper<std::atomic<sbk::core::thread_domain>> observed) -> sbk::detached_task
    {
        co_await sbk::engine::system::get()->get_system_executer()->schedule();
        observed.get() = sbk::core::get_current_thread_domain();
        ranStudio.get() = true;
        co_return sbk::ok();
    }

    TEST_CASE("Can spawn detatched task and have it move to different threads")
    {
        scoped_engine engine(false);

        std::atomic<sbk::core::thread_domain> observedSystem{sbk::core::thread_domain::unknown};
        std::atomic<bool> studioTaskRan{false};
        
        test_detatched_task(studioTaskRan, observedSystem);

        REQUIRE(engine.get()->update().has_value());

        (void)engine.get()->update();

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!studioTaskRan.load() && std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::yield();
        }

        REQUIRE(studioTaskRan.load());
        CHECK(observedSystem.load() == sbk::core::thread_domain::studio);
    }
}

TEST_SUITE("Ring Buffer")
{
    TEST_CASE("Init")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(ringBuffer.init(512, rpmalloc).has_value());
    }

    TEST_CASE("Cannot double init")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(ringBuffer.init(512, rpmalloc).has_value());
        REQUIRE(ringBuffer.init(256, rpmalloc).has_value() == false);
    }

    TEST_CASE("Round up to power of two")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(ringBuffer.init(500, rpmalloc).has_value());
        CHECK(ringBuffer.get_capacity() == 512);    // Should round up to nearest power of two
    }

    TEST_CASE("Writing bytes allows reading of that many bytes")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(ringBuffer.init(4, rpmalloc).has_value());

        std::int32_t writeValue = 7;

        REQUIRE(ringBuffer.write(&writeValue, sizeof(std::int32_t)) == SBK_SUCCESS);

        std::int32_t readValue = 0;

        REQUIRE(ringBuffer.read(&readValue, sizeof(std::int32_t)) == SBK_SUCCESS);
        REQUIRE(writeValue == readValue);
    }

    TEST_CASE("Cannot write over bytes")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(ringBuffer.init(4, rpmalloc).has_value());

        std::int32_t firstWriteValue = 7;
        std::int32_t secondWriteValue = 14;

        REQUIRE(ringBuffer.write(&firstWriteValue, sizeof(std::int32_t)) == SBK_SUCCESS);
        REQUIRE(ringBuffer.write(&secondWriteValue, sizeof(std::int32_t)) != SBK_SUCCESS);

        std::int32_t readValue = 0;

        REQUIRE(ringBuffer.read(&readValue, sizeof(std::int32_t)) == SBK_SUCCESS);
        REQUIRE(firstWriteValue == readValue);
    }

    TEST_CASE("Cannot read over bytes")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(ringBuffer.init(4, rpmalloc).has_value());

        std::int32_t firstWriteValue  = 7;

        REQUIRE(ringBuffer.write(&firstWriteValue, sizeof(std::int32_t)) == SBK_SUCCESS);

        std::int32_t readValue = 0;

        REQUIRE(ringBuffer.read(&readValue, sizeof(std::int32_t)) == SBK_SUCCESS);
        REQUIRE(firstWriteValue == readValue);

        REQUIRE(ringBuffer.read(&readValue, sizeof(std::int32_t)) != SBK_SUCCESS);
    }

    TEST_CASE("Sequential read and writes are corrext")
    {
        scoped_memory memory;

        sbk::mpsc_ring_buffer ringBuffer;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        constexpr std::size_t numOfIntsToWrite = 1024 * 1024;

        REQUIRE(ringBuffer.init(numOfIntsToWrite * sizeof(std::size_t), rpmalloc).has_value());

        std::jthread writeThread([&]() 
            {
                for (std::size_t index = 0U; index < numOfIntsToWrite; ++index)
                {
                    REQUIRE(ringBuffer.write(&index, sizeof(std::size_t)) == SBK_SUCCESS);
                }
            });
        std::jthread readThread([&]() 
            {
                std::size_t iterations = 0;
                std::size_t readValue = 0;

                while (iterations < numOfIntsToWrite)
                {
                    if (ringBuffer.read(&readValue, sizeof(std::size_t)) == SBK_SUCCESS)
                    {
                        REQUIRE(readValue == iterations++);
                    }
                }
            });

        writeThread.join();
        readThread.join();
    }

    TEST_CASE("Reader should see all values from producers")
    {
        for (std::size_t iteration = 0; iteration < 16; ++iteration)
        {
            scoped_memory memory;

            sbk::mpsc_ring_buffer ringBuffer;
            sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

            REQUIRE(ringBuffer.init(512 * sizeof(std::size_t), rpmalloc).has_value());

            std::unordered_set<std::size_t> uniqueThreadValues = {1, 7, 777, 999, 1024, 50202502};
            std::vector<std::jthread> writerThreads;

            constexpr std::size_t writesPerThread = 1024;

            for (std::size_t writerValue : uniqueThreadValues)
            {
                std::jthread writerThread([&, threadValue = writerValue]()
                                          {
                    std::size_t valueToWrite = threadValue;
                    for (std::size_t index = 0; index < writesPerThread; )
                    {
                        if (ringBuffer.write(&valueToWrite, sizeof(std::size_t)) == SBK_SUCCESS)
                        {
                            ++index; // Only move on once we've full written everything
                        }
                    } });
                writerThreads.push_back(std::move(writerThread));
            }

            std::jthread readThread([&]()
                                    { 
                std::unordered_map<std::size_t, std::size_t> threadValueToTimesRead;

                std::size_t readValue{};

                std::size_t numberOfReads{};

                while (numberOfReads < writesPerThread * uniqueThreadValues.size())
                {
                    if (ringBuffer.read(&readValue, sizeof(std::size_t)) == SBK_SUCCESS)
                    {
                        ++threadValueToTimesRead[readValue];
                        ++numberOfReads;
                    }
                }

                for (std::size_t uniqueValue : uniqueThreadValues)
                {
                    CHECK(threadValueToTimesRead[uniqueValue] == writesPerThread);
                } });

            for (auto& thread : writerThreads)
            {
                thread.join();
            }
            readThread.join();
        }
    }
}

TEST_SUITE("Message Queue")
{
    TEST_CASE("Init")
    {
        scoped_memory memory;

        sbk::message_queue messageQueue;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(messageQueue.init(512, rpmalloc).has_value());
    }

    TEST_CASE("Read and write messages")
    {
        scoped_memory memory;

        enum class message_type : std::uint8_t
        {
            start,
            update,
            end
        };

        struct start_message
        {
            int a;
            int b;
        };

        struct update_message
        {
            std::size_t iter;
        };

        struct end_message
        {
            int c;
        };

        sbk::message_queue<message_type> messageQueue;
        sbk::memory::rpmalloc_resource rpmalloc(sbk::memory::object_category::system);

        REQUIRE(messageQueue.init(messageQueue.get_header_size() + sizeof(update_message), rpmalloc).has_value());

        constexpr std::size_t updateLoops = 512;

        std::jthread writeThread([&]()
            {
                sbk_status result;
                std::size_t iteration{};

                do
                {
                    result = messageQueue.write_message(message_type::start, start_message{.a = 7, .b = 9});
                } while (result != SBK_SUCCESS);

                do
                {
                    result = messageQueue.write_message(message_type::update, update_message{.iter = iteration});
                    if (result == SBK_SUCCESS)
                    {
                        ++iteration;
                    }
                } while (iteration < updateLoops);

                do
                {
                    result = messageQueue.write_message(message_type::end, end_message{.c = 11});
                } while (result != SBK_SUCCESS);
            });
        std::jthread readThread([&]()
            {
                bool end = false;
                std::size_t updates{};

                for (;;)
                {
                    sbk::message_queue<message_type>::message_view view{};

                    if (messageQueue.read_begin(&view) == SBK_SUCCESS)
                    {
                        message_type messageType = view.m_type;
                        const bool correctMessageType = messageType == message_type::start || messageType == message_type::update || messageType == message_type::end;
                        REQUIRE(correctMessageType);

                        switch (messageType)
                        {
                            case message_type::start:
                            {
                                const start_message* startMessage = reinterpret_cast<const start_message*>(view.payload);
                                REQUIRE(startMessage->a == 7);
                                REQUIRE(startMessage->b == 9);
                                std::cout << "Start" << std::endl;
                            }
                            break;
                            case message_type::update:
                            {
                                const update_message* updateMessage = reinterpret_cast<const update_message*>(view.payload);
                                REQUIRE(updateMessage->iter == updates++);
                                std::cout << "Update " << updateMessage->iter << std::endl;
                            }
                            break;
                            case message_type::end:
                            {
                                const end_message* endMessage = reinterpret_cast<const end_message*>(view.payload);
                                end                     = true;
                                std::cout << "End"  << std::endl;
                            }
                            break;
                        }

                        REQUIRE(messageQueue.read_end(view) == SBK_SUCCESS);
                    }

                    if (end)
                    {
                        REQUIRE(updates == updateLoops);
                        return;
                    }
                }
            });

        writeThread.join();
        readThread.join();
    }
}