#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "sound_bakery/system.h"

TEST_SUITE("System")
{
    TEST_CASE("System Creation Deletion")
    {
        REQUIRE(sbk::engine::system::create() != nullptr);
        REQUIRE(sbk::engine::system::init() == MA_SUCCESS);
        REQUIRE(sbk::engine::system::update() == MA_SUCCESS);
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }

    TEST_CASE("Re-init")
    {
        REQUIRE(sbk::engine::system::create() != nullptr);
        REQUIRE(sbk::engine::system::init() == MA_SUCCESS);
        REQUIRE(sbk::engine::system::update() == MA_SUCCESS);
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);

        REQUIRE(sbk::engine::system::create() != nullptr);
        REQUIRE(sbk::engine::system::init() == MA_SUCCESS);
        REQUIRE(sbk::engine::system::update() == MA_SUCCESS);
        sbk::engine::system::destroy();
        REQUIRE(sbk::engine::system::get() == nullptr);
    }
}