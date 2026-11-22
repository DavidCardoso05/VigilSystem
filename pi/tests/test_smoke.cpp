// Checks only that GoogleTest and ctest are wired. Removed when the first
// real unit tests (core/BlockingQueue) arrive.
#include <gtest/gtest.h>

TEST(Smoke, GoogleTestIsWired) {
    EXPECT_EQ(1 + 1, 2);
}
