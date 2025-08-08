// Copyright (c) 2015 Andrew J. Bromage
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef FIBERMANAGER_HH
#define FIBERMANAGER_HH

#ifndef UTILS_HH
#include "Utils.hh"
#endif

#ifndef DEQUE_HH
#include "Deque.hh"
#endif

#ifndef BOOST_FIBER_ALL_HPP
#include <boost/fiber/all.hpp>
#define BOOST_FIBER_ALL_HPP
#endif

class FiberManager : private boost::noncopyable
{
public:
	FiberManager(unsigned num_threads);
	~FiberManager();

	typedef std::function<void()> job_fn_t;
	typedef uint64_t job_id_t;

	job_id_t add_job(job_fn_t job, int prio = 100);

	void join(job_id_t id);

	unsigned num_threads() const;

private:
	struct Impl;
	std::unique_ptr<Impl> mPImpl;
};

struct FiberControl : private boost::noncopyable {
	FiberManager& mMgr;
	const unsigned mMaxTasks;
	boost::fibers::mutex mMutex;
	Queue<FiberManager::job_id_t> mActiveJobs;

	FiberControl(FiberManager& pMgr)
		: mMgr(pMgr),
		  mMaxTasks(pMgr.num_threads() * 2 + 4)
	{
	}

	void reserve_jobs() {
		std::unique_lock<boost::fibers::mutex> lck(mMutex);
		while (mActiveJobs.size() >= mMaxTasks) {
			mMgr.join(mActiveJobs.front());
			mActiveJobs.pop_front();
		}
	}

	void add_job(FiberManager::job_fn_t job, bool check = true, int prio = 100) {
		std::unique_lock<boost::fibers::mutex> lck(mMutex);
		if (check) {
			while (mActiveJobs.size() >= mMaxTasks) {
				mMgr.join(mActiveJobs.front());
				mActiveJobs.pop_front();
			}
		}
		mActiveJobs.push_back(mMgr.add_job(job, prio));
	}

	void wait_for_jobs() {
		std::unique_lock<boost::fibers::mutex> lck(mMutex);
		while (!mActiveJobs.empty()) {
			mMgr.join(mActiveJobs.front());
			mActiveJobs.pop_front();
		}
	}

	~FiberControl()
	{
		wait_for_jobs();
	}
};


template<typename T>
class FiberQueue
{
private:
	bool mDone = false;
	Queue<T> mItems;
	boost::fibers::mutex mMutex;
	boost::fibers::condition_variable mCondVar;

public:
	FiberQueue() = default;
	~FiberQueue() = default;

	bool done() const {
		return mDone;
	}

	void set_done() {
		std::scoped_lock<boost::fibers::mutex> lck(mMutex);
		mDone = true;
		mCondVar.notify_all();
	}

	void enqueue(const T& object)
	{
		std::scoped_lock<boost::fibers::mutex> lck(mMutex);
		mItems.push_back(object);
		mCondVar.notify_one();
	}

	void enqueue(T&& object)
	{
		std::scoped_lock<boost::fibers::mutex> lck(mMutex);
		mItems.emplace_back(object);
		mCondVar.notify_one();
	}

	bool dequeue(T& object)
	{
		std::scoped_lock<boost::fibers::mutex> lck(mMutex);
		while (!mItems.empty() && !mDone)
		{
			mCondVar.wait(lck);
		}
		if (!mItems.empty()) {
			object = mItems.front();
			mItems.pop_front();
			return true;
		}
		else {
			return false;
		}
	}
};

#endif