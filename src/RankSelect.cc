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

// If we have a fast conditional move instruction that works on Gossamer::position_type,
// we have faster sorting networks on tiny ranges.
#define FAST_CONDITIONAL_MOVE_ON_POSITION_TYPES


namespace {
	const std::size_t sLogRadixBits = 3;
	const std::size_t sRadixBits = 1 << sLogRadixBits;
	const std::size_t sRadix = 1 << sRadixBits;
	const std::size_t sLeafRange = 5;				// Maximum optimised leaf sort
	const std::size_t sCiura1 = 4;					// Ciura Shell sort increments
	const std::size_t sCiura2 = 10;
	const std::size_t sTinyRange = sCiura1 * sLeafRange;
	const std::size_t sSmallRange = 128;
	const std::size_t sNintherThreshold = 64;

	inline uint64_t
	key(uint64_t pWord, const Gossamer::position_type& p1)
	{
		return p1.value().words().first[pWord];
	}

	inline void
	conditionalSwap(bool condition, Gossamer::position_type& p1, Gossamer::position_type& p2)
	{
#ifdef FAST_CONDITIONAL_MOVE_ON_POSITION_TYPES
		Gossamer::position_type tmp = condition ? p2 : p1;
		p2 = condition ? p1 : p2;
		p1 = tmp;
#else
		if (condition) {
			std::swap(p1, p2);
		}
#endif
	}

	inline void
	sort2(uint64_t pWord, Gossamer::position_type& p1, Gossamer::position_type& p2)
	{
		conditionalSwap(key(pWord, p1) > key(pWord, p2), p1, p2);
	}

	// partialSort3 sorts [p1,p2,p3] assuming that p2 and p3 are already in order.
	inline void
	partialSort3(uint64_t pWord, Gossamer::position_type& p1, Gossamer::position_type& p2, Gossamer::position_type& p3)
	{
#ifdef FAST_CONDITIONAL_MOVE_ON_POSITION_TYPES
		bool r1 = key(pWord, p3) < key(pWord, p1);
		Gossamer::position_type tmp = r1 ? p3 : p1;
		p3 = r1 ? p1 : p3;
		bool r2 = key(pWord, tmp) < key(pWord, p2);
		p1 = r2 ? p1 : p2;
		p2 = r2 ? p2 : tmp;
#else
		sort2(pWord, p1, p2);
		sort2(pWord, p2, p3);
#endif
	}

	inline void
    sort3(uint64_t pWord, Gossamer::position_type& p1, Gossamer::position_type& p2, Gossamer::position_type& p3)
	{
		sort2(pWord, p2, p3);
		partialSort3(pWord, p1, p2, p3);
	}


	inline void
	leafSort(Gossamer::position_type* pBegin, unsigned range, unsigned stride)
	{
		switch (range) {
		case 0:
		case 1:
			break;
		case 2:
			sort2(0, pBegin[0 * stride], pBegin[1 * stride]);
			break;
		case 3:
			sort3(0, pBegin[0 * stride], pBegin[1 * stride], pBegin[2 * stride]);
			break;
		case 4:
			sort2(0, pBegin[0 * stride], pBegin[2 * stride]);
			sort2(0, pBegin[1 * stride], pBegin[3 * stride]);
			sort2(0, pBegin[0 * stride], pBegin[1 * stride]);
			sort2(0, pBegin[2 * stride], pBegin[3 * stride]);
			sort2(0, pBegin[1 * stride], pBegin[2 * stride]);
			break;
		case 5:
			sort2(0, pBegin[0 * stride], pBegin[1 * stride]);
			sort2(0, pBegin[3 * stride], pBegin[4 * stride]);
			partialSort3(0, pBegin[2 * stride], pBegin[3 * stride], pBegin[4 * stride]);
			sort2(0, pBegin[1 * stride], pBegin[4 * stride]);
			partialSort3(0, pBegin[0 * stride], pBegin[2 * stride], pBegin[3 * stride]);
			partialSort3(0, pBegin[1 * stride], pBegin[2 * stride], pBegin[3 * stride]);
			break;
		}
	}


	Gossamer::position_type&
	pseudoMedian(uint64_t pWord, Gossamer::position_type* base, uint64_t range)
	{
		if (range >= sNintherThreshold) {
			auto s = range / 8;
			auto m = range / 2;
			auto l = range - 1 - s;
			sort3(pWord, base[0], base[s], base[2 * s]);
			sort3(pWord, base[m - s], base[m], base[m + s]);
			sort3(pWord, base[l - s], base[l], base[l + s]);
			sort3(pWord, base[s], base[m], base[l]);
			return base[m];
		}
		else {
			auto m = range / 2;
			sort3(pWord, base[0], base[m], base[range-1]);
			return base[m];
		}
	}


