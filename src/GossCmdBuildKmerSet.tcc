// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#ifndef ASYNCMERGE_HH
#include "AsyncMerge.hh"
#endif

#ifndef BACKGROUNDMULTICONSUMER_HH
#include "BackgroundMultiConsumer.hh"
#endif

#ifndef BACKYARDHASH_HH
#include "BackyardHash.hh"
#endif

#ifndef EDGEANDCOUNT_HH
#include "EdgeAndCount.hh"
#endif

#ifndef KMERSET_HH
#include "KmerSet.hh"
#endif

#ifndef TIMER_HH
#include "Timer.hh"
#endif

#ifndef STD_STRING
#include <string>
#endif

#ifndef STD_UTILITY
#include <utility>
#define STD_UTILITY
#endif

#ifndef STD_VECTOR
#include <vector>
#define STD_VECTOR
#endif

#ifndef BOOST_LEXICAL_CAST_HPP
#include <boost/lexical_cast.hpp>
#define BOOST_LEXICAL_CAST_HPP
#endif

namespace {

    typedef std::vector<Gossamer::edge_type> KmerBlock;
    typedef std::shared_ptr<KmerBlock> KmerBlockPtr;
    static constexpr uint64_t blkSz = 4096;
    static constexpr uint32_t sPermutationPrefetch = 16;
    static constexpr int sMergePly = 8;

    class BackyardConsumer
    {
    public:
        void push_back(const KmerBlockPtr& pBlk)
        {
            Profile::Context pc("BackyardConsumer::push_back");
            const KmerBlock& blk(*pBlk);
            for (uint64_t i = 0; i < blk.size(); ++i)
            {
                mHash.insert(blk[i]);
            }
        }

        void end()
        {
        }

        BackyardConsumer(BackyardHash& pHash)
            : mHash(pHash)
        {
        }

    private:
        BackyardHash& mHash;
    };

    class NakedGraph
    {
    public:
        class Builder
        {
        public:
            void push_back(const Gossamer::position_type& pEdge, const uint64_t& pCount)
            {
                Gossamer::EdgeAndCount itm(pEdge, pCount);
                EdgeAndCountCodec::encode(mOut, mPrevEdge, itm);
                mPrevEdge = itm.first;
            }

            void end()
            {
            }

            Builder(const std::string& pBaseName, FileFactory& pFactory)
                : mOutHolder(pFactory.out(pBaseName)), mOut(**mOutHolder),
                  mPrevEdge(0)
            {
            }

        private:
            FileFactory::OutHolderPtr mOutHolder;
            std::ostream& mOut;
            Gossamer::position_type mPrevEdge;
        };
    };

    uint64_t flushMerge(KmerBlock **pBlocks, unsigned pNumBlocks, const std::string& pGraphName,
                        Logger& pLog, FileFactory& pFactory)
    {
        struct HeapEntry {
            KmerBlock* block;
            KmerBlock::const_iterator ii, end;
            bool empty() const { return ii == end; }
        };

        HeapEntry merge_entries[sMergePly];
        HeapEntry* heap[sMergePly];

        unsigned entries = 0;
        for (unsigned i = 0; i < pNumBlocks; ++i) {
            // BOOST_ASSERT(std::is_sorted(pBlocks[i]->begin(), pBlocks[i]->end()));
            merge_entries[i].block = pBlocks[i];
            merge_entries[i].ii = pBlocks[i]->begin();
            merge_entries[i].end = pBlocks[i]->end();
            if (merge_entries[i].ii != merge_entries[i].end) {
                heap[entries++] = &merge_entries[i];
            }
        }

        // Build the heap
        for (int i = 1; i < entries; ++i) {
            int child = i;
            do {
                int parent = (child-1)/2;
                if (*heap[parent]->ii > *heap[child]->ii) {
                    std::swap(heap[parent], heap[child]);
                }
                child = parent;
            } while (child);
        }

        uint32_t n = 0;
        std::pair<Gossamer::edge_type,uint64_t> prev(*heap[0]->ii, 0);

        auto check_heap = [&]() {
            for (int i = 1; i < entries; ++i) {
                int parent = (i-1)/2;
                BOOST_ASSERT(*heap[parent]->ii <= *heap[i]->ii);
            }
        };

        // check_heap();

        try
        {
            NakedGraph::Builder bld(pGraphName, pFactory);
            while (entries) {

                while (!heap[0]->empty() && *heap[0]->ii == prev.first) {
                    ++prev.second;
                    ++heap[0]->ii;
                }

                // Did we exhaust the block?
                if (heap[0]->empty()) {
                    if (entries > 1) {
                        heap[0] = heap[--entries];
                        int parent = 0;
                        int child;
                        do {
                            child = 2 * parent + 1;
                            if (child >= entries) {
                                break;
                            }
                            if (child + 1 < entries && *heap[child]->ii > *heap[child+1]->ii) {
                                ++child;
                            }
                            if (*heap[parent]->ii <= *heap[child]->ii) {
                                break;
                            }
                            std::swap(heap[parent], heap[child]);
                            parent = child;
                        } while (true);
                    }
                    else {
                        --entries;
                    }
                    // check_heap();
                    continue;
                }

                // Try a down heap
                bool down_heap_worked = false;
                if (entries) {
                    int parent = 0;
                    int child;
                    do {
                        child = 2 * parent + 1;
                        if (child >= entries) {
                            break;
                        }
                        if (child + 1 < entries && *heap[child]->ii > *heap[child+1]->ii) {
                            ++child;
                        }
                        if (*heap[parent]->ii <= *heap[child]->ii) {
                            break;
                        }
                        down_heap_worked = true;
                        std::swap(heap[parent], heap[child]);
                        parent = child;
                    } while (true);
                    // check_heap();
                }

                if (!down_heap_worked) {
                    // We must have exhausted this entry.
                    bld.push_back(prev.first, prev.second);
                    ++n;
                    if (entries) {
                        BOOST_ASSERT(prev.first < *heap[0]->ii);
                        prev.first = *heap[0]->ii;
                        prev.second = 0;
                        // check_heap();
                    }
                }
            }
            if (prev.second > 0) {
                bld.push_back(prev.first, prev.second);
                ++n;
            }

        }
        catch (std::ios_base::failure& e)
        {
            BOOST_THROW_EXCEPTION(Gossamer::error()
                << Gossamer::write_error_info(pGraphName));
        }
        pLog(info, "wrote " + boost::lexical_cast<std::string>(n) + " pairs.");
        return n;
    }


