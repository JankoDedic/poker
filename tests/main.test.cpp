#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

TEST_CASE("testing the test case") {
    REQUIRE_EQ(42, 43);
}
