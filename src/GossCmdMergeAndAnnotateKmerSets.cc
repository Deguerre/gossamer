// Copyright (c) 2008-1016, NICTA (National ICT Australia).
// Copyright (c) 2016, Commonwealth Scientific and Industrial Research
// Organisation (CSIRO) ABN 41 687 119 230.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#include "GossCmdMergeAndAnnotateKmerSets.hh"

#include "LineSource.hh"
#include "BackgroundMultiConsumer.hh"
#include "FastaParser.hh"
#include "GossCmdReg.hh"
#include "GossOptionChecker.hh"
#include "GossReadSequenceBases.hh"
#include "KmerSet.hh"
#include "Phylogeny.hh"
#include "RunLengthCodedSet.hh"
#include "Spinlock.hh"
#include "Timer.hh"
#include "WorkQueue.hh"
#include "ProgressMonitor.hh"
#include "Sample.hh"

#include <string>
#include <boost/lexical_cast.hpp>
#include <random>
#include <unordered_map>

using namespace boost;
using namespace boost::program_options;
using namespace std;


namespace {
    static bool sEstimateGraphStatistics = true;
}

void
GossCmdMergeAndAnnotateKmerSets::operator()(const GossCmdContext& pCxt)
{
    FileFactory& fac(pCxt.fac);
    Logger& log(pCxt.log);

    Timer t;

    KmerSet lhs(mLhs, fac);
    KmerSet rhs(mRhs, fac);

    if (lhs.count() == 0 || rhs.count() == 0)
    {
        throw "nonsense";
    }

    if (lhs.K() != rhs.K())
    {
        throw "nonsense";
    }

	uint64_t n = 0, c = 0;
	if (sEstimateGraphStatistics) {
		// Estimate the number of common kmers between the two sets.

		log(info, "Estimating kmer statistics");

		WorkQueue wq(mNumThreads);
		uint64_t common = 0, visited = 0;

		struct WorkItem {
			unsigned mJobNum;
			KmerSet& mLhs;
			KmerSet& mRhs;
			uint64_t mBegin, mEnd;
			uint64_t c = 0, visited = 0;
			std::promise<void> complete;
			WorkItem(unsigned pJobNum, KmerSet& pLhs, KmerSet& pRhs, uint64_t pBegin, uint64_t pEnd)
				: mJobNum(pJobNum), mLhs(pLhs), mRhs(pRhs), mBegin(pBegin), mEnd(pEnd)
			{
			}
		};

		std::deque<WorkItem> items;
		uint64_t size, chunksize;
		if (lhs.count() > rhs.count()) {
			size = lhs.count();
			chunksize = std::max<uint64_t>(size / mNumThreads / 50, 65536);
			for (uint64_t num = 0, begin = 0; begin <= size; begin += chunksize, ++num) {
				const uint64_t end = std::min<uint64_t>(begin + chunksize, size);
				items.emplace_back(num, lhs, rhs, begin, end);
			}
		}
		else {
			size = rhs.count();
			chunksize = std::max<uint64_t>(size / mNumThreads / 16, 65536);
			for (uint64_t num = 0, begin = 0; begin <= size; begin += chunksize, ++num) {
				const uint64_t end = std::min<uint64_t>(begin + chunksize, size);
				items.emplace_back(num, rhs, lhs, begin, end);
			}
		}
		auto samples = std::min<uint64_t>(std::max<uint64_t>(std::sqrt(size) + 1, 65536), size);
		log(info, "Taking " + lexical_cast<std::string>(samples) + " samples.");

        double drawProbability = (double)samples / (double)size;
		ProgressMonitorFixed mon(log, size, chunksize);
        auto prev_time = std::chrono::steady_clock::now();
		for (auto& item : items) {
            wq.push_back([&]() {
                std::mt19937_64 rng(item.mJobNum);
                const uint64_t populationSize = item.mEnd - item.mBegin;
                const uint32_t sampleSize = (uint32_t)(populationSize * drawProbability) + 1;
                std::vector<uint64_t> draw;
                Gossamer::sampleWithoutReplacement(rng, populationSize, sampleSize, draw);
                std::sort(draw.begin(), draw.end());
				for (auto i : draw) {
					KmerSet::Edge le = item.mLhs.select(i + item.mBegin);
					if (item.mRhs.access(le)) {
						++item.c;
					}
					++item.visited;
				}
				item.complete.set_value();
			});
		}
		uint64_t done = 0;
		for (auto& item : items) {
			auto future = item.complete.get_future();
            auto new_time = std::chrono::steady_clock::now();
			if (!Gossamer::future_is_ready(future) && (new_time - prev_time) >= 5s) {
				mon.tick(done);
                prev_time = new_time;
            }
			future.wait();
			c += item.c;
			visited += item.visited;
			done = item.mEnd;
		}
		mon.end();

		// Use 99% confidence interval
		auto [wmin, wmax] = Gossamer::binomialConfidenceInterval(common, visited, 2.58);
		// log(info, "kmers sampled: " + lexical_cast<std::string>(visited) + " / " + lexical_cast<std::string>(size));
		// log(info, "common found: " + lexical_cast<std::string>(common) + " / " + lexical_cast<std::string>(visited));
		// log(info, "99% confidence interval: " + lexical_cast<std::string>(wmin) + " - " + lexical_cast<std::string>(wmax));
		auto cmin = wmin * size;
		auto nmin = lhs.count() + rhs.count() - cmin;
		auto cmax = wmax * size;
		auto nmax = lhs.count() + rhs.count() - cmax;
		c = common ? (uint64_t)(0.5 * (wmin + wmax) * size) : (uint64_t)(wmax * size);
		n = lhs.count() + rhs.count() - c;
		log(info, "Estimating that " + lexical_cast<string>(c) + " kmers are common, total of " + lexical_cast<string>(n) + " kmers");
	}
	else {
		// Accurately calculate the common kmers between the two sets.

		WorkQueue wq(mNumThreads);

		uint64_t visited = 0;
		struct WorkItem {
			unsigned mJobNum;
			KmerSet& mLhs;
			KmerSet& mRhs;
			uint64_t mBegin, mEnd;
			uint64_t c = 0, visited = 0;
			std::promise<void> complete;
			WorkItem(unsigned pJobNum, KmerSet& pLhs, KmerSet& pRhs, uint64_t pBegin, uint64_t pEnd)
				: mJobNum(pJobNum), mLhs(pLhs), mRhs(pRhs), mBegin(pBegin), mEnd(pEnd)
			{
			}
		};
		std::deque<WorkItem> items;
		uint64_t size;
		if (lhs.count() < rhs.count()) {
			size = lhs.count();
			const uint64_t chunksize = std::max<uint64_t>(size / mNumThreads / 16, 65536);
			for (uint64_t num = 0, begin = 0; begin <= size; begin += chunksize, ++num) {
				const uint64_t end = std::min<uint64_t>(begin + chunksize, size);
				items.emplace_back(num, lhs, rhs, begin, end);
			}
		}
		else {
			size = rhs.count();
			const uint64_t chunksize = std::max<uint64_t>(size / mNumThreads / 16, 65536);
			for (uint64_t num = 0, begin = 0; begin <= size; begin += chunksize, ++num) {
				const uint64_t end = std::min<uint64_t>(begin + chunksize, size);
				items.emplace_back(num, rhs, lhs, begin, end);
			}
		}

		ProgressMonitorNew mon(log, size);
		for (auto& item : items) {
			wq.push_back([&]() {
				for (auto l = item.mBegin; l < item.mEnd; ++l) {
					KmerSet::Edge le = item.mLhs.select(l);
					if (item.mRhs.access(le)) {
						++item.c;
					}
					++item.visited;
				}
				item.complete.set_value();
			});
		}
		for (auto& item : items) {
			item.complete.get_future().wait();
			c += item.c;
			visited += item.visited;
			mon.tick(item.mEnd);
		}

		n = lhs.count() + rhs.count() - c;
		BOOST_ASSERT(visited == n);

		log(info, "writing out " + lexical_cast<string>(n) + " kmers.");
		log(info, "of which " + lexical_cast<string>(c) + " are common.");
	}

    KmerSet::Builder bld(lhs.K(), mOut, fac, n);
    WordyBitVector::Builder lhsBld(mOut + ".lhs-bits", fac);
    WordyBitVector::Builder rhsBld(mOut + ".rhs-bits", fac);

    {
        log(info, "Building bitsets");

        struct OutputRecord {
            Gossamer::position_type edge;
            uint8_t lpresent : 1, rpresent = 1;
            OutputRecord(Gossamer::position_type pEdge, bool pL, bool pR)
                : edge(pEdge), lpresent(pL), rpresent(pR)
            {
            }
        };

        struct WorkItem {
            unsigned mJobNum;
            uint64_t begin, end;
            uint64_t total = 0, common = 0, lcount = 0, rcount = 0;
            std::vector<Gossamer::position_type> edge;
            std::vector<bool> left;
            std::vector<bool> right;
            ComplexWorkQueue::Task* worker = 0;
            ComplexWorkQueue::Task* edgeWriter = 0;
            ComplexWorkQueue::Task* bitWriter = 0;
            ComplexWorkQueue::Task* done = 0;

            WorkItem(unsigned pJobNum, uint64_t pBegin, uint64_t pEnd)
                : mJobNum(pJobNum), begin(pBegin), end(pEnd)
            {
            }
        };

        std::deque<WorkItem> items;

        const uint64_t num_blocks = std::max<uint64_t>(mNumThreads * 2, 2);
        const uint64_t chunksize = Gossamer::align_down(mWorkingMemory * 0.4 / num_blocks, Gossamer::sPageAlignBits) / sizeof(OutputRecord);

        const uint64_t size = lhs.count();
        for (uint64_t num = 0, begin = 0; begin <= size; begin += chunksize, ++num) {
            const uint64_t end = std::min<uint64_t>(begin + chunksize, size);
            items.emplace_back(num, begin, end);
        }

        std::mutex mut;
        uint64_t total = 0;
        uint64_t common = 0;
        uint64_t lcount = 0;
        uint64_t rcount = 0;
        ProgressMonitorNew mon(log, lhs.count());
        ComplexWorkQueue wq(mNumThreads);

        auto jobDone = wq.add([]{});
        for (auto& item : items) {
            item.worker = wq.add([&]() {
                KmerSet::RangeIterator lhsit(lhs, item.begin, item.end);
                auto rhsbegin = rhs.rank(lhs.select(item.begin));
                auto rhsend = item.end == lhs.count() ? rhs.count() : rhs.rank(lhs.select(item.end));
                KmerSet::RangeIterator rhsit(rhs, rhsbegin, rhsend);
                auto capacity = item.end - item.begin + rhsend - rhsbegin;
                item.edge.clear();
                item.edge.reserve(capacity);
                item.left.clear();
                item.left.reserve(capacity);
                item.right.clear();
                item.right.reserve(capacity);

                if (lhsit.valid() && rhsit.valid())
                {
                    auto l = *lhsit;
                    auto r = *rhsit;
                    while (lhsit.valid() && rhsit.valid())
                    {
                        if (l.first < r.first) {
                            item.edge.emplace_back(l.first.value());
                            item.left.push_back(true);
                            item.right.push_back(false);
                            ++lhsit;
                            ++item.total;
                            ++item.lcount;
                            if (lhsit.valid()) l = *lhsit;
                            continue;
                        }
                        if (l.first > r.first) {
                            item.edge.emplace_back(r.first.value());
                            item.left.push_back(false);
                            item.right.push_back(true);
                            ++rhsit;
                            ++item.total;
                            ++item.rcount;
                            if (rhsit.valid()) r = *rhsit;
                            continue;
                        }

                        item.edge.emplace_back(l.first.value());
                        item.left.push_back(true);
                        item.right.push_back(true);
                        ++lhsit;
                        if (lhsit.valid()) l = *lhsit;
                        ++rhsit;
                        if (rhsit.valid()) r = *rhsit;
                        ++item.total;
                        ++item.lcount;
                        ++item.rcount;
                        ++item.common;
                    }
                }
                while (lhsit.valid()) {
                    auto l = *lhsit;
                    item.edge.emplace_back(l.first.value());
                    item.left.push_back(true);
                    item.right.push_back(false);
                    ++lhsit;
                    ++item.total;
                    ++item.lcount;
                }
                while (rhsit.valid()) {
                    auto r = *rhsit;
                    item.edge.emplace_back(r.first.value());
                    item.left.push_back(false);
                    item.right.push_back(true);
                    ++rhsit;
                    ++item.total;
                    ++item.rcount;
                }
            });

            item.edgeWriter = wq.add([&] {
                for (auto& edge : item.edge) {
                    bld.push_back(edge);
                }

                item.edge.swap(std::vector<Gossamer::position_type>());
            });
            wq.addDependency(item.worker, item.edgeWriter);

            item.bitWriter = wq.add([&] {
                BOOST_ASSERT(item.left.size() == item.right.size());
                for (auto i = 0; i < item.left.size(); ++i) {
                    lhsBld.push_backX(item.left[i]);
                    rhsBld.push_backX(item.right[i]);
                }
                item.left.swap(std::vector<bool>());
                item.right.swap(std::vector<bool>());
            });
            wq.addDependency(item.worker, item.bitWriter);

            item.done = wq.add([&] {
                std::unique_lock<std::mutex> lock(mut);
                mon.tick(item.end);
                total += item.total;
                common += item.common;
                lcount += item.lcount;
                rcount += item.rcount;
            });
            wq.addDependency(item.edgeWriter, item.done);
            wq.addDependency(item.bitWriter, item.done);
            wq.addDependency(item.done, jobDone);
        }

        for (unsigned i = 0; i + 1 < items.size(); ++i) {
            wq.addDependency(items[i].edgeWriter, items[i + 1].edgeWriter);
            wq.addDependency(items[i].bitWriter, items[i + 1].bitWriter);
        }

        for (unsigned i = 0; i + mNumThreads < items.size(); ++i) {
            wq.addDependency(items[i].done, items[i + mNumThreads].worker);
        }

        for (auto& item : items) {
            wq.go(item.worker);
            wq.go(item.edgeWriter);
            wq.go(item.bitWriter);
            wq.go(item.done);
        }
        wq.go(jobDone);

        wq.wait(jobDone);
        wq.end();

        bld.end();
        lhsBld.end();
        rhsBld.end();
        mon.end();

        BOOST_ASSERT(lcount == lhs.count());
        BOOST_ASSERT(rcount == rhs.count());

        log(info, "Found " + lexical_cast<string>(common) + " kmers are common, total of " + lexical_cast<string>(total) + " kmers");
    }
#if 0
    {
        uint64_t l = 0;
        uint64_t r = 0;
        uint64_t total = 0;
        uint64_t common = 0;
        KmerSet::Edge le = lhs.select(l);
        KmerSet::Edge re = rhs.select(r);
        while (l < lhs.count() && r < rhs.count())
        {
            if (le < re)
            {
                bld.push_back(le.value());
                lhsBld.push_backX(true);
                rhsBld.push_backX(false);

                ++l;
                ++total;
                if (l < lhs.count())
                {
                    le = lhs.select(l);
                }
                continue;
            }
            if (le > re)
            {
                bld.push_back(re.value());
                lhsBld.push_backX(false);
                rhsBld.push_backX(true);

                ++r;
                ++total;
                if (r < rhs.count())
                {
                    re = rhs.select(r);
                }
                continue;
            }

            bld.push_back(le.value());
            lhsBld.push_backX(true);
            rhsBld.push_backX(true);

            ++l;
            if (l < lhs.count())
            {
                le = lhs.select(l);
            }

            ++r;
            if (r < rhs.count())
            {
                re = rhs.select(r);
            }
            ++total;
            ++common;
        }
        while (l < lhs.count())
        {
            bld.push_back(le.value());
            lhsBld.push_backX(true);
            rhsBld.push_backX(false);

            ++l;
            ++total;
            if (l < lhs.count())
            {
                le = lhs.select(l);
            }
        }
        while (r < rhs.count())
        {
            bld.push_back(re.value());
            lhsBld.push_backX(false);
            rhsBld.push_backX(true);

            ++r;
            ++total;
            if (r < rhs.count())
            {
                re = rhs.select(r);
            }
        }
        bld.end();
        lhsBld.end();
        rhsBld.end();

        log(info, "Actual total kmers: " + lexical_cast<string>(total));
    }
#endif
    log(info, "total elapsed time: " + lexical_cast<string>(t.check()));
}


GossCmdPtr
GossCmdFactoryMergeAndAnnotateKmerSets::create(App& pApp, const variables_map& pOpts)
{
    GossOptionChecker chk(pOpts);

    string lhs;
    string rhs;
    chk.getRepeatingTwice("graph-in", lhs, rhs);

    string out;
    chk.getMandatory("graph-out", out);

    uint64_t T = 4;
    chk.getOptional("num-threads", T);

    uint64_t B = 2;
    chk.getOptional("buffer-size", B);

    chk.throwIfNecessary(pApp);

    return make_goss_cmd<GossCmdMergeAndAnnotateKmerSets>(T, B << 30, lhs, rhs, out);
}

GossCmdFactoryMergeAndAnnotateKmerSets::GossCmdFactoryMergeAndAnnotateKmerSets()
    : GossCmdFactory("Decorate a graph with an assignment of kmers to graphs.")
{
    mCommonOptions.insert("graph-in");
    mCommonOptions.insert("graph-out");
    mCommonOptions.insert("buffer-size");
}
