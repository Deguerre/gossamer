// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "GossCmdComputeNearKmers.hh"

#include "Utils.hh"
#include "FastaParser.hh"
#include "GossCmdReg.hh"
#include "GossOptionChecker.hh"
#include "GossReadSequenceBases.hh"
#include "KmerSet.hh"
#include "Phylogeny.hh"
#include "ProgressMonitor.hh"
#include "RunLengthCodedSet.hh"
#include "Spinlock.hh"
#include "WorkQueue.hh"
#include "Timer.hh"

#include <string>
#include <atomic>
#include <boost/lexical_cast.hpp>
#include <thread>

using namespace boost;
using namespace boost::program_options;
using namespace std;


// Usually no need to synchronise the lb and rb bit vectors, because different
// threads never access the same underlying word.
#undef SYNCHRONISE_BIT_VECTORS

void
GossCmdComputeNearKmers::operator()(const GossCmdContext& pCxt)
{
    FileFactory& fac(pCxt.fac);
    Logger& log(pCxt.log);

    Timer t;

    KmerSet s(mIn, fac);

    dynamic_bitset<> lb(s.count());
    dynamic_bitset<> rb(s.count());

    std::atomic<uint64_t> global(0);
    std::atomic<uint64_t> gray(0);
    {
        WordyBitVector lhs(mIn + ".lhs-bits", fac);
        WordyBitVector rhs(mIn + ".rhs-bits", fac);

        // Initialise lb and rb.
        log(info, "initialising bitsets");
        for (uint64_t i = 0; i < s.count(); ++i)
        {
            lb[i] = lhs.get(i);
            rb[i] = rhs.get(i);
        }

        log(info, "calculating grey set");
        WorkQueue wq(mNumThreads);

        struct BlockWorkerJob
        {
            BlockWorkerJob(const uint64_t& pBegin, const uint64_t& pEnd)
                : mBegin(pBegin), mEnd(pEnd)
            {
            }

            const uint64_t mBegin;
            const uint64_t mEnd;
            std::promise<void> mComplete;
        };

        Queue<BlockWorkerJob> jobs;
        const uint64_t d = Gossamer::align_up((uint64_t(1) << 21) * mNumThreads, 10);
        ProgressMonitorFixed pm(log, s.count(), d);
#ifdef SYNCHRONISE_BIT_VECTORS
        Spinlock lbLock, rbLock;
#endif
        for (uint64_t i = 0, b = 0; b < s.count(); ++i, b += d)
        {
            uint64_t e = std::min<uint64_t>(b + d, s.count());
            jobs.emplace_back(b, e);
        }

        std::vector<Gossamer::position_type> nucleotides;
        nucleotides.reserve(s.K() * 4);
        {
            for (uint64_t j = 0; j < s.K(); ++j)
            {
                for (uint64_t b = 0; b < 4; ++b) {
                    Gossamer::position_type m(b);
                    m <<= j*2;
                    nucleotides.push_back(m);
                }
            }
        }

		for (auto& job : jobs) {
            wq.push_back([&] {
                KmerSet::RangeIterator it(s, job.mBegin, job.mEnd);
                for (uint64_t i = job.mBegin; i < job.mEnd; ++i, ++global, ++it)
                {
                    if (lhs.get(i) == rhs.get(i))
                    {
                        continue;
                    }
                    BOOST_ASSERT(it.curRank() == i);
                    auto [x, count] = *it;
                    bool found = false;
                    for (auto& nt : nucleotides) {
                        KmerSet::Edge y = x;
                        y.value() ^= nt;
                        if (x == y) {
                            continue;
                        }
                        s.normalize(y);
                        uint64_t r = 0;
                        if (s.accessAndRank(y, r) && lhs.get(r) != rhs.get(r) && lhs.get(i) != lhs.get(r))
                        {
                            found = true;
                            break;
                        }
                    }
                    bool lhsBit = lhs.get(i);
                    bool rhsBit = lhs.get(i);
                    if (found)
                    {
                        ++gray;
                        lhsBit = rhsBit = false;
                    }
                    {
#ifdef SYNCHRONISE_BIT_VECTORS
                        SpinlockHolder lk(lbLock);
#endif
                        lb[i] = lhsBit;
                    }
                    {
#ifdef SYNCHRONISE_BIT_VECTORS
                        SpinlockHolder lk(rbLock);
#endif
                        rb[i] = rhsBit;
                    }
                }
                job.mComplete.set_value();
            });
	    }
        for (auto& job: jobs) {
            auto future = job.mComplete.get_future();
            if (!Gossamer::future_is_ready(future)) {
                pm.tick(global);
            }
            future.wait();
        }
        pm.end();
    }

    log(info, "found " + lexical_cast<string>(gray) + " gray bits (out of " + lexical_cast<string>(s.count()) + ").");
    WordyBitVector::Builder lhsBld(mIn + ".lhs-bits", fac);
    WordyBitVector::Builder rhsBld(mIn + ".rhs-bits", fac);
    for (uint64_t i = 0; i < s.count(); ++i)
    {
        lhsBld.push_backX(lb[i]);
        rhsBld.push_backX(rb[i]);
    }
    lhsBld.end();
    rhsBld.end();

    log(info, "total elapsed time: " + lexical_cast<string>(t.check()));
}


GossCmdPtr
GossCmdFactoryComputeNearKmers::create(App& pApp, const variables_map& pOpts)
{
    GossOptionChecker chk(pOpts);

    string in;
    chk.getRepeatingOnce("graph-in", in);

    uint64_t t = 4;
    chk.getOptional("num-threads", t);
    chk.throwIfNecessary(pApp);

    return make_goss_cmd<GossCmdComputeNearKmers>(in, t);
}

GossCmdFactoryComputeNearKmers::GossCmdFactoryComputeNearKmers()
    : GossCmdFactory("Decorate a graph with an assignment of kmers to graphs.")
{
    mCommonOptions.insert("graph-in");
}
