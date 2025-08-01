#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../external/doctest/doctest.h"

int add(int a, int b) { return a + b; }

int subtract(int a, int b) { return a - b; }

TEST_CASE("Addition works correctly") {
  CHECK(add(2, 3) == 5);
  CHECK(add(-1, -1) == -2);
  CHECK(add(0, 0) == 0);
}

TEST_CASE("Subtraction works correctly") {
  CHECK(subtract(5, 3) == 2);
  CHECK(subtract(0, 0) == 0);
  CHECK(subtract(-3, -2) == -1);
}
