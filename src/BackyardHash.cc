// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "BackyardHash.hh"
#include "BlendedSort.hh"

#include <iostream>
#include <unordered_map>

using namespace std;
using namespace boost;


namespace // anonymous
{
    class IdxAccumulator
    {
    public:
        void operator()(uint32_t pIdx)
        {
            mVec.push_back(pIdx);
        }

        IdxAccumulator(vector<uint32_t>& pVec)
            : mVec(pVec)
        {
        }

    private:
        vector<uint32_t>& mVec;
    };

    class Radix64Cmp
    {
    public:
        static const uint32_t zero()
        {
            return 0;
        }

        uint64_t radix(const uint32_t& pIdx) const
        {
            return mHash.radix64(pIdx);
        }

        bool operator()(const uint32_t& pLhs, const uint32_t& pRhs) const
        {
            return mHash.less(pLhs, pRhs);
        }

        Radix64Cmp(const BackyardHash& pHash)
            : mHash(pHash)
        {
        }

    private:
        const BackyardHash& mHash;
    };

    template <int N>
    class RadixCmp
    {
    public:
        static const uint32_t zero()
        {
            return 0;
        }

        uint64_t radix(const uint32_t& pIdx) const
        {
            return mHash.radix<N>(pIdx);
        }

        bool operator()(const uint32_t& pLhs, const uint32_t& pRhs) const
        {
            return mHash.less(pLhs, pRhs);
        }

        RadixCmp(const BackyardHash& pHash)
            : mHash(pHash)
        {
        }

    private:
        const BackyardHash& mHash;
    };

    template <int N>
    void do_sort(uint64_t pNumThreads, vector<uint32_t>& pPerm, const BackyardHash& pHash, uint64_t pN)
    {
        if (pN == N)
        {
            RadixCmp<64 - N> cmp(pHash);
            BlendedSort<uint32_t>::sort(pNumThreads, pPerm, N, cmp);
        }
        else
        {
            do_sort<N - 1>(pNumThreads, pPerm, pHash, pN);
        }
    }

    template <>
    void do_sort<0>(uint64_t pNumThreads, vector<uint32_t>& pPerm, const BackyardHash& pHash, uint64_t pN)
    {
        Radix64Cmp cmp(pHash);
        std::sort(pPerm.begin(), pPerm.end(), cmp);
    }

} // namespace anonymous

