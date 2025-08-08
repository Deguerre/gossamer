// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef SPINLOCK_HH
#define SPINLOCK_HH


class Spinlock
{
private:
    std::atomic<bool> mLatch;

public:
    void lock()
    {
        for (;;) {
            unsigned count = 1;
            if (!mLatch.exchange(true, std::memory_order_acquire)) {
                break;
            }
            while (mLatch.load(std::memory_order_acquire)) {
                if (count > 256) {
                    std::this_thread::yield();
                }
                else {
                    for (unsigned i = 0; i < count; ++i) {
                        Gossamer::cpu_relax();
                    }
                    count *= 2;
                }
            }
        }
    }

    void unlock()
    {
        mLatch.store(false, std::memory_order_release);
    }

    Spinlock()
    {
    }
};

class SpinlockHolder
{
    Spinlock& mLock;
    bool mHeld = false;

public:
    SpinlockHolder(Spinlock& pLock)
        : mLock(pLock)
    {
        mLock.lock();
        mHeld = true;
    }

    void unlock()
    {
        mLock.unlock();
        mHeld = false;
    }

    ~SpinlockHolder()
    {
        if (mHeld) mLock.unlock();
        mHeld = false;
    }
};

#endif // SPINLOCK_HH
