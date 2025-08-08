// Copyright (c) 2024 Andrew J. Bromage
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//


#include "Deque.hh"
#include <vector>
#include <deque>
#include <boost/timer/timer.hpp>

using namespace boost;
using namespace std;

#define GOSS_TEST_MODULE TestDeque
#include "testBegin.hh"

#include <iostream>

template<typename T>
void dump_container(T& q)
{
	for (auto it = q.begin(); it != q.end(); ++it) {
		cerr << ' ' << *it;
		cerr.flush();
	}
	cerr << '\n';
}


template<typename T>
void basic_queue_test(T& q)
{
	constexpr unsigned test_values = 256;
	std::vector<uint64_t> values;
	values.reserve(test_values);
	for (unsigned i = 0; i < test_values; ++i) {
		values.push_back(i);
	}

	for (auto x : values) {
		q.push_back(x);
		// dump_container(q);
	}
	for (auto x : values) {
		auto item = q.front();
		BOOST_CHECK_EQUAL(x, item);
		q.pop_front();
		// dump_container(q);
	}
	BOOST_CHECK(!q.size());
}


template<typename T>
void basic_revqueue_test(T& q)
{
	constexpr unsigned test_values = 256;
	std::vector<uint64_t> values;
	values.reserve(test_values);
	for (unsigned i = 0; i < test_values; ++i) {
		values.push_back(i);
	}

	for (auto x : values) {
		q.push_front(x);
		// dump_container(q);
	}
	for (auto x : values) {
		auto item = q.back();
		BOOST_CHECK_EQUAL(x, item);
		q.pop_back();
		// dump_container(q);
	}
	BOOST_CHECK(!q.size());
}


template<typename T>
void basic_stack_test(T& q)
{
	constexpr unsigned test_values = 256;
	std::vector<uint64_t> values;
	values.reserve(test_values);
	for (unsigned i = 0; i < test_values; ++i) {
		values.push_back(i);
	}

	for (auto x : values) {
		q.push_back(x);
		// dump_container(q);
	}
	// dump_container(q);

	std::for_each(values.rbegin(), values.rend(), [&](auto x) {
		auto item = q.back();
		BOOST_CHECK_EQUAL(x, item);
		q.pop_back();
		// dump_container(q);
	});
	BOOST_CHECK(!q.size());
}


template<typename T>
void basic_revstack_test(T& q)
{
	constexpr unsigned test_values = 256;
	std::vector<uint64_t> values;
	values.reserve(test_values);
	for (unsigned i = 0; i < test_values; ++i) {
		values.push_back(i);
	}

	for (auto x : values) {
		q.push_front(x);
		// dump_container(q);
	}
	// dump_container(q);

	std::for_each(values.rbegin(), values.rend(), [&](auto x) {
		auto item = q.front();
		BOOST_CHECK_EQUAL(x, item);
		q.pop_front();
		// dump_container(q);
	});
	BOOST_CHECK(!q.size());
}


BOOST_AUTO_TEST_CASE(time_algorithm)
{
	constexpr uint64_t test_values = 5000000;
	std::vector<uint64_t> values;
	values.reserve(test_values);
	for (unsigned i = 1; i <= test_values; ++i) {
		values.push_back(i);
	}

	bool ok = false;
	{
		std::cerr << "Testing deque in queue mode\n";
		boost::timer::auto_cpu_timer timer;

		std::deque<uint64_t> q(values.begin(), values.end());
		while (q.size() > 1) {
			auto x1 = q.front();
			q.pop_front();
			auto x2 = q.front();
			q.pop_front();
			q.push_back(x1 + x2);
		}
		ok = q.front() == test_values * (test_values + 1) / 2;
	}
	BOOST_CHECK(ok);

	{
		std::cerr << "Testing deque in stack mode\n";
		boost::timer::auto_cpu_timer timer;

		std::deque<uint64_t> q(values.begin(), values.end());
		while (q.size() > 1) {
			auto x1 = q.back();
			q.pop_back();
			auto x2 = q.back();
			q.pop_back();
			q.push_back(x1 + x2);
		}
		ok = q.front() == test_values * (test_values + 1) / 2;
	}
	BOOST_CHECK(ok);

	{
		std::cerr << "Testing Queue\n";
		boost::timer::auto_cpu_timer timer;

		Queue<uint64_t> q(values.begin(), values.end());
		while (q.size() > 1) {
			auto x1 = q.front();
			q.pop_front();
			auto x2 = q.front();
			q.pop_front();
			q.push_back(x1 + x2);
		}
		ok = q.front() == test_values * (test_values + 1) / 2;
	}
	BOOST_CHECK(ok);

	{
		std::cerr << "Testing Stack\n";
		boost::timer::auto_cpu_timer timer;

		Stack<uint64_t> q(values.begin(), values.end());
		while (q.size() > 1) {
			auto x1 = q.back();
			q.pop_back();
			auto x2 = q.back();
			q.pop_back();
			q.push_back(x1 + x2);
		}
		ok = q.front() == test_values * (test_values + 1) / 2;
	}
	BOOST_CHECK(ok);
}


BOOST_AUTO_TEST_CASE(test_stack_basic)
{
	Queue<int> q;
	basic_stack_test(q);
	Stack<int> s;
	basic_stack_test(s);
	Deque<int> d;
	basic_stack_test(d);
}

BOOST_AUTO_TEST_CASE(test_queue_basic)
{
	Queue<int> q;
	basic_queue_test(q);
	Stack<int> s;
	basic_queue_test(s);
	Deque<int> d;
	basic_queue_test(d);
}

BOOST_AUTO_TEST_CASE(test_revstack_basic)
{
	Queue<int> q;
	basic_revstack_test(q);
	Stack<int> s;
	basic_revstack_test(s);
	Deque<int> d;
	basic_revstack_test(d);
}

BOOST_AUTO_TEST_CASE(test_revqueue_basic)
{
	Queue<int> q;
	basic_revqueue_test(q);
	Stack<int> s;
	basic_revqueue_test(s);
	Deque<int> d;
	basic_revqueue_test(d);
}

#include "testEnd.hh"