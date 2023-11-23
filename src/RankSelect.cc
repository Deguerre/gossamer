// Copyright (c) 2023 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//

#include "RankSelect.hh"
#include <algorithm>
#include <boost/assert.hpp>


/* BIG THEORY STATEMENT:
 * 
 * Sorting BigIntegers using std::sort is expensive because of the full multi-word
 * comparison happening all the time, even on small ranges. It's not actually the
 * comparisons that are the problem, but rather the mispredicted branches inside the
 * comparison operation itself.
 * 
 * Instead, this custom sort tries very hard to minimise the multi-word comparisons
 * by using a combination of radix sort (which is also very cache-efficient on large
 * ranges due to the sequential scanning) and ternary quick sort on word keys.
 */

namespace {
	const std::size_t sLogRadixBits = 3;
	const std::size_t sRadixBits = 1 << sLogRadixBits;
	const std::size_t sRadix = 1 << sRadixBits;
	const std::size_t sTinyRange = 32;
	const std::size_t sSmallRange = 1000;

	inline uint64_t
	key(uint64_t pWord, const Gossamer::position_type& p1)
	{
		return p1.value().words().first[pWord];
	}

	inline void
	compareAndSwap(uint64_t pWord, Gossamer::position_type& p1, Gossamer::position_type& p2)
	{
		if (key(pWord, p2) < key(pWord, p1)) {
			std::swap(p1, p2);
		}
	}


	inline void
    sort3(uint64_t pWord, Gossamer::position_type& p1, Gossamer::position_type& p2, Gossamer::position_type& p3)
	{
		compareAndSwap(pWord, p1, p2);
		compareAndSwap(pWord, p2, p3);
		compareAndSwap(pWord, p1, p2);
	}

	Gossamer::position_type&
	pseudoMedian(uint64_t pWord, Gossamer::position_type* base, uint64_t range)
	{
		auto s = range / 8;
		auto m = range / 2;
		auto l = range - 1 - s;
		sort3(pWord, base[0], base[s], base[2 * s]);
		sort3(pWord, base[m-s], base[m], base[m+s]);
		sort3(pWord, base[l-s], base[l], base[l+s]);
		sort3(pWord, base[s], base[m], base[l]);
		return base[m];
	}

	void
	insertionSortKmers(const uint64_t pWord, Gossamer::position_type* pBegin, uint64_t pRange)
	{
		if (pWord == 0) {
			// If we have narrowed the key space down to one word, just sort on words.
			for (int i = 0; i < pRange; ++i) {
				auto j = i;
				for (auto j = i; j > 0 && pBegin[j].asUInt64() < pBegin[j - 1].asUInt64(); --j) {
					std::swap(pBegin[j - 1], pBegin[j]);
				}
			}
		}
		else {
			for (int i = 0; i < pRange; ++i) {
				auto j = i;
				for (auto j = i; j > 0 && pBegin[j] < pBegin[j - 1]; --j) {
					std::swap(pBegin[j - 1], pBegin[j]);
				}
			}
		}
	}

	void
    quickSortKmers(const uint64_t pWord, Gossamer::position_type* pBegin, Gossamer::position_type* pEnd)
	{
		auto range = pEnd - pBegin;

		// Tiny ranges get insertion-sorted.
		if (range < sTinyRange) {
			insertionSortKmers(pWord, pBegin, range);
			return;
		}

		// Small ranges use ternary quicksort by word.
		auto pivot = key(pWord, pseudoMedian(pWord, pBegin, range));
		int pa = 0;
		int pb = 0;
		int pc = range - 1;
		int pd = range - 1;
		for (;;) {
			uint64_t w;
			while (pb <= pc && (w = key(pWord, pBegin[pb])) <= pivot) {
				if (w == pivot) {
					std::swap(pBegin[pa++], pBegin[pb]);
				}
				++pb;
			}
			while (pc >= pb && (w = key(pWord, pBegin[pc])) >= pivot) {
				if (w == pivot) {
					std::swap(pBegin[pc], pBegin[pd--]);
				}
				--pc;
			}
			if (pb > pc) break;
			std::swap(pBegin[pb], pBegin[pc]);
			++pb;
			--pc;
		}
		for (int s = std::min<int>(pa, pb - pa), i = 0; i < s; ++i) {
			std::swap(pBegin[i], pBegin[pb - s + i]);
		}
		for (int s = std::min<int>(pd-pc, range - pd - 1), i = 0; i < s; ++i) {
			std::swap(pBegin[pb+i], pBegin[range - s + i]);
		}
		if (pb - pa > 1) {
			quickSortKmers(pWord, pBegin, pBegin + pb - pa);
		}
		if (pd - pc > 1) {
			quickSortKmers(pWord, pEnd - pd + pc, pEnd);
		}
		auto eqstart = pb - pa;
		auto eqend = range - pd + pc;
		if (eqend - eqstart > 1 && pWord) {
			quickSortKmers(pWord-1, pBegin + eqstart, pBegin + eqend);
		}
	}

	void
	radixSortKmers(const uint64_t pRadixBase, Gossamer::position_type* pBegin, Gossamer::position_type* pEnd)
	{
		auto range = pEnd - pBegin;
		auto word = pRadixBase / BigIntegerBase::sBitsPerWord;
		auto shift = pRadixBase % BigIntegerBase::sBitsPerWord;
		if (range < sSmallRange) {
			quickSortKmers(word, pBegin, pEnd);
			return;
		}

		// American flag radix pass.
		std::size_t counts[sRadix + 1], nextFree[sRadix];
		std::memset(counts, 0, sizeof(counts));
		for (auto it = pBegin; it < pEnd; ++it) {
			auto digit = (key(word, *it) >> shift) & (sRadix - 1);
			++counts[digit];
		}
		{
			std::size_t cum = 0;
			for (int i = 0; i < sRadix; ++i) {
				auto count = counts[i];
				counts[i] = cum;
				nextFree[i] = cum;
				cum += count;
			}
			counts[sRadix] = cum;
		}

		for (int curBlock = 0; curBlock < sRadix; ) {
			auto i = nextFree[curBlock];
			if (i >= counts[curBlock + 1]) {
				++curBlock;
				continue;
			}
			auto digit = (key(word, pBegin[i]) >> shift) & (sRadix - 1);
			if (digit != curBlock) {
				std::swap(pBegin[i], pBegin[nextFree[digit]]);
			}
			++nextFree[digit];
		}

		// Recursively sort the buckets if there is any key material left.
		if (pRadixBase > 0) {
			for (int i = 0; i < sRadix; ++i) {
				auto bbegin = counts[i];
				auto bend = counts[i+1];
				auto brange = bend - bbegin;
				if (brange > 1) {
					radixSortKmers(pRadixBase - sRadixBits, pBegin + bbegin, pBegin + bend);
				}
			}
		}
	}
}

void
Gossamer::sortKmers(const uint64_t pK, Gossamer::position_type* pBegin, Gossamer::position_type* pEnd)
{
	auto radixBits = pK * 2;
	auto radixBase = radixBits > sRadixBits ? Gossamer::align_down(radixBits - 1, sLogRadixBits) : 0;
	radixSortKmers(radixBase, pBegin, pEnd);
}