void
BackyardHash::insert(const value_type& pItem)
{
    // Figure out if the item is already in the hash table
    // and if so, update the count. Unfortunately, this
    // requires holding the lock while we unhash and check
    // the key.

    PartialHash p = partialHash(pItem);

    if (0)
    {
        vector<uint64_t> ss;
        for (uint64_t j = 0; j < J; ++j)
        {
            Hash h = hash(p, j);
            uint64_t s = h.slot();
            ss.push_back(s);
        }
        std::sort(ss.begin(), ss.end());
        for (uint64_t j = 0; j < J; ++j)
        {
            cerr << ss[j] << '\t';
        }
        cerr << lexical_cast<string>(pItem) << endl;
    }
    //cerr << "pItem  = " << lexical_cast<string>(pItem) << endl;
    //cerr << "p.slot() = " << p.slot() << endl;
    //cerr << "p.hash() = " << p.hash() << endl;

    Hash h[J];
    value_type item[J];
    for (uint64_t j = 0; j < J; ++j)
    {
        h[j] = hash(p, j);
        item[j] = pack(j, 0, h[j].value());
    }

    for (uint64_t j = 0; j < J; ++j)
    {
        uint64_t s0 = h[j].slot();
        uint64_t s = s0;
        while (s < mItems.size())
        {
            Gossamer::cache_prefetch_l1_read(&mItems[s]);
            SpinlockHolder lk(mMutexes[lockNum(s0)]);
            if (value_type::equalWithMask(mItems[s], item[j], mValueNonCountMask) && value_type::testAgainstMask(mItems[s], mValueCountMask))
            {
                Content x = unpack(mItems[s]);
                BOOST_ASSERT(x.value() == h[j].value());
                uint64_t c = x.count() + 1;
                if (c <= mCountMask)
                {
                    mItems[s] = pack(j, c, h[j].value());
                    return;
                }
                else
                {
                    mItems[s] = value_type(0);
                    lk.unlock();
                    SpinlockHolder lk(mBackyardMutex);
                    if (mBackyard.find(pItem) == mBackyard.end())
                    {
                        ++mSpills;
                    }
                    mBackyard[pItem] += c;
                    return;
                }
            }
            s += (1ULL << mSlotBits);
        }
    }

    // Figure out if the item is already in the spill table.
    // Actually, since we're already allowing duplicates and
    // the probability of anything being in the spill table
    // is very very small, let's avoid the search (urk - a div
    // instruction!) and the locking.
    if (0)
    {
        SpinlockHolder lk(mBackyardMutex);
        if (auto i = mBackyard.find(pItem); i != mBackyard.end())
        {
            i->second++;
            return;
        }
    }

    // Okay, now we're really cuckoo hash insert.
    //cerr << "begin cuckoo insert" << endl;
    ++mSize;
    value_type k = pItem;
    uint64_t c = 1;
    uint64_t j = (++mRandom) % J;
    for (uint64_t i = 0; i < S; ++i)
    {
        Hash h = hash(p, j);
        uint64_t s0 = h.slot();
        uint64_t s = s0;
        while (s < mItems.size())
        {
            Gossamer::cache_prefetch_l1_write(&mItems[s]);
            //cerr << s << '\t' << lexical_cast<string>(k) << endl;
            value_type vOld;
            value_type vNew = pack(j, c, h.value());
            {
                SpinlockHolder lk(mMutexes[lockNum(s0)]);
                vOld = mItems[s];
                mItems[s] = vNew;
            }
            if (!value_type::testAgainstMask(vOld, mValueCountMask))
            {
                return;
            }
            Content x = unpack(vOld);
            s += (1ULL << mSlotBits);
            j = x.hash();
            c = x.count();
            h = Hash(s0, x.value());
        }
        k = unhash(s0, j, h.value());
        j = (j + 1) % J;
        p = partialHash(k);
        // The call to partialHash redoes the hash0 done in unhash on the same value.
        // Ideally, we would cache this value.
    }
    //cerr << "cuckoo insert failed" << endl;

    // Too hard. Let's just drop the item into the spill table.
    SpinlockHolder lk(mBackyardMutex);
    if (mBackyard.find(k) == mBackyard.end())
    {
        ++mPanics;
    }
    mBackyard[k] += c;
    BOOST_ASSERT(mPanics + mSpills == mBackyard.size());
}

void
BackyardHash::sort(vector<uint32_t>& pPerm, uint64_t pNumThreads) const
{
    if (size() >= (1ULL << 32))
    {
        BOOST_THROW_EXCEPTION(
            Gossamer::error()
                << Gossamer::general_error_info("backyard hash has too many items for a 32 bit permutation vector."));
    }
    index();
    //cerr << "constructing permutation vector" << endl;
    uint32_t z = size();
    pPerm.clear();
    pPerm.reserve(z);
    IdxAccumulator acc(pPerm);
    visit0(acc);
    uint64_t radixBits = mItemBits - mSlotBits;
    //cerr << "radix sorting" << endl;
    if (radixBits >= 64)
    {
        Radix64Cmp cmp(*this);
        BlendedSort<uint32_t>::sort(pNumThreads, pPerm, 64, cmp);
    }
    else
    {
        do_sort<63>(pNumThreads, pPerm, *this, radixBits);
    }
}

BackyardHash::BackyardHash(uint64_t pSlotBits, uint64_t pItemBits, uint64_t pNumSlots)
    : mSlotBits(pSlotBits), mSlotMask((1ULL << pSlotBits) - 1), mItemBits(pItemBits),
      mHashNumBits(2), mHashNumMask((1ULL << mHashNumBits) - 1),
      mCountBits(8 * sizeof(value_type) - pItemBits + min(pItemBits, mSlotBits) - mHashNumBits),
      mCountMask((1ULL << min(static_cast<uint64_t>(63), mCountBits)) - 1),
      mMutexes(uint64_t(1) << L), mItems(pNumSlots), mRandom(0), mSize(0), mSpills(0), mPanics(0)
{
    mValueCountMask = value_type(mCountMask);
    mValueCountMask <<= mHashNumBits;
    mValueNonCountMask = ~mValueCountMask;
}

