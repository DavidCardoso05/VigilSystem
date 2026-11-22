#include <gtest/gtest.h>

#include "vigil/net/ReconnectBackoff.hpp"

using namespace vigil;
using std::chrono::seconds;

TEST(ReconnectBackoff, DoublesUpTo60AndNeverStops) {
    ReconnectBackoff b;
    const int expected[] = {1, 2, 4, 8, 16, 32, 60, 60, 60};
    for (int e : expected) EXPECT_EQ(b.next(-1), seconds(e));
    for (int i = 0; i < 100; ++i) EXPECT_EQ(b.next(0x88), seconds(60));   // server unavailable
}

TEST(ReconnectBackoff, ResetAfterConnack) {
    ReconnectBackoff b;
    b.next(-1);
    b.next(-1);
    b.reset();
    EXPECT_EQ(b.next(-1), seconds(1));
}

TEST(ReconnectBackoff, AuthErrorsEvery5Minutes) {
    ReconnectBackoff b;
    EXPECT_EQ(b.next(0x86), seconds(300));
    EXPECT_EQ(b.next(0x87), seconds(300));
    EXPECT_TRUE(ReconnectBackoff::isAuthError(0x86));
    EXPECT_FALSE(ReconnectBackoff::isAuthError(0x9F));
}
