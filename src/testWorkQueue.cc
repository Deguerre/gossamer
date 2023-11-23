// Copyright (c) 2023 Andrew J. Bromage
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "WorkQueue.hh"

#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <boost/dynamic_bitset.hpp>
#include <random>


using namespace boost;
using namespace std;

#define GOSS_TEST_MODULE TestWorkQueue
#include "testBegin.hh"

BOOST_AUTO_TEST_CASE(test1)
{
	ComplexWorkQueue wq(1);

	auto startJob = wq.add([&] {
		std::cerr << "Start job\n";
	});

	auto endJob = wq.add([&] {
		std::cerr << "End job\n";
	});
	for (int i = 0; i < 10; ++i) 
	{
		auto job = wq.add([i] {
			std::cerr << "Job " << i << "\n";
		});
		wq.addDependency(startJob, job);
		wq.addDependency(job, endJob);
		wq.go(job);
	}
	wq.go(endJob);
	wq.go(startJob);

	wq.wait(endJob);
	wq.end();
}


#include "testEnd.hh"
