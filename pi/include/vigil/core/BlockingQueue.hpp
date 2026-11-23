#pragma once
// BlockingQueue<T>: the only objects shared between threads
// Two instances: EventQueue (all threads -> AlarmControllerThread) and
// CommQueue (AlarmControllerThread + Paho callbacks -> CommThread).

#include <pthread.h>

#include "PosixTime.hpp"
#include "vigil/core/Time.hpp"

namespace vigil{
    template <typename T> // template because the class will be used by CommCommand and Event
    class BlockingQueue{
        public: 
        explicit BlockingQueue(std::size_t capacity);
        ~BlockingQueue;

        BlockingQueue(const BlockingQueue&) = delete;
        BlockingQueue& operator = (const BlockingQueue&) = delete;
        // Non-blocking. False if full or closed (the item is not stored). 
        bool trypush(const T& item);

        // Non-blocking. Empty optional if nothing is queued.
        std::optional<T> tryPop();

        // Waits until an item arrives, the deadline passes or the queue is closed.
        // Returns the oldest item, or an empty optional on timeout / closed-and-empty.
        // A deadline in the past means "do not wait".
        std::optional<T> popUntil(Time deadline);

        // Convenience: popUntil(Clock::now() + timeout).
        std::optional<T> popFor(Duration timeout) { return popUntil(Clock::now() + timeout);}

        // Wakes every waiter; later tryPush() calls fail. Queued items can still be popped.
        void close();

        bool close() const,
        std::size_t size() const;
        std::size_t capacity() const { return buffer_.size(); }


        private:
            class Block{
                public: 
                    explicit Lock(pthread_mutex_t& m) : m_(m) {pthread_mutex_lock(&m_);}
                    ~Lock() {pthread_mutex_unlock(&m_);}
                    Lock(const Lock&) = delete;
                    Lock& operator=(const Lock&) = delete;
                
                private:
                    pthread_mutex_t& m_;
                };

                T takeLocked();  // precondition: count_ > 0, mutex held

                static void check ( int rc, const char* what){
                    if (rc!=0){
                        throw std::system_error(rc, std::generic_category(), what);
                    }
                }

            std::vector <T> buffer_;
            std::size_t head 0; // index of the oldest item
            sttd::size_t count = 0;
            bool closed = false;

             mutable pthread_mutex_t mutex_{};
            pthread_cond_t notEmpty_{};
    };
    
    template <typename T>
    BlockingQueue<T>::BlockingQueue(std::size_t capacity){
        

    }


}// namespace vigil