// L1 tests for BlockingQueue<T> and PosixTime.
//
// Not testable here, by design (see the class header):
//  - that the mutex really uses PTHREAD_PRIO_INHERIT and the condition variable
//    CLOCK_MONOTONIC: POSIX cannot query them after creation; the constructor fails if
//    either attribute is rejected;
//  - pthread failures while running: they only happen after a bug (undefined behaviour).
// ThreadSanitizer runs these same tests in a separate build.
#include <gtest/gtest.h>

#include <time.h>

#include <chrono>
#include <optional>
#include <stdexcept>
#include <thread>
#include <variant>
#include <vector>

#include "vigil/core/BlockingQueue.hpp"
#include "vigil/core/CommCommand.hpp"
#include "vigil/core/Event.hpp"
#include "vigil/core/PosixTime.hpp"

using namespace vigil;
using namespace std::chrono_literals;

namespace {

// Test shortcut for a relative wait. The queue itself only offers absolute deadlines.
template <typename T>
std::optional<T> popFor(BlockingQueue<T>& q, Duration timeout) {
    return q.popUntil(Clock::now() + timeout);
}

long msBetween(Time a, Time b) {
    return static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(b - a).count());
}

}  // namespace

// ---- single thread ------------------------------------------------------------------

TEST(BlockingQueue, ZeroCapacityThrows) {
    EXPECT_THROW(BlockingQueue<int>{0}, std::invalid_argument);
}

TEST(BlockingQueue, CapacityIsTheConstructorValue) {
    BlockingQueue<int> q{5};
    EXPECT_EQ(q.capacity(), 5u);
}

TEST(BlockingQueue, FifoOrder) {
    BlockingQueue<int> q{3};
    ASSERT_TRUE(q.tryPush(1));
    ASSERT_TRUE(q.tryPush(2));
    ASSERT_TRUE(q.tryPush(3));
    EXPECT_EQ(q.tryPop(), 1);
    EXPECT_EQ(q.tryPop(), 2);
    EXPECT_EQ(q.tryPop(), 3);
}

TEST(BlockingQueue, FullQueueRefusesAndKeepsContents) {
    BlockingQueue<int> q{3};
    q.tryPush(1);
    q.tryPush(2);
    q.tryPush(3);
    EXPECT_FALSE(q.tryPush(4));  // refused: the new item is the one lost
    EXPECT_EQ(q.tryPop(), 1);    // the older items are all still there, in order
    EXPECT_EQ(q.tryPop(), 2);
    EXPECT_EQ(q.tryPop(), 3);
    EXPECT_FALSE(q.tryPop().has_value());
}

TEST(BlockingQueue, EmptyTryPopReturnsNothing) {
    BlockingQueue<int> q{2};
    EXPECT_FALSE(q.tryPop().has_value());
}

TEST(BlockingQueue, RingBufferWrapsAround) {
    BlockingQueue<int> q{2};
    for (int i = 0; i < 10; ++i) {
        ASSERT_TRUE(q.tryPush(i));
        ASSERT_EQ(q.tryPop(), i);
    }
    EXPECT_FALSE(q.tryPop().has_value());
}

TEST(BlockingQueue, CarriesEventsAndCommCommands) {
    BlockingQueue<Event> events{4};
    const Time t = Clock::now();
    ASSERT_TRUE(events.tryPush(Event{EventType::EVT_MOTION_DETECTED, EventSource::IR2, t}));
    const auto ev = events.tryPop();
    ASSERT_TRUE(ev.has_value());
    EXPECT_EQ(ev->type, EventType::EVT_MOTION_DETECTED);
    EXPECT_EQ(ev->source, EventSource::IR2);
    EXPECT_EQ(ev->timestamp, t);

    BlockingQueue<CommCommand> comm{4};
    ASSERT_TRUE(comm.tryPush(RecordEvent{EventLogType::ARMED, EventLogDetail::REMOTE}));
    const auto cmd = comm.tryPop();
    ASSERT_TRUE(cmd.has_value());
    ASSERT_TRUE(std::holds_alternative<RecordEvent>(*cmd));
    EXPECT_EQ(std::get<RecordEvent>(*cmd).type, EventLogType::ARMED);
    EXPECT_EQ(std::get<RecordEvent>(*cmd).detail, EventLogDetail::REMOTE);
}

TEST(BlockingQueue, HighWaterRecordsPeakAndNeverDrops) {
    BlockingQueue<int> q{4};
    EXPECT_EQ(q.highWater(), 0u);
    q.tryPush(1);
    q.tryPush(2);
    q.tryPush(3);
    EXPECT_EQ(q.highWater(), 3u);
    q.tryPop();
    q.tryPop();
    EXPECT_EQ(q.highWater(), 3u);  // popping does not lower it
    q.tryPush(4);                  // 2 items now: below the peak
    EXPECT_EQ(q.highWater(), 3u);
    q.tryPush(5);
    q.tryPush(6);                  // 4 items: new peak
    EXPECT_EQ(q.highWater(), 4u);
    EXPECT_FALSE(q.tryPush(7));    // a refused push does not count
    EXPECT_EQ(q.highWater(), 4u);
}

// ---- time ---------------------------------------------------------------------------

TEST(BlockingQueue, PopUntilTimesOutNotBeforeTheDeadline) {
    BlockingQueue<int> q{1};
    const Time start = Clock::now();
    const Time deadline = start + 50ms;
    EXPECT_FALSE(q.popUntil(deadline).has_value());
    const Time end = Clock::now();
    EXPECT_GE(end, deadline);                   // strict: never early
    EXPECT_LT(msBetween(start, end), 50 + 40);  // loose: margin for a loaded laptop
}

