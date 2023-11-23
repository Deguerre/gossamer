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

// On Intel ISA (i.e. everything so far), we try to use the pause
// instruction in spin loops. This gives a hint to the CPU that it
// shouldn't saturate the bus with cache acquire requests until some
// other CPU issues a write.
//
// Pause is technically only supported in SSE2 or higher. This is
// supported on every x86_64 CPU released to date, but in theory may
// not be guaranteed. Thankfully, it's backwards compatible with IA32,
// since it's the same as rep ; nop.


class Spinlock
{
private:
    std::atomic<uint64_t> mLatch = 0;

public:
    void lock()
    {
        for (;;) {
            if (!mLatch.exchange(true, std::memory_order_acquire)) {
                break;
            }
            while (mLatch.load(std::memory_order_relaxed)) {
                Gossamer::cpu_relax();
            }
        }
    }

    void unlock()
    {
        mLatch.store(0, std::memory_order_release);
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
