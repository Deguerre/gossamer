// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "GossCmdBuildKmerSet.hh"

#include "AsyncMerge.hh"
#include "LineSource.hh"
#include "BackgroundMultiConsumer.hh"
#include "BackyardHash.hh"
#include "Debug.hh"
#include "EdgeAndCount.hh"
#include "EdgeCompiler.hh"
#include "FastaParser.hh"
#include "FastqParser.hh"
#include "GossCmdLintGraph.hh"
#include "GossCmdMergeGraphs.hh"
#include "GossCmdReg.hh"
#include "GossOptionChecker.hh"
#include "GossReadSequenceBases.hh"
#include "KmerSet.hh"
#include "KmerizingAdapter.hh"
#include "LineParser.hh"
#include "Logger.hh"
#include "Profile.hh"
#include "ReadSequenceFileSequence.hh"
#include "Timer.hh"
#include "IntegerCodecs.hh"

#include <iostream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>


#undef GOSS_BUILD_KMER_SET_WITH_BACKYARD_HASH


using namespace boost;
using namespace boost::program_options;
using namespace std;

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
            void push_back(const Gossamer::position_type& pEdge)
            {
                mEncoder.encode(mOut, mPrevEdge, pEdge);
                mPrevEdge = pEdge;
            }

            void end()
            {
                mEncoder.encodeEof(mOut);
            }

            Builder(const std::string& pBaseName, FileFactory& pFactory)
                : mOutHolder(pFactory.out(pBaseName)), mOut(**mOutHolder),
                mPrevEdge(~Gossamer::position_type(0))
            {
            }

        private:
            FileFactory::OutHolderPtr mOutHolder;
            std::ostream& mOut;
            Gossamer::position_type mPrevEdge;
            EdgeEncoder<Gossamer::position_type> mEncoder;
        };
    };

    uint64_t flushOne(KmerBlock* pBlock, const std::string& pGraphName, uint64_t pK,
        Logger& pLog, FileFactory& pFactory, bool pFlushThis = false)
    {
        std::uint64_t n = 0;
        FileFactory::OutHolderPtr dumpFile;
        if (pFlushThis) {
            dumpFile = pFactory.out("flush-graph.txt");
            **dumpFile << std::hex << std::setprecision(8);
        }
        try
        {
            NakedGraph::Builder bld(pGraphName, pFactory);
            for (auto& kmer : *pBlock) {
                if (pFlushThis) {
                    auto k1 = kmer.asUInt64();
                    auto kmer2 = kmer;
                    kmer2.reverseComplement(pK);
                    auto k2 = kmer2.asUInt64();

                    **dumpFile << std::setfill('0') << std::setw(16) << std::min(k1, k2) << ' ' << std::max(k1, k2) << '\n';
                }
                if (kmer.asUInt64() == 0x262DC) {
                    std::cerr << "Kmer found (non-merge dump)\n";
                }
                bld.push_back(kmer);
                ++n;
            }
            bld.end();
        }
        catch (std::ios_base::failure& e)
        {
            BOOST_THROW_EXCEPTION(Gossamer::error()
                << Gossamer::write_error_info(pGraphName));
        }
        pLog(info, "wrote " + boost::lexical_cast<std::string>(n) + " kmers.");
        return n;
    }

    uint64_t flushMerge(KmerBlock** pBlocks, unsigned pNumBlocks, const std::string& pGraphName, uint64_t pK,
        Logger& pLog, FileFactory& pFactory, bool pFlushThis = false)
    {
        if (pNumBlocks == 1) {
            return flushOne(pBlocks[0], pGraphName, pK, pLog, pFactory, pFlushThis);
        }

        struct HeapEntry {
            KmerBlock* block;
            KmerBlock::const_iterator ii, end;
            bool empty() const { return ii == end; }
        };

        HeapEntry merge_entries[sMergePly];
        HeapEntry* heap[sMergePly];
        auto heap_compare = [](const HeapEntry* x, const HeapEntry* y) { return *x->ii > *y->ii; };

        FileFactory::OutHolderPtr dumpFile;
        if (pFlushThis) {
            dumpFile = pFactory.out("flush-graph.txt");
            **dumpFile << std::hex << std::setprecision(8);
        }

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
        std::make_heap(heap, heap + entries, heap_compare);

        uint32_t n = 0;
        auto prev = *heap[0]->ii;
        if (prev.asUInt64() == 0x262DC) {
            std::cerr << "Kmer found (merge phase 1)\n";
        }


        auto check_heap = [&]() {
            for (int i = 1; i < entries; ++i) {
                int parent = (i - 1) / 2;
                BOOST_ASSERT(*heap[parent]->ii <= *heap[i]->ii);
            }
            };

        // check_heap();

        try
        {
            NakedGraph::Builder bld(pGraphName, pFactory);
            while (entries) {
                while (!heap[0]->empty() && *heap[0]->ii == prev) {
                    ++heap[0]->ii;
                }

                // Did we exhaust the block?
                if (heap[0]->empty()) {
                    if (entries > 1) {
                        std::pop_heap(heap, heap + entries, heap_compare);
                        --entries;
                    }
                    else {
                        --entries;
                    }
                    // check_heap();
                    continue;
                }

                // Try a down heap
                if (entries) {
                    int parent = 0;
                    int child;
                    bool down_heap_worked = false;
                    do {
                        child = 2 * parent + 1;
                        if (child >= entries) {
                            break;
                        }
                        if (child + 1 < entries && *heap[child]->ii > *heap[child + 1]->ii) {
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
                    if (down_heap_worked) {
                        continue;
                    }
                }

                // We must have exhausted this entry.
                if (pFlushThis) {
                    **dumpFile << std::setfill('0') << std::setw(16) << prev.asUInt64() << '\n';
                }

                bld.push_back(prev);
                ++n;
                if (entries) {
                    BOOST_ASSERT(prev < *heap[0]->ii);
                    prev = *heap[0]->ii;
                    if (prev.asUInt64() == 0x262DC) {
                        std::cerr << "Kmer found (merge phase 2)\n";
                    }
                    // check_heap();
                }
            }
            if (pFlushThis) {
                **dumpFile << std::setfill('0') << std::setw(16) << prev.asUInt64() << '\n';
            }
            bld.push_back(prev);
            bld.end();
        }
        catch (std::ios_base::failure& e)
        {
            BOOST_THROW_EXCEPTION(Gossamer::error()
                << Gossamer::write_error_info(pGraphName));
        }
        pLog(info, "wrote " + boost::lexical_cast<std::string>(n) + " kmers.");
        return n;
    }


    uint64_t flushNaked(const BackyardHash& pHash, const std::string& pGraphName, uint64_t pK,
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

                auto prev = pHash[perm[0]].first;
                if (prev.asUInt64() == 0x262DC) {
                    std::cerr << "Kmer found (flushNaked 1)\n";
                }
                for (uint32_t i = 1; i < permsize; ++i)
                {
                    auto itm = pHash[perm[i]].first;
                    //cerr << "edge: " << itm.value() << '\n';
                    if (itm < prev)
                    {
                        std::cerr << "ERROR! Edges out of order\n";
                    }

                    if (itm == prev)
                    {
                        continue;
                    }
                    bld.push_back(prev);
                    ++n;
                    prev = itm;
                    if (prev.asUInt64() == 0x262DC) {
                        std::cerr << "Kmer found (flushNaked 2)\n";
                    }
                }
                bld.push_back(prev);
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
                auto prev = pHash[perm[0]].first;
                for (uint32_t i = 1; i < perm.size(); ++i)
                {
                    auto itm = pHash[perm[i]].first;
                    //cerr << "edge: " << itm.value() << '\n';
                    if (itm < prev)
                    {
                        std::cerr << "ERROR! Edges out of order\n";
                    }

                    if (itm == prev)
                    {
                        continue;
                    }
                    bld.push_back(Gossamer::edge_type(prev));
                    prev = itm;
                }
                bld.push_back(Gossamer::edge_type(prev));
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

    std::vector<AsyncMerge::Part> parts;
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
        if (kmer.asUInt64() == 0x262DC) {
            std::cerr << "Kmer found (hash 1)\n";
            Gossamer::position_type kmer = *pKmerSrc;
        }
        kmer.normalize(mK);
        if (kmer.asUInt64() == 0x262DC) {
            std::cerr << "Kmer found (hash 2)\n";
        }

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
                    std::string nm = tmp + "-" + boost::lexical_cast<std::string>(j);
                    log(info, "dumping temporary graph " + nm);
                    uint64_t z0 = flushNaked(h, nm, mK, mT, log, fac);
                    z += z0;
                    parts.emplace_back(j, std::move(nm), z0);
                    ++j;
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
            std::string nm = tmp + "-" + boost::lexical_cast<std::string>(j);
            log(info, "dumping temporary graph " + nm);
            uint64_t z0 = flushNaked(h, nm, mK, mT, log, fac);
            z += z0;
            parts.emplace_back(j, std::move(nm), z0);
            ++j;
            h.clear();
            log(info, "done.");
        }

        log(info, "merging temporary graphs");

        AsyncMerge::merge<KmerSet, Gossamer::position_type>(parts, mKmerSetName, mK, z, mT, 65536, fac);

        for (auto& part : parts) {
            fac.remove(part.mFname);
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
                condvar.wait(free_lock, [&] { return free_blocks.size() > 0; });
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
                wq.push_back([&, thisblk] {
                    // std::cerr << "Thread received " << thisblk << "\n";

                    for (auto& kmer : *thisblk) {
                        if (kmer.asUInt64() == 0x262DC) {
                            std::cerr << "Kmer found (sort phase)\n";
                        }
                        kmer.normalize(mK);
                        if (kmer.asUInt64() == 0x262DC) {
                            std::cerr << "Kmer found (sort phase)\n";
                        }
                    }

                    Gossamer::sortKmers(mK, *thisblk);
                    Gossamer::uniqueAfterSort(*thisblk);

                    std::unique_lock<std::mutex> merge_lock(merge_mut);
                    merge_blocks.push_back(thisblk);
                    if (merge_blocks.size() >= sMergePly) {
                        const auto num_blocks = std::min<std::size_t>(merge_blocks.size(), sMergePly);
                        KmerBlock* blocks_to_merge[sMergePly];
                        for (unsigned i = 0; i < num_blocks; ++i) {
                            blocks_to_merge[i] = merge_blocks.back();
                            merge_blocks.pop_back();
                        }
                        auto this_name = temp_name++;
                        std::string nm = tmp + "-" + boost::lexical_cast<std::string>(this_name);
                        log(info, "dumping temporary graph " + nm);
                        merge_lock.unlock();
                        auto z0 = flushMerge(blocks_to_merge, num_blocks, nm, mK, log, fac);
                        std::unique_lock<std::mutex> free_lock(free_mut);
                        log(info, "dump of " + nm + " done.");
                        for (unsigned i = 0; i < num_blocks; ++i) {
                            blocks_to_merge[i]->clear();
                            free_blocks.push_back(blocks_to_merge[i]);
                            // std::cerr << "Recycled " << blocks_to_merge[i] << "\n";
                        }
                        condvar.notify_all();
                        parts.emplace_back(temp_name, std::move(nm), z0);
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
        auto this_name = temp_name++;
        std::string nm = tmp + "-" + boost::lexical_cast<std::string>(this_name);
        log(info, "dumping temporary graph " + nm);
        auto z0 = flushMerge(blocks_to_merge, num_blocks, nm, mK, log, fac, true);
        log(info, "dump of " + nm + " done.");
        parts.emplace_back(temp_name, std::move(nm), z0);
        ++temp_name;
        z += z0;
    }

    // Force a clear of buffer memory
    for (auto& blk : blocks) {
        blk.swap(KmerBlock());
    }

    if (parts.size() > 0) {
        log(info, "merging temporary graphs");

        uint64_t buffer_size = Gossamer::align_down(std::max<uint64_t>(mM * 0.2 / parts.size(), 65536), Gossamer::sPageAlignBits) / sizeof(Gossamer::position_type);
        AsyncMerge::merge<KmerSet, Gossamer::position_type>(parts, mKmerSetName, mK, z, mT, buffer_size, fac);

        for (auto& part : parts) {
            fac.remove(part.mFname);
        }
    }

#endif

    log(info, "finish graph build");
    log(info, "total build time: " + boost::lexical_cast<std::string>(t.check()));
}


GossCmdBuildKmerSet::GossCmdBuildKmerSet(const uint64_t& pK, const uint64_t& pS, const uint64_t& pM,
    const uint64_t& pT, const std::string& pKmerSetName)
    : mK(pK), mS(pS), mM(pM), mT(pT), mKmerSetName(pKmerSetName),
    mFastaNames(), mFastqNames(), mLineNames()
{
}

GossCmdBuildKmerSet::GossCmdBuildKmerSet(const uint64_t& pK, const uint64_t& pS, const uint64_t& pM,
    const uint64_t& pT, const std::string& pKmerSetName,
    const strings& pFastaNames, const strings& pFastqNames, const strings& pLineNames)
    : mK(pK), mS(pS), mM(pM), mT(pT), mKmerSetName(pKmerSetName),
    mFastaNames(pFastaNames), mFastqNames(pFastqNames), mLineNames(pLineNames)
{
}


typedef vector<string> strings;

void
GossCmdBuildKmerSet::operator()(const GossCmdContext& pCxt)
{
    Logger& log(pCxt.log);
    FileFactory& fac(pCxt.fac);

    Queue<GossReadSequence::Item> items;

    {
        GossReadSequenceFactoryPtr seqFac
            = std::make_shared<GossReadSequenceBasesFactory>();

        GossReadParserFactory lineParserFac(LineParser::create);
        for (auto& f: mLineNames)
        {
            items.push_back(GossReadSequence::Item(f, lineParserFac, seqFac));
        }

        GossReadParserFactory fastaParserFac(FastaParser::create);
        for (auto& f: mFastaNames)
        {
            items.push_back(GossReadSequence::Item(f, fastaParserFac, seqFac));
        }

        GossReadParserFactory fastqParserFac(FastqParser::create);
        for (auto& f: mFastqNames)
        {
            items.push_back(GossReadSequence::Item(f, fastqParserFac, seqFac));
        }
    }

    UnboundedProgressMonitor umon(log, 100000, " reads");
    LineSourceFactory lineSrcFac(BackgroundLineSource::create);
    ReadSequenceFileSequence reads(items, fac, lineSrcFac, &umon, &log);

    KmerizingAdapter x(reads, mK);
    (*this)(pCxt, x);
}

GossCmdPtr
GossCmdFactoryBuildKmerSet::create(App& pApp, const variables_map& pOpts)
{

    GossOptionChecker chk(pOpts);

    uint64_t K = 0;
    chk.getMandatory("kmer-size", K, GossOptionChecker::RangeCheck(KmerSet::MaxK));

    uint64_t B = 2;
    chk.getOptional("buffer-size", B);

#ifdef GOSS_BUILD_KMER_SET_WITH_BACKYARD_HASH
    uint64_t S = BackyardHash::maxSlotBits(B << 30);
    chk.getOptional("log-hash-slots", S);
#else
    uint64_t S = 0;
#endif

    uint64_t T = 4;
    chk.getOptional("num-threads", T);

    FileFactory& fac(pApp.fileFactory());
    GossOptionChecker::FileCreateCheck createChk(fac, true);
    GossOptionChecker::FileReadCheck readChk(fac);

    string graphName;
    chk.getMandatory("graph-out", graphName, createChk);

    strings fastaNames;
    chk.getRepeating0("fasta-in", fastaNames, readChk);

    strings fasFiles;
    chk.getOptional("fastas-in", fasFiles);
    chk.expandFilenames(fasFiles, fastaNames, fac);

    strings fastqNames;
    chk.getRepeating0("fastq-in", fastqNames, readChk);

    strings fqsFiles;
    chk.getOptional("fastqs-in", fqsFiles);
    chk.expandFilenames(fqsFiles, fastqNames, fac);

    strings lineNames;
    chk.getRepeating0("line-in", lineNames, readChk);

    chk.throwIfNecessary(pApp);

    return make_goss_cmd<GossCmdBuildKmerSet>(K, S, B, T, graphName, fastaNames, fastqNames, lineNames);
}

GossCmdFactoryBuildKmerSet::GossCmdFactoryBuildKmerSet()
    : GossCmdFactory("create a new graph")
{
    mCommonOptions.insert("kmer-size");
    mCommonOptions.insert("buffer-size");
    mCommonOptions.insert("graph-out");
    mCommonOptions.insert("fasta-in");
    mCommonOptions.insert("fastas-in");
    mCommonOptions.insert("fastq-in");
    mCommonOptions.insert("fastqs-in");
    mCommonOptions.insert("line-in");

#ifdef GOSS_BUILD_KMER_SET_WITH_BACKYARD_HASH
    mCommonOptions.insert("log-hash-slots");
    mSpecificOptions.addOpt<uint64_t>("log-hash-slots", "S",
            "log2 of the number of hash slots to use (default 24)");
#endif
}

