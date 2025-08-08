// Copyright (c) 2015 Andrew J. Bromage
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "FiberManager.hh"

using namespace boost::fibers;

namespace {
	class FiberManagerProperties : public fiber_properties {
	public:
		FiberManagerProperties(boost::fibers::context* ctx)
			: fiber_properties(ctx), prio(100)
		{
		}

		int get_priority() const {
			return prio;
		}

		void set_priority(int p) {
			if (p != prio) {
				prio = p;
				notify();
			}
		}

	private:
		int prio;
	};

	struct FiberManagerScheduler
		: public algo::algorithm_with_properties<FiberManagerProperties>,
		private boost::noncopyable
	{
		typedef scheduler::ready_queue_type rqueue_type;
		typedef scheduler::ready_queue_type lqueue_type;

		static rqueue_type     	rqueue_;
		static std::mutex   	rqueue_mtx_;

		lqueue_type            	lqueue_{};
		std::mutex              mtx_{};
		std::condition_variable cnd_{};
		bool                    flag_{ false };
		bool                    suspend_{ true };

		FiberManagerScheduler() = default;

		FiberManagerScheduler(bool suspend) :
			suspend_{ suspend } {
		}

		void awakened(context* ctx, FiberManagerProperties& props) noexcept override;

		context* pick_next() noexcept override;

		bool has_ready_fibers() const noexcept override {
			std::unique_lock< std::mutex > lock{ rqueue_mtx_ };
			return !rqueue_.empty() || !lqueue_.empty();
		}

		void suspend_until(std::chrono::steady_clock::time_point const& time_point) noexcept override;

		void notify() noexcept override;

		void property_change(context* ctx, FiberManagerProperties& props) noexcept override;
	};

	FiberManagerScheduler::rqueue_type FiberManagerScheduler::rqueue_{};
	std::mutex FiberManagerScheduler::rqueue_mtx_{};

	void FiberManagerScheduler::awakened(context* ctx, FiberManagerProperties& props) noexcept
	{
		int prio = props.get_priority();

		if (ctx->is_context(type::pinned_context)) {
			lqueue_type::iterator it(std::find_if(lqueue_.begin(), lqueue_.end(),
				[prio, this](context& c) {
					return properties(&c).get_priority() < prio;
				}));
			lqueue_.insert(it, *ctx);
		}
		else {
			ctx->detach();
			std::unique_lock< std::mutex > lk{ rqueue_mtx_ };
			rqueue_type::iterator it(std::find_if(rqueue_.begin(), rqueue_.end(),
				[prio, this](context& c) {
					return properties(&c).get_priority() <= prio;
				}));
			rqueue_.insert(it, *ctx);
		}
	}

	context* FiberManagerScheduler::pick_next() noexcept
	{
		context* ctx = nullptr;
		std::unique_lock< std::mutex > lk{ rqueue_mtx_ };
		if (!rqueue_.empty()) {
			ctx = &rqueue_.front();
			rqueue_.pop_front();
			lk.unlock();
			BOOST_ASSERT(nullptr != ctx);
			context::active()->attach(ctx);
		}
		else {
			lk.unlock();
			if (!lqueue_.empty()) {
				ctx = &lqueue_.front();
				lqueue_.pop_front();
			}
		}
		return ctx;
	}


	void FiberManagerScheduler::suspend_until(std::chrono::steady_clock::time_point const& time_point) noexcept {
		if (suspend_) {
			if ((std::chrono::steady_clock::time_point::max)() == time_point) {
				std::unique_lock< std::mutex > lk{ mtx_ };
				cnd_.wait(lk, [this]() { return flag_; });
				flag_ = false;
			}
			else {
				std::unique_lock< std::mutex > lk{ mtx_ };
				cnd_.wait_until(lk, time_point, [this]() { return flag_; });
				flag_ = false;
			}
		}
	}

	void FiberManagerScheduler::property_change(context* ctx, FiberManagerProperties& props) noexcept
	{
		if (!ctx->ready_is_linked()) {
			return;
		}

		ctx->ready_unlink();
		awakened(ctx, props);
	}


	void FiberManagerScheduler::notify() noexcept {
		if (suspend_) {
			std::unique_lock< std::mutex > lk{ mtx_ };
			flag_ = true;
			lk.unlock();
			cnd_.notify_all();
		}
	}
}

struct FiberManager::Impl {
	boost::fibers::mutex mtx;
	boost::fibers::condition_variable_any condvar;
	unsigned mNumThreads = 0;
	bool draining = false;

	std::mutex dsmtx;
	std::vector<std::thread> mThreads;
	std::map<FiberManager::job_id_t, fiber> mFibers;

	job_id_t mId = 0;

	FiberManager::job_id_t add_job(job_fn_t job, int prio)
	{
		fiber f(job);
		FiberManagerProperties& props(f.properties<FiberManagerProperties>());
		props.set_priority(prio);
		job_id_t id = 0;

		{
			std::unique_lock<std::mutex> lck(dsmtx);
			id = ++mId;
			mFibers.emplace(id, std::move(f));
		}

		condvar.notify_one();

		return id;
	}

	Impl(unsigned pNumThreads);

	void join(FiberManager::job_id_t id)
	{
		boost::fibers::fiber f;
		{
			std::unique_lock<std::mutex> lck(dsmtx);
			auto it = mFibers.find(id);
			f = std::move(it->second);
			mFibers.erase(it);
		}
		f.join();
	}

	void join();

	~Impl()
	{
		join();
	}
};



FiberManager::Impl::Impl(unsigned pNumThreads)
	: mNumThreads(pNumThreads)
{
	mThreads.reserve(mNumThreads);

	for (unsigned i = 0; i < mNumThreads; ++i) {
		mThreads.emplace_back([&]() {
			boost::fibers::use_scheduling_algorithm< FiberManagerScheduler >();

			mtx.lock();
			while (!draining) {
				condvar.wait(mtx);
			}
			mtx.unlock();
		});
	}

	boost::fibers::use_scheduling_algorithm< FiberManagerScheduler >();
}


void
FiberManager::Impl::join()
{
	mtx.lock();
	draining = true;
	condvar.notify_all();
	mtx.unlock();

	for (auto& thr : mThreads) {
		thr.join();
	}
}


FiberManager::FiberManager(unsigned pNumThreads)
	: mPImpl(std::make_unique<FiberManager::Impl>(pNumThreads))
{
}


FiberManager::~FiberManager()
{
}


FiberManager::job_id_t
FiberManager::add_job(job_fn_t job, int prio)
{
	return mPImpl->add_job(job, prio);
}


void
FiberManager::join(FiberManager::job_id_t id)
{
	mPImpl->join(id);
}

unsigned
FiberManager::num_threads() const
{
	return mPImpl->mNumThreads;
}

