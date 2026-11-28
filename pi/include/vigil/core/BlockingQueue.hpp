#pragma once
// BlockingQueue<T>: the only objects shared between threads.
// Two instances: EventQueue (all threads -> AlarmControllerThread) and
// CommQueue (AlarmControllerThread + Paho callbacks -> CommThread).
//
// Monitor Object:
//  - fixed capacity: a ring buffer allocated once in the constructor, so pushing and
//    popping never allocate (no allocation in the real-time loops);
//  - pthread mutex with PTHREAD_PRIO_INHERIT: the queues are shared by SCHED_FIFO and
//    SCHED_OTHER threads, so a low-priority holder inherits the priority of a waiter;
//  - pthread condition variable on CLOCK_MONOTONIC: popUntil() waits until an absolute
//    deadline that NTP or wall-clock changes cannot move.
//
// Producers never block: tryPush() returns false when the queue is full or closed, and the
// caller counts and logs the loss. The alarm path never waits for a slower consumer.
//
// Errors: every pthread call is checked. A failure throws std::system_error, which the
// WorkerThread error policy turns into a log entry and a watchdog reset.
// Callers outside a WorkerThread (Paho callbacks) must catch it.

#include <pthread.h>
#include <syslog.h>

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <vector>

#include "vigil/core/PosixTime.hpp"
#include "vigil/core/Time.hpp"

namespace vigil {

template <typename T>
class BlockingQueue {
public:
    // capacity must be > 0 (std::invalid_argument). T must be default-constructible and
    // move-assignable (Event and CommCommand are): the buffer is filled once with T{}.
    explicit BlockingQueue(std::size_t capacity);
    ~BlockingQueue();

    // One queue, shared by reference: a copy would have its own mutex and protect nothing.
    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    // Never blocks. False if the queue is full or closed; the item is then not stored and
    // the items already queued are unchanged.
    bool tryPush(const T& item);

    // Never blocks. The oldest item, or nullopt if the queue is empty.
    std::optional<T> tryPop();

    // Waits until an item arrives, the deadline passes or the queue is closed.
    // Returns the oldest item, or nullopt on timeout or when closed and empty.
    // A deadline in the past means "do not wait".
    std::optional<T> popUntil(Time deadline);

    // Clean stop (SIGTERM): wakes every waiter and refuses later pushes. Items already
    // queued can still be popped, so nothing queued before the stop is lost.
    void close();

    // Once true, stays true. Lets a consumer tell "closed" from "timed out".
    bool closed() const;

    // Fixed in the constructor.
    std::size_t capacity() const { return buffer_.size(); }

    // Largest number of items held at the same time since construction. Only grows.
    // Used to check the capacity margin in the real-time measurements on the Pi.
    std::size_t highWater() const;

private:
    // RAII lock: the mutex is released on every exit path, including exceptions.
    class Lock {
    public:
        explicit Lock(pthread_mutex_t& m) : m_(m) {
            check(pthread_mutex_lock(&m_), "BlockingQueue: mutex lock");
        }
        // A destructor must not throw. An unlock failure means the mutex is not ours:
        // the queue state cannot be trusted, so fail loudly and let the watchdog reset.
        ~Lock() {
            if (pthread_mutex_unlock(&m_) != 0) {
                syslog(LOG_CRIT, "BlockingQueue: mutex unlock failed");
                std::abort();
            }
        }
        Lock(const Lock&) = delete;
        Lock& operator=(const Lock&) = delete;

    private:
        pthread_mutex_t& m_;
    };

    T takeLocked();  // precondition: mutex held, count_ > 0

    static void check(int rc, const char* what) {
        if (rc != 0) {
            throw std::system_error(rc, std::generic_category(), what);
        }
    }

    std::vector<T> buffer_;
    std::size_t head_ = 0;   // index of the oldest item
    std::size_t count_ = 0;
    std::size_t highWater_ = 0;
    bool closed_ = false;

