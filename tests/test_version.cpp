#include <catch2/catch_test_macros.hpp>

#include "core/version.h"

TEST_CASE("version string is set") {
    REQUIRE(chess::version() == "0.1.0");
}
