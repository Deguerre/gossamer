// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "EdgeAndCount.hh"
#include "Graph.hh"
#include "JobManager.hh"
#include "RankSelect.hh"
#include "IntegerCodecs.hh"

#include <iostream>
#include <stdio.h>

namespace
{
    template<typename Item>
    class Elem
    {
    public:
        virtual std::uint64_t edgesRead() const = 0;

        virtual std::string label() const = 0;

        virtual const JobManager::Tokens& deps() const = 0;

        virtual const std::vector<Item>& items() const = 0;

        virtual void moveToFront(const typename std::vector<Item>::const_iterator& pFrom) = 0;

        virtual void fill() = 0;

        virtual ~Elem() {}
    };

    template<typename Item>
    class Loader : public Elem<Item>
    {
    public:
        std::uint64_t edgesRead() const override
        {
            return mEdgesRead;
        }

        std::string label() const override
        {
            return mPart.mFname;
        }

        const JobManager::Tokens& deps() const override
        {
            return mDeps;
        }

        const std::vector<Item>& items() const override
        {
            return mItems;
        }

        void moveToFront(const typename std::vector<Item>::const_iterator& pFrom) override
        {
            if (pFrom == mItems.begin())
            {
                return;
            }
            //cerr << "loader: moving " << (mItems.end() - pFrom) << " items." << endl;
            auto i = mItems.begin();
            for (auto j = pFrom; j != mItems.end();)
            {
                *i++ = *j++;
            }
            mItems.erase(i, mItems.end());
            //cerr << "mItems.size() = " << mItems.size() << endl;
        }

        void fill() override
        {
            std::uint64_t z = mItems.size();
            // std::cerr << "mItem = " << lexical_cast<std::string>(Gossamer::EdgeItemTraits<Item>::edge(mItem)) << endl;

            if (mItems.size() >= mBufferSize || mEof) {
                return;
            }

            do
            {
                if (!mDecoder.decode(mItem)) {
                    mEof = true;
                    return;
                }

                BOOST_ASSERT(mItems.empty() || mItems.back() < mItem);
                mItems.push_back(mItem);

                ++mEdgesRead;
            }
            while (mItems.size() < mBufferSize);

            // cerr << "job " << mPart.mNumber << " read " << (mItems.size() - z) << endl;
        }

        Loader(const AsyncMerge::Part& pPart, uint64_t pBufferSize, FileFactory& pFactory)
            : mFactory(pFactory), mPart(pPart), mBufferSize(pBufferSize),
              mEdgesRead(0), mItem(), mInP(mFactory.in(mPart.mFname)), mDecoder(**mInP)
        {
            mItems.reserve(pBufferSize);
            Gossamer::EdgeItemTraits<Item>::edge(mItem) = ~Gossamer::position_type(0);
        }

        ~Loader()
        {
            // cerr << mPart.mNumber << '\t' << mEdgesRead << '\t' << mPart.mSize << endl;
            BOOST_ASSERT(mEdgesRead == mPart.mSize);
        }

    private:
        FileFactory& mFactory;
        const AsyncMerge::Part mPart;
        const JobManager::Tokens mDeps;
        std::uint64_t mBufferSize;
        std::uint64_t mEdgesRead;
        Item mItem;
        std::vector<Item> mItems;
        FileFactory::InHolderPtr mInP;
        EdgeDecoder<Item> mDecoder;
        bool mEof = false;
    };

    template<typename Item>
    class Merger : public Elem<Item>
    {
    public:
        std::uint64_t edgesRead() const override
        {
            return mLhs->edgesRead() + mRhs->edgesRead();
        }

        std::string label() const override
        {
            return "merger";
        }

        const JobManager::Tokens& deps() const override
        {
            return mDeps;
        }

        const std::vector<Item>& items() const override
        {
            return mItems;
        }

        void moveToFront(const typename std::vector<Item>::const_iterator& pFrom) override
        {
            if (pFrom == mItems.begin())
            {
                return;
            }
            //cerr << "merger: moving " << (mItems.end() - pFrom) << " items." << endl;
            auto i = mItems.begin();
            for (auto j = pFrom; j != mItems.end();)
            {
                *i++ = *j++;
            }
            mItems.erase(i, mItems.end());
        }

