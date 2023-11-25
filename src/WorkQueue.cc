// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "WorkQueue.hh"

ComplexWorkQueue::Task*
ComplexWorkQueue::waitUntilTaskIsAvailable()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mReadyQueue.empty()) {
        mCond.wait(lock, [&] { return !mReadyQueue.empty() || mFinished;  });
    }
    if (mReadyQueue.empty()) {
        return 0;
    }
    auto task = &mReadyQueue.front();
    mReadyQueue.pop_front();
    return task;
}

ComplexWorkQueue::Task*
ComplexWorkQueue::getAvailableTask()
{
    std::unique_lock<std::mutex> lock(mMutex);
    if (mReadyQueue.empty()) {
        return 0;
    }
    auto task = &mReadyQueue.front();
    task->unlink();
    return task;
}

void
ComplexWorkQueue::workOnTask(ComplexWorkQueue::Task* task)
{
    {
        std::unique_lock<std::mutex> lock(mMutex);
        task->unlink();
    }
    task->mState = Task::State::RUNNING;
    task->mFunction();
    finishTask(task);
}

void
ComplexWorkQueue::helpWithWork()
{
    auto task = getAvailableTask();
    if (task) {
        workOnTask(task);
    }
    else {
        std::this_thread::yield();
    }
}

void
ComplexWorkQueue::readyTask(ComplexWorkQueue::Task* task)
{
    task->mState = Task::State::READY;
    std::unique_lock<std::mutex> lock(mMutex);
    task->unlink();
    mReadyQueue.push_back(*task);
    mCond.notify_one();
}

void
ComplexWorkQueue::completeDependency(ComplexWorkQueue::Task* task)
{
    if (!--task->mPredecessors) {
        readyTask(task);
    }
}

void
ComplexWorkQueue::finishTask(ComplexWorkQueue::Task* task)
{
    task->mState = Task::State::COMPLETE;
    for (auto s : task->mSuccessors) {
        completeDependency(s);
    }
}


ComplexWorkQueue::ComplexWorkQueue(uint64_t pNumThreads)
    : mFinished(false), mJoined(false)
{
    //std::cerr << "creating WorkQueue with " << pNumThreads << " threads." << std::endl;
    for (uint64_t i = 0; i < pNumThreads; ++i)
    {
        mThreads.create([&]() {
            while (!mFinished) {
                Task* task = waitUntilTaskIsAvailable();
                if (!task) {
                    continue;
                }
                workOnTask(task);
            }
        });
    }
}


ComplexWorkQueue::~ComplexWorkQueue()
{
    if (!mJoined)
    {
        end();
    }
}