TEST(BlockingQueue, PastDeadlineDoesNotWait) {
    BlockingQueue<int> q{1};
    const Time start = Clock::now();
    EXPECT_FALSE(q.popUntil(start - 1s).has_value());
    EXPECT_LT(msBetween(start, Clock::now()), 10);
}

TEST(BlockingQueue, PastDeadlineStillReturnsQueuedItem) {
    BlockingQueue<int> q{1};
    ASSERT_TRUE(q.tryPush(7));
    EXPECT_EQ(q.popUntil(Clock::now() - 1s), 7);
}

TEST(PosixTime, SteadyClockIsClockMonotonic) {
    timespec mono{};
    clock_gettime(CLOCK_MONOTONIC, &mono);
    const timespec ours = toTimespec(Clock::now());
    const long long diffNs =
        (static_cast<long long>(ours.tv_sec) - mono.tv_sec) * 1'000'000'000LL + (ours.tv_nsec - mono.tv_nsec);
    EXPECT_GE(diffNs, 0);
    EXPECT_LT(diffNs, 50'000'000LL);  // same clock: read a few microseconds apart
}

TEST(PosixTime, SplitsSecondsAndNanoseconds) {
    const timespec ts = toTimespec(Time{std::chrono::nanoseconds{3'250'000'001}});
    EXPECT_EQ(ts.tv_sec, 3);
    EXPECT_EQ(ts.tv_nsec, 250'000'001);

    const timespec zero = toTimespec(Time{});
    EXPECT_EQ(zero.tv_sec, 0);
    EXPECT_EQ(zero.tv_nsec, 0);

    const timespec negative = toTimespec(Time{} - 5s);  // "already in the past"
    EXPECT_EQ(negative.tv_sec, 0);
    EXPECT_EQ(negative.tv_nsec, 0);
}

// ---- close --------------------------------------------------------------------------

TEST(BlockingQueue, CloseWakesWaitingConsumer) {
    BlockingQueue<int> q{1};
    std::thread closer([&q] {
        std::this_thread::sleep_for(30ms);
        q.close();
    });
    const Time start = Clock::now();
    EXPECT_FALSE(popFor(q, 5s).has_value());
    EXPECT_LT(msBetween(start, Clock::now()), 500);  // woken by close(), not by the 5 s timeout
    closer.join();
}

TEST(BlockingQueue, CloseRefusesPushes) {
    BlockingQueue<int> q{4};
    q.close();
    EXPECT_FALSE(q.tryPush(1));
    EXPECT_FALSE(q.tryPop().has_value());
}

TEST(BlockingQueue, ItemsQueuedBeforeCloseStillComeOut) {
    BlockingQueue<int> q{4};
    q.tryPush(1);
    q.tryPush(2);
    q.close();
    EXPECT_EQ(popFor(q, 1s), 1);
    EXPECT_EQ(q.tryPop(), 2);
}

TEST(BlockingQueue, ClosedAndEmptyReturnsImmediately) {
    BlockingQueue<int> q{1};
    q.close();
    const Time start = Clock::now();
    EXPECT_FALSE(popFor(q, 5s).has_value());
    EXPECT_LT(msBetween(start, Clock::now()), 10);  // a consumer must treat this as "stop"
}

TEST(BlockingQueue, ClosedStaysTrue) {
    BlockingQueue<int> q{1};
    EXPECT_FALSE(q.closed());
    q.close();
    EXPECT_TRUE(q.closed());
    q.tryPush(1);
    q.tryPop();
    q.close();  // closing twice is harmless
    EXPECT_TRUE(q.closed());
}

// ---- several threads ----------------------------------------------------------------

TEST(BlockingQueue, PushWakesWaitingConsumer) {
    BlockingQueue<int> q{1};
    Time pushedAt{};
    std::thread producer([&] {
        std::this_thread::sleep_for(30ms);
        pushedAt = Clock::now();
        q.tryPush(42);
    });
    const auto v = popFor(q, 5s);
    const Time gotAt = Clock::now();
    producer.join();  // pushedAt is read only after the join
    ASSERT_EQ(v, 42);
    EXPECT_LT(msBetween(pushedAt, gotAt), 20);  // woken by the push, not by the timeout
}

TEST(BlockingQueue, ManyProducersLoseNothingAndKeepOrder) {
    constexpr int kProducers = 4;
    constexpr int kPerProducer = 5000;
    BlockingQueue<int> q{16};  // small on purpose: producers often find it full

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&q, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                while (!q.tryPush(p * kPerProducer + i)) {
                    std::this_thread::yield();  // the test retries; product code counts a drop
                }
            }
        });
    }

    std::vector<int> lastSeen(kProducers, -1);
    bool ordered = true;
    for (int received = 0; received < kProducers * kPerProducer; ++received) {
        const auto v = popFor(q, 2s);
        ASSERT_TRUE(v.has_value()) << "consumer starved after " << received << " items";
        const int p = *v / kPerProducer;
        const int i = *v % kPerProducer;
        ordered = ordered && (i == lastSeen[p] + 1);  // each producer's items in its own order
        lastSeen[p] = i;
    }
    for (auto& t : producers) {
        t.join();
    }
    EXPECT_TRUE(ordered);
    EXPECT_FALSE(q.tryPop().has_value());
    EXPECT_EQ(q.highWater(), 16u);  // it really was full at some point
}