        void fill() override
        {
            if (mItems.size())
            {
                return;
            }

            const auto& lhs = mLhs->items();
            const auto& rhs = mRhs->items();
            //cerr << (void*)this << '\t' << "in" << '\t' << lhs.size() << '\t' << rhs.size() << '\t'
            //        << mItems.size() << endl;
            mItems.reserve(mItems.size() + lhs.size() + rhs.size());
            auto l = lhs.begin();
            //cerr << "lhs.size() = " << lhs.size() << endl;
            auto r = rhs.begin();
            //cerr << "rhs.size() = " << rhs.size() << endl;
            while (l != lhs.end() && r != rhs.end())
            {
                auto& lkey = Gossamer::EdgeItemTraits<Item>::edge(*l);
                auto& rkey = Gossamer::EdgeItemTraits<Item>::edge(*r);
                if (lkey < rkey)
                {
                    BOOST_ASSERT(mItems.empty() || mItems.back() < *l);
                    mItems.push_back(*l);
                    ++l;
                    continue;
                }
                if (lkey > rkey)
                {
                    BOOST_ASSERT(mItems.empty() || mItems.back() < *r);
                    mItems.push_back(*r);
                    ++r;
                    continue;
                }
                BOOST_ASSERT(mItems.empty() || mItems.back() < *l);
                mItems.push_back(*l);
                Gossamer::EdgeItemTraits<Item>::combine(mItems.back(), *r);
                ++l;
                ++r;
            }
            if (lhs.empty())
            {
                while (r != rhs.end())
                {
                    mItems.push_back(*r);
                    ++r;
                }
            }
            if (rhs.empty())
            {
                while (l != lhs.end())
                {
                    mItems.push_back(*l);
                    ++l;
                }
            }

            //cerr << (void*)this << '\t' << "out" << '\t' << (lhs.end() - l) << '\t' << (rhs.end() - r) << '\t'
            //        << mItems.size() << endl;
            mDeps.clear();
            if (lhs.size() > 0)
            {
                //cerr << "merger: merged " << (l - lhs.begin()) << " of " << lhs.size() << " lhs items." << endl;
                mLhs->moveToFront(l);
                //cerr << "previously got " << lhs.size() << " items, scheduling " << mLhs->label() << endl;
                JobManager::Token lt = mMgr.enqueue(std::bind(&Elem::fill, mLhs.get()), mLhs->deps());
                mDeps.insert(lt);
            }
            if (rhs.size() > 0)
            {
                //cerr << "merger: merged " << (r - rhs.begin()) << " of " << rhs.size() << " rhs items." << endl;
                mRhs->moveToFront(r);
                //cerr << "previously got " << rhs.size() << " items, scheduling " << mRhs->label() << endl;
                JobManager::Token rt = mMgr.enqueue(std::bind(&Elem::fill, mRhs.get()), mRhs->deps());
                mDeps.insert(rt);
            }
        }

        Merger(JobManager& pMgr, uint64_t pBufferSize, const std::shared_ptr<Elem<Item>>& pLhs, const std::shared_ptr<Elem<Item>>& pRhs)
            : mMgr(pMgr), mBufferSize(pBufferSize), mLhs(pLhs), mRhs(pRhs)
        {
            JobManager::Token lt = mMgr.enqueue(std::bind(&Elem::fill, mLhs.get()), mLhs->deps());
            mDeps.insert(lt);
            JobManager::Token rt = mMgr.enqueue(std::bind(&Elem::fill, mRhs.get()), mRhs->deps());
            mDeps.insert(rt);
        }

    private:
        JobManager& mMgr;
        JobManager::Tokens mDeps;
        uint64_t mBufferSize;
        std::shared_ptr<Elem<Item>> mLhs;
        std::shared_ptr<Elem<Item>> mRhs;
        std::vector<Item> mItems;
    };

    template<typename Item>
    std::shared_ptr<Elem<Item>> build(const std::vector<AsyncMerge::Part>& pParts,
                  FileFactory& pFactory, std::uint64_t pBufferSize, JobManager& pMgr)
    {
        BOOST_ASSERT(pParts.size() > 0);
        std::deque<std::shared_ptr<Elem<Item>>> ptrs;
        for (auto& part : pParts) {
            ptrs.push_back(std::shared_ptr<Elem<Item>>(new Loader<Item>(part, pBufferSize, pFactory)));
        }
        while (ptrs.size() > 1)
        {
            std::shared_ptr<Elem<Item>> l = ptrs.front();
            ptrs.pop_front();
            std::shared_ptr<Elem<Item>> r = ptrs.front();
            ptrs.pop_front();
            ptrs.push_back(std::shared_ptr<Elem<Item>>(new Merger<Item>(pMgr, pBufferSize, l, r)));
            //cerr << "build: " << (void*)ptrs.back().get() << '\t' << (void*)l.get() << '\t' << (void*)r.get() << endl;
        }
        return ptrs.front();
    }

    template <typename T,typename Item>
    void do_merge(JobManager& pMgr, std::shared_ptr<Elem<Item>>& pRoot, const std::string& pBaseName, std::uint64_t pK, std::uint64_t pN, FileFactory& pFactory)
    {
        typename T::Builder bld(pK, pBaseName, pFactory, pN);
        while (true)
        {
            JobManager::Token t = pMgr.enqueue(std::bind(&Elem<Item>::fill, pRoot.get()), pRoot->deps());
            pMgr.wait(t);
            if (pRoot->items().empty())
            {
                break;
            }
            const auto& itms(pRoot->items());
            for (auto i = itms.begin(); i != itms.end(); ++i)
            {
                //cerr << lexical_cast<string>(i->first) << '\t' << i->second << endl;
                bld.push_back(*i);
            }
            pRoot->moveToFront(itms.end());
        }
        bld.end();
    }
}
// namespace anonymous

template <typename T, typename Item>
void
AsyncMerge::merge(const std::vector<AsyncMerge::Part>& pParts, const std::string& pGraphName,
                  std::uint64_t pK, std::uint64_t pN, std::uint64_t pNumThreads, std::uint64_t pBufferSize, FileFactory& pFactory)
{
    JobManager mgr(pNumThreads);
    std::shared_ptr<Elem<Item>> r = build<Item>(pParts, pFactory, pBufferSize, mgr);
    do_merge<T>(mgr, r, pGraphName, pK, pN, pFactory);
    mgr.wait();
}