    uint64_t flushNaked(const BackyardHash& pHash, const std::string& pGraphName,
                        uint64_t pNumThreads, Logger& pLog, FileFactory& pFactory)
    {
        std::vector<uint32_t> perm;
        pLog(info, "sorting the hashtable...");
        pHash.sort(perm, pNumThreads);
        pLog(info, "sorting done.");
        pLog(info, "writing out naked edges.");
        uint32_t n = 0;

        try
        {
            NakedGraph::Builder bld(pGraphName, pFactory);
            auto permsize = perm.size();
            if (permsize > 0)
            {
                // Keep track of the previous edge/count pair,
                // since it is *possible* for the BackyardHash
                // to contain duplicates.

                std::pair<Gossamer::edge_type,uint64_t> prev = pHash[perm[0]];
                for (uint32_t i = 1; i < permsize ; ++i)
                {
                    std::pair<Gossamer::edge_type,uint64_t> itm = pHash[perm[i]];
                    //cerr << "edge: " << itm.first.value() << ", count: " << itm.second << '\n';
                    if ((itm.first < prev.first))
                    {
                        std::cerr << "ERROR! Edges out of order\n";
                    }

                    if (itm.first == prev.first)
                    {
                        prev.second += itm.second;
                        continue;
                    }
                    bld.push_back(prev.first, prev.second);
                    ++n;
                    prev = itm;
                }
                bld.push_back(prev.first, prev.second);
                ++n;
            }
            bld.end();
        }
        catch (std::ios_base::failure& e)
        {
            BOOST_THROW_EXCEPTION(Gossamer::error()
                << Gossamer::write_error_info(pGraphName));
        }
        pLog(info, "wrote " + boost::lexical_cast<std::string>(perm.size()) + " pairs.");
        return n;
    }


    void flush(const BackyardHash& pHash, uint64_t pK, const std::string& pGraphName,
               uint64_t pNumThreads, Logger& pLog, FileFactory& pFactory)
    {
        std::vector<std::uint32_t> perm;
        pLog(info, "sorting the hashtable...");
        pHash.sort(perm, pNumThreads);
        pLog(info, "sorting done.");

        try
        {
            KmerSet::Builder bld(pK, pGraphName, pFactory, perm.size());
            if (perm.size() > 0)
            {
                // Keep track of the previous edge/count pair,
                // since it is *possible* for the BackyardHash
                // to contain duplicates.
                std::pair<Gossamer::edge_type,std::uint64_t> prev = pHash[perm[0]];
                for (uint32_t i = 1; i < perm.size(); ++i)
                {
                    std::pair<Gossamer::edge_type,uint64_t> itm = pHash[perm[i]];
                    //cerr << "edge: " << itm.first.value() << ", count: " << itm.second << '\n';
                    if ((itm.first < prev.first))
                    {
                        std::cerr << "ERROR! Edges out of order\n";
                    }

                    if (itm.first == prev.first)
                    {
                        prev.second += itm.second;
                        continue;
                    }
                    bld.push_back(Gossamer::edge_type(prev.first), prev.second);
                    prev = itm;
                }
                bld.push_back(Gossamer::edge_type(prev.first), prev.second);
            }
            bld.end();
        }
        catch (std::ios_base::failure& e)
        {
            BOOST_THROW_EXCEPTION(Gossamer::error()
                << Gossamer::write_error_info(pGraphName));
        }
    }
}


