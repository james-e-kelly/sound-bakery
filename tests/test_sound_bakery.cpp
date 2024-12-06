#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "sound_bakery/system.h"

TEST_SUITE("System")
{
    TEST_CASE("System Creation Deletion")
    {
        sb_system_config config = sb_system_config_init_default();

        REQUIRE(sbk::engine::system::create() != nullptr);
        REQUIRE(sbk::engine::system::init(config) == MA_SUCCESS);
        REQUIRE(sbk::engine::system::update() == MA_SUCCESS);
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }

    TEST_CASE("Re-init")
    {
        sb_system_config config = sb_system_config_init_default();

        REQUIRE(sbk::engine::system::create() != nullptr);
        REQUIRE(sbk::engine::system::init(config) == MA_SUCCESS);
        REQUIRE(sbk::engine::system::update() == MA_SUCCESS);
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);

        REQUIRE(sbk::engine::system::create() != nullptr);
        REQUIRE(sbk::engine::system::init(config) == MA_SUCCESS);
        REQUIRE(sbk::engine::system::update() == MA_SUCCESS);
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }
}