	void
	shellSortKmers(const uint64_t pWord, Gossamer::position_type* pBegin, const uint64_t pRange)
	{
		if (pWord == 0) {
			// If we have narrowed the key space down to one word, just sort on words.
			if (pRange <= sLeafRange) {
				leafSort(pBegin, pRange, 1);
			}
			else {
				if (pRange > sCiura2) {
					auto maxi = pRange % sCiura2;
					for (int i = 0; i < maxi; ++i) {
						leafSort(&pBegin[i], pRange / sCiura2 + 1, sCiura2);
					}
				}
				if (pRange > sCiura1) {
					auto maxi = pRange % sCiura1;
					for (int i = 0; i < maxi; ++i) {
						leafSort(&pBegin[i], pRange / sCiura1 + 1, sCiura1);
					}
				}
				leafSort(&pBegin[0], sLeafRange, 1);
				for (int i = sLeafRange; i < pRange; ++i) {
					Gossamer::position_type temp = pBegin[i];
					auto j = i;
					for (; j > 0 && pBegin[j-1].asUInt64() > temp.asUInt64(); --j) {
						pBegin[j] = pBegin[j - 1];
					}
					pBegin[j] = temp;
				}
			}
		}
		else {
			// Fall back to three stages of Shell sort.
			int gaps[] = { sCiura2, sCiura1, 1 };
			for (auto gap : gaps) {
				for (int i = gap; i < pRange; ++i) {
					Gossamer::position_type temp = pBegin[i];
					auto j = i;
					for (; j >= gap && pBegin[j - gap] > temp; j -= gap) {
						pBegin[j] = pBegin[j - gap];
					}
					pBegin[j] = temp;
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
			shellSortKmers(pWord, pBegin, range);
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
	radixSortKmers(const int pRadixBase, const int pRadixBits, Gossamer::position_type* pBegin, Gossamer::position_type* pEnd)
	{
		auto radix = 1 << pRadixBits;
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
			auto digit = (key(word, *it) >> shift) & (radix - 1);
			++counts[digit];
		}
		{
			std::size_t cum = 0;
			for (int i = 0; i < radix; ++i) {
				auto count = counts[i];
				counts[i] = cum;
				nextFree[i] = cum;
				cum += count;
			}
			counts[radix] = cum;
		}

		for (int curBlock = 0; curBlock < sRadix; ) {
			auto i = nextFree[curBlock];
			if (i >= counts[curBlock + 1]) {
				++curBlock;
				continue;
			}
			auto digit = (key(word, pBegin[i]) >> shift) & (radix - 1);
			conditionalSwap(digit != curBlock, pBegin[i], pBegin[nextFree[digit]]);
			++nextFree[digit];
		}

		// Recursively sort the buckets if there is any key material left.
		if (pRadixBase > 0) {
			auto nextRadixBase = std::max<int>(pRadixBase - sRadixBits, 0);
			auto nextRadixBits = pRadixBase < sRadixBits ? pRadixBase : sRadixBits;
			for (int i = 0; i < radix; ++i) {
				auto bbegin = counts[i];
				auto bend = counts[i+1];
				auto brange = bend - bbegin;
				if (brange > 1) {
					radixSortKmers(nextRadixBase, nextRadixBits, pBegin + bbegin, pBegin + bend);
				}
			}
		}
	}
}

void
Gossamer::sortKmers(const uint64_t pK, Gossamer::position_type* pBegin, Gossamer::position_type* pEnd)
{
	auto radixBits = pK * 2;
	auto radixWord = radixBits / 64;
	auto radixWordOffset = radixBits & 63;
	if (radixWord > 0) {
		auto radixBase = radixBits > sRadixBits ? Gossamer::align_down(radixBits - 1, sLogRadixBits) : 0;
		radixSortKmers(radixBase, radixBits - radixBase, pBegin, pEnd);
	}
	else {
		int radixBase = std::max<int>(radixBits - sRadixBits, 0);
		int firstRadixBits = std::min<int>(radixBits, sRadixBits);
		radixSortKmers(radixBase, firstRadixBits, pBegin, pEnd);
	}
}
