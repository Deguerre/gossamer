// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef WORKQUEUE_HH
#define WORKQUEUE_HH

#include <boost/noncopyable.hpp>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <deque>
#include <boost/intrusive/list.hpp>
#include "ThreadGroup.hh"
#include "Utils.hh"

class WorkQueue : private boost::noncopyable
{
public:
    typedef std::function<void (void)> Item;
    typedef std::deque<Item> Items;

private:
    class Worker;
    friend class Worker;
    typedef std::shared_ptr<Worker> WorkerPtr;

    std::mutex mMutex;
    std::condition_variable mCond;
    Items mItems;
    uint64_t mWaiters;
    bool mFinished;
    bool mJoined;
    std::vector<WorkerPtr> mWorkers;
    ThreadGroup mThreads;

    class Worker
    {
    public:
        void operator()()
        {
            while (true)
            {
                Item itm;
                {
                    std::unique_lock<std::mutex> lock(mQueue.mMutex);
                    while (mQueue.mItems.empty() && !mQueue.mFinished)
                    {
                        //std::cerr << "waiting...." << std::endl;
                        ++mQueue.mWaiters;
                        mQueue.mCond.wait(lock);
                        --mQueue.mWaiters;
                    }
                    if (mQueue.mItems.empty() && mQueue.mFinished)
                    {
                        return;
                    }
                    //std::cerr << "evaluating...." << std::endl;
                    itm = mQueue.mItems.front();
                    mQueue.mItems.pop_front();
                }
                itm();
            }
        }

        Worker(WorkQueue& pQueue)
            : mQueue(pQueue)
        {
        }

    private:
        WorkQueue& mQueue;
    };

public:

    void push_back(const Item& pItem)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mItems.push_back(pItem);
        if (mWaiters > 0)
        {
            mCond.notify_one();
        }
    }

    void wait()
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mFinished = true;
            mCond.notify_all();
        }
        mThreads.join();
        mJoined = true;
    }

    WorkQueue(uint64_t pNumThreads)
        : mWaiters(0), mFinished(false), mJoined(false)
    {
        //std::cerr << "creating WorkQueue with " << pNumThreads << " threads." << std::endl;
        mWorkers.reserve(pNumThreads);
        for (uint64_t i = 0; i < pNumThreads; ++i)
        {
            auto w = std::shared_ptr<Worker>(new Worker(*this));
            mWorkers.push_back(w);
            mThreads.create(*w);
        }
    }

    ~WorkQueue()
    {
        if (!mJoined)
        {
            wait();
        }
    }
};


class ComplexWorkQueue : private boost::noncopyable
{
public:
    typedef std::function<void(void)> WorkFunction;

    struct Task
        : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>,
          private boost::noncopyable
    {
        static constexpr unsigned sMaxSuccessors = 16;
        Task* mSuccessors[sMaxSuccessors];
        WorkFunction mFunction;
        std::atomic<uint32_t> mPredecessors = 1;
        uint8_t mSuccessorCount = 0;
        enum class State { WAIT, READY, RUNNING, COMPLETE } mState = State::WAIT;

        void addSuccessor(Task* task)
        {
            BOOST_ASSERT(mSuccessorCount + 1 < sMaxSuccessors);
            mSuccessors[mSuccessorCount++] = task;
        }

        Task(const WorkFunction& pFunction)
            : mFunction(pFunction)
        {
            std::memset(mSuccessors, 0, sizeof(mSuccessors));
        }
    };

private:
    std::mutex mMutex;
    std::condition_variable mCond;
    std::atomic_bool mFinished;
    bool mJoined;
    std::vector<std::unique_ptr<Task>> mTasks;

    typedef boost::intrusive::list<Task, boost::intrusive::constant_time_size<false>> queue_type;
    queue_type mReadyQueue;
    queue_type mWaitQueue;

    ThreadGroup mThreads;

    Task* waitUntilTaskIsAvailable()
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

    Task* getAvailableTask()
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mReadyQueue.empty()) {
            return 0;
        }
        auto task = &mReadyQueue.front();
        task->unlink();
        return task;
    }

    void workOnTask(Task* task)
    {
        {
            std::unique_lock<std::mutex> lock(mMutex);
            task->unlink();
        }
        task->mState = Task::State::RUNNING;
        task->mFunction();
        finishTask(task);
    }

    void helpWithWork()
    {
        auto task = getAvailableTask();
        if (task) {
            workOnTask(task);
        }
        else {
            std::this_thread::yield();
        }
    }

    void readyTask(Task* task)
    {
        task->mState = Task::State::READY;
        std::unique_lock<std::mutex> lock(mMutex);
        task->unlink();
        mReadyQueue.push_back(*task);
        mCond.notify_one();
    }

    void completeDependency(Task* task)
    {
        if (!--task->mPredecessors) {
            readyTask(task);
        }
    }

    void finishTask(Task* task)
    {
        task->mState = Task::State::COMPLETE;
        for (auto i = 0; i < task->mSuccessorCount; ++i) {
            completeDependency(task->mSuccessors[i]);
        }
    }


public:
    Task* add(const WorkFunction& pFn)
    {
        auto task = std::make_unique<Task>(pFn);
        Task* retval = task.get();
        std::unique_lock<std::mutex> lock(mMutex);
        Gossamer::ensureCapacity(mTasks, 1);
        mTasks.emplace_back(std::move(task));
        mWaitQueue.push_back(*retval);
        return retval;
    }

    void addDependency(Task* pred, Task* succ)
    {
        ++succ->mPredecessors;
        pred->addSuccessor(succ);
    }

    void go(Task* task)
    {
        completeDependency(task);
    }
 
    void wait(Task* task)
    {
        while (task->mState != Task::State::COMPLETE) {
            helpWithWork();
        }
    }

    void end()
    {
        mFinished = true;
        mCond.notify_all();
        mThreads.join();
        mJoined = true;
    }

    ComplexWorkQueue(uint64_t pNumThreads)
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

    ~ComplexWorkQueue()
    {
        if (!mJoined)
        {
            end();
        }
    }
};


#endif // WORKQUEUE_HH