template<typename KmerSrc> 
void 
GossCmdBuildKmerSet::operator()(const GossCmdContext& pCxt, KmerSrc& pKmerSrc)
{
    Logger& log(pCxt.log);
    FileFactory& fac(pCxt.fac);

    Timer t;
    log(info, "accumulating edges.");
    
    std::vector<std::string> parts;
    std::vector<uint64_t> sizes;
    std::string tmp = fac.tmpName();

#ifdef GOSS_BUILD_KMER_SET_WITH_BACKYARD_HASH
    const uint64_t N = mM / (1.5 * sizeof(uint32_t) + sizeof(BackyardHash::value_type));

    log(info, "using " + boost::lexical_cast<std::string>(mS) + " slot bits.");
    log(info, "using " + boost::lexical_cast<std::string>(log2(N)) + " table bits.");

    BackyardHash h(mS, 2 * mK, N);
    BackyardConsumer bc(h);

    BackgroundMultiConsumer<KmerBlockPtr> bg(4096);
    for (uint64_t i = 0; i < mT; ++i)
    {
        bg.add(bc);
    }

    KmerBlockPtr blk(new KmerBlock);
    blk->reserve(blkSz);
    uint64_t n = 0;
    uint64_t nSinceClear = 0;
    uint64_t j = 0;
    uint64_t prevLoad = 0;
    const uint64_t m = (1 << (mS / 2)) - 1;
    uint64_t z = 0;
    while (pKmerSrc.valid())
    {
        Gossamer::position_type kmer = *pKmerSrc;
        kmer.normalize(mK);
        blk->push_back(kmer);
        if (blk->size() == blkSz)
        {
            Profile::Context pc("GossCmdBuildKmerSet::push-block");
            bg.push_back(blk);
            blk = KmerBlockPtr(new KmerBlock);
            blk->reserve(blkSz);
            ++nSinceClear;
            if ((++n & m) == 0)
            {
                uint64_t cap = h.capacity();
                uint64_t spl = h.spills();
                uint64_t sz = h.size();
                double ld = static_cast<double>(sz) / static_cast<double>(cap);
                uint64_t l = 200 * ld;
                if (l != prevLoad)
                {
                    log(info, "processed " + boost::lexical_cast<std::string>(n * blkSz) + " individual k-mers.");
                    log(info, "hash table load is " + boost::lexical_cast<std::string>(ld));
                    log(info, "number of spills is " + boost::lexical_cast<std::string>(spl));
                    log(info, "the average k-mer frequency is " + boost::lexical_cast<std::string>(1.0 * (nSinceClear * blkSz) / sz));
                    prevLoad = l;
                }
                if (spl > 100)
                {
                    bg.sync(mT);
                    uint64_t cap = h.capacity();
                    uint64_t sz = h.size();
                    double ld = static_cast<double>(sz) / static_cast<double>(cap);
                    log(info, "hash table load at dumping is " + boost::lexical_cast<std::string>(ld));
                    std::string nm = tmp + "-" + boost::lexical_cast<std::string>(j++);
                    log(info, "dumping temporary graph " + nm);
                    uint64_t z0 = flushNaked(h, nm, mT, log, fac);
                    z += z0;
                    parts.push_back(nm);
                    sizes.push_back(z0);
                    h.clear();
                    nSinceClear = 0;
                    log(info, "done.");
                }
            }
        }
        ++pKmerSrc;
    }
    if (blk->size() > 0)
    {
        bg.push_back(blk);
        blk = KmerBlockPtr();
    }
    bg.wait();

    if (parts.size() == 0)
    {
        log(info, "writing out graph (no merging necessary).");
        flush(h, mK, mKmerSetName, mT, log, fac);
    }
    else
    {
        if (h.size() > 0)
        {
            std::string nm = tmp + "-" + boost::lexical_cast<std::string>(j++);
            log(info, "dumping temporary graph " + nm);
            uint64_t z0 = flushNaked(h, nm, mT, log, fac);
            z += z0;
            parts.push_back(nm);
            sizes.push_back(z0);
            h.clear();
            log(info, "done.");
        }
        
        log(info, "merging temporary graphs");

        AsyncMerge::merge<KmerSet>(parts, sizes, mKmerSetName, mK, z, mT, 65536, fac);

        for (uint64_t i = 0; i < parts.size(); ++i)
        {
            fac.remove(parts[i]);
        }
    }
#else
    uint64_t num_blocks = std::max<uint64_t>(mT * 2, sMergePly * 2);
    uint64_t buffer_size = Gossamer::align_down(mM / num_blocks, Gossamer::sPageAlignBits) / sizeof(Gossamer::position_type);
    std::mutex free_mut, merge_mut;
    std::condition_variable condvar;
    std::vector<KmerBlock> blocks;
    std::vector<KmerBlock*> merge_blocks;
    std::vector<KmerBlock*> free_blocks;
    unsigned temp_name = 0;
    uint64_t z = 0;

    blocks.resize(num_blocks);
    merge_blocks.reserve(num_blocks);
    free_blocks.reserve(num_blocks);

    for (auto& blk : blocks) {
        free_blocks.push_back(&blk);
    }

    {
        WorkQueue wq(mT);
        KmerBlock* curblk = 0;
        while (pKmerSrc.valid())
        {
            if (!curblk) {
                std::unique_lock<std::mutex> free_lock(free_mut);
                condvar.wait(free_lock, [&]{ return free_blocks.size() > 0; });
                curblk = free_blocks.back();
                // std::cerr << "Acquired " << curblk << "\n";
                free_blocks.pop_back();
                curblk->reserve(buffer_size);
            }

            Gossamer::position_type kmer = *pKmerSrc;
            curblk->push_back(kmer);
            if (curblk->size() >= buffer_size) {
                auto thisblk = curblk;
                curblk = 0;
                // std::cerr << "Sending " << thisblk << " for processing\n";
                wq.push_back([&,thisblk]{
                    // std::cerr << "Thread received " << thisblk << "\n";

                    for (auto& kmer : *thisblk) {
                        kmer.normalize(mK);
                    }

                    Gossamer::sortKmers(mK, *thisblk);

                    std::unique_lock<std::mutex> merge_lock(merge_mut);
                    merge_blocks.push_back(thisblk);
                    if (merge_blocks.size() >= sMergePly) {
                        const auto num_blocks = std::min<std::size_t>(merge_blocks.size(), sMergePly);
                        KmerBlock* blocks_to_merge[sMergePly];
                        for (unsigned i = 0; i < num_blocks; ++i) {
                            blocks_to_merge[i] = merge_blocks.back();
                            merge_blocks.pop_back();
                        }
                        std::string nm = tmp + "-" + boost::lexical_cast<std::string>(temp_name++);
                        log(info, "dumping temporary graph " + nm);
                        merge_lock.unlock();
                        auto z0 = flushMerge(blocks_to_merge, num_blocks, nm, log, fac);
                        std::unique_lock<std::mutex> free_lock(free_mut);
                        log(info, "dump of " + nm + " done.");
                        for (unsigned i = 0; i < num_blocks; ++i) {
                            blocks_to_merge[i]->clear();
                            free_blocks.push_back(blocks_to_merge[i]);
                            // std::cerr << "Recycled " << blocks_to_merge[i] << "\n";
                        }
                        condvar.notify_all();
                        parts.push_back(nm);
                        sizes.push_back(z0);
                        z += z0;
                    }
                });
            }
            ++pKmerSrc;
        }
    }

    if (merge_blocks.size()) {
        const auto num_blocks = std::min<std::size_t>(merge_blocks.size(), sMergePly);
        KmerBlock* blocks_to_merge[sMergePly];
        for (unsigned i = 0; i < num_blocks; ++i) {
            blocks_to_merge[i] = merge_blocks.back();
            merge_blocks.pop_back();
        }
        std::string nm = tmp + "-" + boost::lexical_cast<std::string>(temp_name++);
        log(info, "dumping temporary graph " + nm);
        z += flushMerge(blocks_to_merge, num_blocks, nm, log, fac);
        log(info, "dump of " + nm + " done.");
    }

    // Force a clear of buffer memory
    for (auto& blk : blocks) {
        blk.swap(KmerBlock());
    }

    if (parts.size() > 0) {
        log(info, "merging temporary graphs");

        uint64_t buffer_size = Gossamer::align_down(std::max<uint64_t>(mM * 0.1 / parts.size(), 65536), Gossamer::sPageAlignBits) / sizeof(Gossamer::EdgeAndCount);
        AsyncMerge::merge<KmerSet>(parts, sizes, mKmerSetName, mK, z, mT, buffer_size, fac);

        for (uint64_t i = 0; i < parts.size(); ++i)
        {
            fac.remove(parts[i]);
        }
    }

#endif

    log(info, "finish graph build");
    log(info, "total build time: " + boost::lexical_cast<std::string>(t.check()));
}
