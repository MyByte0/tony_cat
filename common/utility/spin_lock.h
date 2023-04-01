#ifndef COMMON_UTILITY_SPIN_LOCK_H_
#define COMMON_UTILITY_SPIN_LOCK_H_

#include <atomic>

#include "common/core_define.h"

TONY_CAT_SPACE_BEGIN

class SpinLock {
 public:
    SpinLock() {
        m_atomic_bool.store(false, std::memory_order_relaxed);
        return;
    }

    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;

 public:
    void lock() {
        while (m_atomic_bool.exchange(true, std::memory_order_acquire)) {
            while (true) {
                if (!m_atomic_bool.load(std::memory_order_relaxed)) {
                    break;
                }
            }
        }
        return;
    }

    bool try_lock() {
        return !m_atomic_bool.exchange(true, std::memory_order_acquire);
    }

    void unlock() {
        m_atomic_bool.store(false, std::memory_order_release);
        return;
    }

 private:
    std::atomic<bool> m_atomic_bool;
};

TONY_CAT_SPACE_END

#endif  // COMMON_UTILITY_SPIN_LOCK_H_