    mutable pthread_mutex_t mutex_{};  // mutable: const queries also lock
    pthread_cond_t notEmpty_{};
};

// ---- implementation (template: must be visible where it is used) ----------------------

template <typename T>
BlockingQueue<T>::BlockingQueue(std::size_t capacity) {
    if (capacity == 0) {
        throw std::invalid_argument("BlockingQueue: capacity must be > 0");
    }
    buffer_.resize(capacity);  // the only allocation

    pthread_mutexattr_t ma;
    check(pthread_mutexattr_init(&ma), "BlockingQueue: mutexattr init");
    // ENOTSUP here means the C library has no priority inheritance: refuse to run without it.
    int rc = pthread_mutexattr_setprotocol(&ma, PTHREAD_PRIO_INHERIT);
    if (rc == 0) {
        rc = pthread_mutex_init(&mutex_, &ma);
    }
    pthread_mutexattr_destroy(&ma);
    check(rc, "BlockingQueue: priority-inheritance mutex");

    pthread_condattr_t ca;
    rc = pthread_condattr_init(&ca);
    if (rc == 0) {
        // Without this the condition variable measures deadlines on the wall clock.
        rc = pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
        if (rc == 0) {
            rc = pthread_cond_init(&notEmpty_, &ca);
        }
        pthread_condattr_destroy(&ca);
    }
    if (rc != 0) {
        pthread_mutex_destroy(&mutex_);  // created above; the destructor will not run
        check(rc, "BlockingQueue: monotonic condition variable");
    }
}

template <typename T>
BlockingQueue<T>::~BlockingQueue() {
    pthread_cond_destroy(&notEmpty_);
    pthread_mutex_destroy(&mutex_);
}

template <typename T>
bool BlockingQueue<T>::tryPush(const T& item) {
    Lock lock(mutex_);
    if (closed_ || count_ == buffer_.size()) {
        return false;
    }
    buffer_[(head_ + count_) % buffer_.size()] = item;
    ++count_;
    if (count_ > highWater_) {
        highWater_ = count_;
    }
    // Signal while holding the mutex: the consumer becomes ready at this point, and if this
    // producer is preempted before unlocking, priority inheritance bounds the delay. Signalling
    // after unlocking would let a low-priority producer (CommThread) be preempted with the
    // item queued and the consumer still asleep.
    check(pthread_cond_signal(&notEmpty_), "BlockingQueue: cond signal");
    return true;
}

template <typename T>
std::optional<T> BlockingQueue<T>::tryPop() {
    Lock lock(mutex_);
    if (count_ == 0) {
        return std::nullopt;
    }
    return takeLocked();
}

template <typename T>
std::optional<T> BlockingQueue<T>::popUntil(Time deadline) {
    const timespec ts = toTimespec(deadline);
    Lock lock(mutex_);
    // while, not if: timedwait may return without a signal (spurious wake-up), so the
    // condition is checked again every time. timedwait releases the mutex while asleep and
    // holds it again when it returns, also on timeout.
    while (count_ == 0 && !closed_) {
        const int rc = pthread_cond_timedwait(&notEmpty_, &mutex_, &ts);
        if (rc == ETIMEDOUT) {
            break;
        }
        check(rc, "BlockingQueue: cond timedwait");
    }
    if (count_ == 0) {
        return std::nullopt;  // timed out, or closed and empty
    }
    return takeLocked();  // an item that arrived just before the timeout is still returned
}

template <typename T>
void BlockingQueue<T>::close() {
    Lock lock(mutex_);
    closed_ = true;
    check(pthread_cond_broadcast(&notEmpty_), "BlockingQueue: cond broadcast");
}

template <typename T>
bool BlockingQueue<T>::closed() const {
    Lock lock(mutex_);
    return closed_;
}

template <typename T>
std::size_t BlockingQueue<T>::highWater() const {
    Lock lock(mutex_);
    return highWater_;
}

template <typename T>
T BlockingQueue<T>::takeLocked() {
    T item = std::move(buffer_[head_]);
    head_ = (head_ + 1) % buffer_.size();
    --count_;
    return item;
}

}  // namespace vigil
