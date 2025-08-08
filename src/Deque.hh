// Copyright (c) 2024 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef DEQUE_HH
#define DEQUE_HH

// BIG THEORY STATEMENT:
// 
// The implementation is pretty much the same as all deque implementations
// since the SGI STL: an array (called the "map") of fixed-size blocks,
// called "decks".
//
// MSVC famously has an inefficient deque implementation. Everyone keeps
// asking them to fix it, but it would break binary compatibility.
// We need deque a lot, so rewriting it for performance turns out to be
// important. But this also gives us an opportunity to write a data structure
// that can be tuned for stack/queue workloads.
// 
// The time_algorithm test (see testDeque.cc) tests whether this is the case or
// not. On my Windows machine, Deque represents about a 2x speedup compared to
// std::deque.
//
// The key things that you can configure are the left policy and the right
// policy, which determines how many empty decks are kept. The options are
// to keep none, one, or all.
// 
// For a stack or queue, where we exclusively push_back, we keep all
// decks on the right and none on the left. In the case of a queue where
// where we remove decks from the front, they are recycled to the back
// (unless this would result in a reorganisation of the map).
// 
// The tradeoff is that this can result in O(N^2) behaviour (albeit with a
// low constant factor) if you use a Deque against its policy, such as doing
// a lot of push_front on a stack or queue.
// 
// CAVEATS:
// 
// We are not very careful about overflow, which would be important were this
// part of the standard library. But it isn't, so we aren't.
// 
// C++ COMPLIANCE:
//
// We do not claim that this is a drop-in replacement for std::deque, but it
// should be as long as you stick to the operations that it supports (i.e.
// the operations that are used in Gossamer). It is a bug if that isn't the
// case.
// 
// There are a lot of things missing, most notably random access and iterator
// categories. Gossamer does not use Deque in standard algorithms.
// 
// DEPENDENCIES:
// 
// The only "external" dependencies (i.e. those outside of the standard
// library) are boost::compressed_pair (used to store the allocator) and
// Gossamer::roundUpToNextPowerOf2 (used to compute the map size). We don't
// actually need a full compressed_pair, because only the allocator might be
// zero-sized, so that's something to consider if we ever want to make this
// reasonably stand-alone.
// 
// Some day that may happen. Today is not that day.
// 

#ifndef BOOST_COMPRESSED_PAIR_HPP_INCLDED
#include <boost/compressed_pair.hpp>
#define BOOST_COMPRESSED_PAIR_HPP_INCLDED
#endif

#ifndef UTILS_HH
#include "Utils.hh"
#endif

// Defining this does additional work in return for easier debugging of Deque
// itself. Specifically, empty decks are (mostly) filled with known junk,
// and unusued map entries are zeroed out.
#define GOSS_DEBUG_DEQUE

namespace DequeDetail {
    static const uint64_t min_items_per_block = 16;

    constexpr uint64_t _deque_items_per_block(size_t block_size, size_t size) {
        return size < block_size / min_items_per_block ? block_size / size : min_items_per_block;
    }

    enum BlockPolicy {
        DEQUE_KEEP_NONE = 0,
        DEQUE_KEEP_ONE = 1,
        DEQUE_KEEP_ALL = 2
    };
}

struct DequeTraitsBase {
    static const unsigned initial_map_size = 8;
#ifdef GOSS_DEBUG_DEQUE
    static const uint64_t block_size = 4096;
#else
    static const uint64_t block_size = 4096;
#endif
    static const DequeDetail::BlockPolicy left_policy = DequeDetail::DEQUE_KEEP_ONE;
    static const DequeDetail::BlockPolicy right_policy = DequeDetail::DEQUE_KEEP_ONE;
};


template<typename T>
struct DequeTraitsDefault : public DequeTraitsBase {
    static const uint64_t items_per_block = DequeDetail::_deque_items_per_block(block_size, sizeof(T));
};


template<typename T>
struct DequeTraitsQueue : public DequeTraitsDefault<T> {
    static const DequeDetail::BlockPolicy left_policy = DequeDetail::DEQUE_KEEP_NONE;
    static const DequeDetail::BlockPolicy right_policy = DequeDetail::DEQUE_KEEP_ALL;
};

template<typename T>
struct DequeTraitsStack : public DequeTraitsDefault<T> {
    static const DequeDetail::BlockPolicy left_policy = DequeDetail::DEQUE_KEEP_NONE;
    static const DequeDetail::BlockPolicy right_policy = DequeDetail::DEQUE_KEEP_ALL;
};


template <typename T, typename Pointer, typename Reference, typename MapPointer, typename DifferenceType, typename Traits>
class DequeIterator
{
private:
    MapPointer _iter = nullptr;
    Pointer _ptr = nullptr;
public:
    typedef Pointer pointer;
    typedef DifferenceType difference_type;

    typedef T value_type;
    typedef std::random_access_iterator_tag  iterator_category;
    typedef Reference reference;

    reference operator*() const
    {
        return *_ptr;
    }

    pointer operator->() const
    {
        return _ptr;
    }

    DequeIterator& operator++()
    {
        if (++_ptr - *_iter == Traits::items_per_block) {
            ++_iter;
            _ptr = *_iter;
        }
        return *this;
    }

    DequeIterator operator++(int)
    {
        DequeIterator retval(*this);
        ++(*this);
        return retval;
    }

    DequeIterator& operator--()
    {
        if (_ptr == *_iter) {
            --_iter;
            _ptr = &(*_iter)[Traits::items_per_block];
        }
        --_ptr;
        return *this;
    }

    DequeIterator operator--(int)
    {
        DequeIterator retval(*this);
        --(*this);
        return retval;
    }


    DequeIterator& operator+=(difference_type n)
    {
        if (n) {
            n += ptr - *_iter;
            if (_n > 0) {
                _iter += _n / Traits::items_per_block;
                _ptr += *_iter + n % Traits::items_per_block;
            }
            else {
                difference_type d = Traits::items_per_block - 1 - n;
                _iter -= z / Traits::items_per_block;
                _ptr = *_iter + (Traits::items_per_block - 1 - z % Traits::items_per_block);
            }
        }
        return *this;
    }

    DequeIterator& operator-=(difference_type n)
    {
        return *this += -n;
    }

    DequeIterator operator+(difference_type n) const
    {
        DequeIterator retval(*this);
        retval += n;
        return retval;
    }

    DequeIterator operator-(difference_type n) const
    {
        DequeIterator retval(*this);
        retval -= n;
        return retval;
    }

    friend DequeIterator operator+(difference_type n, const DequeIterator& it)
    {
        return it + n;
    }

    friend difference_type operator-(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return lhs == rhs ? 0 : (lhs._iter - rhs._iter) * Traits::items_per_block + (lhs._ptr - *lhs._iter) - (rhs._ptr - *rhs._iter);
    }

    reference operator[](difference_type n) const
    {
        return *(*this + n);
    }

    friend bool operator==(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return lhs._ptr == rhs._ptr;
    }

    friend bool operator!=(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return !(lhs._ptr == rhs._ptr);
    }

    friend bool operator<(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return lhs._iter < rhs._iter || (lhs._iter == rhs._iter && lhs._ptr < lhs._ptr);
    }

    friend bool operator>(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator<=(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return !(rhs < lhs);
    }

    friend bool operator>=(const DequeIterator& lhs, const DequeIterator& rhs)
    {
        return !(lhs < rhs);
    }

private:
    template <typename T, typename Allocator, typename Traits> friend class DequeBase;

    DequeIterator(MapPointer iter, Pointer ptr) noexcept
        : _iter(iter), _ptr(ptr)
    {
    }

    DequeIterator() = delete;
};


template <typename T, typename Allocator, typename Traits>
class DequeBase
{
public:
    typedef typename Allocator allocator_type;
    typedef std::allocator_traits<Allocator> allocator_traits_type;
    typedef typename std::allocator_traits<allocator_type>::size_type size_type;
    typedef typename std::allocator_traits<allocator_type>::difference_type difference_type;
    typedef typename std::allocator_traits<allocator_type>::pointer pointer;
    typedef typename std::allocator_traits<allocator_type>::const_pointer const_pointer;

    typedef T value_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;

private:
    typedef typename allocator_traits_type::template rebind_alloc<T*> _map_allocator_type;
    typedef std::allocator_traits<_map_allocator_type> map_allocator_traits_type;
    typedef typename map_allocator_traits_type::pointer _map_pointer;
    typedef typename map_allocator_traits_type::const_pointer _map_const_pointer;

public:
    typedef DequeIterator<value_type, pointer, reference, _map_pointer, difference_type, Traits> iterator;
    typedef DequeIterator<value_type, const_pointer, const_reference, _map_const_pointer, difference_type, Traits> const_iterator;

private:
    iterator _mutable_iterator(const_iterator it)
    {
        return iterator(const_cast<_map_pointer>(it._iter), const_cast<pointer>(it._ptr));
    }

    iterator _mutable_iterator(iterator it)
    {
        return it;
    }

    T** map = nullptr;
    size_type first, map_size, map_begin, map_end;
    boost::compressed_pair<size_type, allocator_type> size_and_alloc;

    size_type& _size() noexcept { return size_and_alloc.first(); }
    const size_type& _size() const noexcept { return size_and_alloc.first(); }
    allocator_type& _alloc() noexcept { return size_and_alloc.second(); }
    const allocator_type& _alloc() const noexcept { return size_and_alloc.second(); }

    _map_allocator_type _map_alloc() noexcept 
    {
        return _map_allocator_type(_alloc());
    }

private:
    void _expand_map(size_type min_left_capacity, size_type min_right_capacity);

    void _construct(size_type min_capacity = 0);

    void _destroy_range(iterator b, iterator e)
    {
        if constexpr (std::is_trivially_destructible_v<T>) {
            (void)b;
            (void)e;
        }
        else {
            auto start = _mutable_iterator(b);
            for (auto i = start; i != e; ++i)
            {
                i->~T();
            }
        }
    }

    void _destroy()
    {
        _destroy_range(begin(), end());
        for (auto i = map_begin; i < map_end; ++i) {
            allocator_traits_type::deallocate(_alloc(), map[i], Traits::items_per_block);
        }
        map_allocator_traits_type::deallocate(_map_alloc(), map, map_size);
    }

    size_type capacity() const {
        return map_size ? map_size * Traits::items_per_block - 1 : 0;
    }

    bool _has_front_capacity() const
    {
        if (!first) return false;
        auto map_index = (first-1) / Traits::items_per_block;
        return map_begin <= map_index && map_index < map_end;
    }

    bool _has_back_capacity() const
    {
        auto n = first + _size();
        auto map_index = n / Traits::items_per_block;
        return map_begin <= map_index && map_index < map_end;
    }

    void _deque_empty();

    void _front_add_capacity();
    void _back_add_capacity();

    void _adjust_left();
    void _adjust_right();

protected:
    DequeBase()
    {
        _construct();
    }

    explicit DequeBase(size_type num_elements)
    {
        _construct(num_elements);
    }

    template<typename ForwardIterator>
    DequeBase(ForwardIterator f, ForwardIterator l)
        : DequeBase(std::distance(f, l))
    {
        _append_unchecked(f, l);
    }

    DequeBase(const DequeBase& rhs)
        : DequeBase(rhs.begin(), rhs.end())
    {
    }

    DequeBase(DequeBase&& rhs)
    {
        map = rhs.map;
        first = rhs.first;
        map_size = rhs.map_size;
        map_begin = rhs.map_begin;
        map_end = rhs.map_end;
        _size() = rhs._size();
        rhs.map = nullptr;
    }

    template<class InputIterator>
    void _append(InputIterator f, InputIterator l)
    {
        for (; f != l; ++f) {
            emplace_back(*f);
        }
    }

    template<class InputIterator>
    void _append_unchecked(InputIterator f, InputIterator l)
    {
        for (; f != l; ++f) {
            allocator_traits_type::construct(_alloc(), std::addressof(*end()), *f);
            ++_size();
        }
    }

public:
    void reserve_back(uint64_t pSize)
    {
        // XXX NYI
        (void)pSize;
    }

    void reserve_front(uint64_t pSize)
    {
        // XXX NYI
        (void)pSize;
    }

    iterator begin()
    {
        auto iter = &map[first / Traits::items_per_block];
        auto ptr = &(*iter)[first % Traits::items_per_block];
        return iterator(iter, ptr);
    }

    const_iterator begin() const
    {
        auto iter = &map[first / Traits::items_per_block];
        auto ptr = &(*iter)[first % Traits::items_per_block];
        return const_iterator(iter, ptr);
    }

    iterator end()
    {
        auto n = first + _size();
        auto iter = &map[n / Traits::items_per_block];
        auto ptr = &(*iter)[n % Traits::items_per_block];
        return iterator(iter, ptr);
    }

    const_iterator end() const
    {
        auto n = first + _size();
        auto iter = (_map_pointer) &map[n / Traits::items_per_block];
        auto ptr = &(*iter)[n % Traits::items_per_block];
        return const_iterator(iter, ptr);
    }

    void push_back(const value_type& x)
    {
        if (!_has_back_capacity()) {
            _back_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*end()), x);
        ++_size();
    }

    void push_back(value_type&& x)
    {
        if (!_has_back_capacity()) {
            _back_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*end()), std::move(x));
        ++_size();
    }

    template <class... Args>
    reference
    emplace_back(Args&&... args)
    {
        if (!_has_back_capacity()) {
            _back_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*end()), std::forward<Args>(args)...);
        ++_size();
        return *--end();
    }

    void push_front(const value_type& x)
    {
        if (!_has_front_capacity()) {
            _front_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*--begin()), x);
        ++_size();
        --first;
    }

    void push_front(value_type&& x)
    {
        if (!_has_front_capacity()) {
            _front_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*--begin()), std::move(x));
        ++_size();
        --first;
    }

    template <class... Args>
    reference
    emplace_front(Args&&... args)
    {
        if (!_has_front_capacity()) {
            _front_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*--begin()), std::forward<Args>(args)...);
        ++_size();
        --first;
        return *begin();
    }

    const T& front() const
    {
        return *begin();
    }

    T& front()
    {
        return *begin();
    }

    const T& back() const
    {
        return *--end();
    }

    T& back()
    {
        return *--end();
    }

    void pop_front()
    {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            begin()->~T();
        }
        ++first;
        if (!--_size()) {
            _deque_empty();
        }
        else if constexpr (Traits::left_policy != DequeDetail::DEQUE_KEEP_ALL) {
            if (!(first % Traits::items_per_block)) {
                _adjust_left();
            }
        }
    }

    void pop_back()
    {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            (--end())->~T();
        }
        if (!--_size()) {
            _deque_empty();
        }
        else if constexpr (Traits::right_policy != DequeDetail::DEQUE_KEEP_ALL) {
            if (!((first + _size()) % Traits::items_per_block)) {
                _adjust_right();
            }
        }
    }

    size_type size() const
    {
        return _size();
    }

    bool empty() const
    {
        return !_size();
    }

    void clear()
    {
        _destroy_range(begin(), end());
        _size() = 0;
        _deque_empty();
    }

    ~DequeBase()
    {
        if (map) {
            _destroy();
        }
    }
};


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_construct(size_type  min_capacity)
{
    map_size = std::max<size_type>(Traits::initial_map_size, Gossamer::roundUpToNextPowerOf2(min_capacity / Traits::items_per_block + 1));
    map = map_allocator_traits_type::allocate(_map_alloc(), map_size);
    auto initial_blocks = (min_capacity + Traits::items_per_block - 1) / Traits::items_per_block;
#ifdef GOSS_DEBUG_DEQUE
    std::memset(map, 0, sizeof(map[0]) * map_size);
#endif
    auto max_map_begin = map_size - 1 - initial_blocks;
    switch (Traits::left_policy) {
    case DequeDetail::DEQUE_KEEP_NONE:
        map_begin = 0;
        break;
    case DequeDetail::DEQUE_KEEP_ONE:
        map_begin = std::min<size_type>(max_map_begin, 1);
        break;
    case DequeDetail::DEQUE_KEEP_ALL:
        switch (Traits::right_policy) {
        case DequeDetail::DEQUE_KEEP_NONE:
            map_begin = std::min(max_map_begin, map_size - 1);
            break;
        case DequeDetail::DEQUE_KEEP_ONE:
            map_begin = std::min(max_map_begin, map_size - 2);
            break;
        case DequeDetail::DEQUE_KEEP_ALL:
            map_begin = std::min(max_map_begin, map_size >> 1);
            break;
        }
    }
    map_end = map_begin + initial_blocks;
    for (auto i = map_begin; i < map_end; ++i) {
        map[i] = allocator_traits_type::allocate(_alloc(), Traits::items_per_block);
    }
    first = map_begin * Traits::items_per_block;
    _size() = 0;
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T,Allocator,Traits>::_deque_empty()
{
    if constexpr (Traits::left_policy == DequeDetail::DEQUE_KEEP_NONE || Traits::left_policy == DequeDetail::DEQUE_KEEP_ONE) {
        auto desired_left = Traits::left_policy == DequeDetail::DEQUE_KEEP_ONE ? 1 : 0;
        int64_t offset = desired_left - map_begin;
        if (offset) {
            std::memmove(&map[desired_left], &map[map_begin], (map_end - map_begin) * sizeof(map[0]));
            map_begin += offset;
            map_end += offset;
#ifdef GOSS_DEBUG_DEQUE
            std::memset(&map[0], 0, map_begin * sizeof(map[0]));
            std::memset(&map[map_end], 0, (map_size - map_end) * sizeof(map[0]));
#endif
        }
        first = desired_left * Traits::items_per_block;
    }
    else if constexpr (Traits::right_policy == DequeDetail::DEQUE_KEEP_NONE || Traits::right_policy == DequeDetail::DEQUE_KEEP_ONE) {
        auto desired_right = map_size - (Traits::right_policy == DequeDetail::DEQUE_KEEP_ONE ? 1 : 0) - 1;
        int64_t offset = desired_right - map_end;

        if (map_end != desired_right) {
            std::memmove(&map[map_begin + offset], &map[map_begin], (map_end - map_begin) * sizeof(map[0]));
            map_begin += offset;
            map_end += offset;
#ifdef GOSS_DEBUG_DEQUE
            std::memset(&map[0], 0, map_begin * sizeof(map[0]));
            std::memset(&map[map_end], 0, (map_size - map_end) * sizeof(map[0]));
#endif
        }
        first = desired_right * Traits::items_per_block;
    }
    else {
        // If both left and right policies are KEEP_ALL, we don't really have any clues as to
        // what the user wants here.
        first -= first % Traits::items_per_block;
    }
    _size() = 0;
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_expand_map(size_type min_left_capacity, size_type min_right_capacity)
{
    auto deck_offset = first % Traits::items_per_block;

    auto old_map_size = map_size;

    if constexpr (Traits::left_policy == DequeDetail::DEQUE_KEEP_ONE) ++min_left_capacity;
    if constexpr (Traits::right_policy  == DequeDetail::DEQUE_KEEP_ONE) ++min_right_capacity;
    size_type new_map_size = std::max(old_map_size * 2, Gossamer::roundUpToNextPowerOf2(old_map_size + min_left_capacity + min_right_capacity));

    T** new_map = map_allocator_traits_type::allocate(_map_alloc(), new_map_size);
#ifdef GOSS_DEBUG_DEQUE
    std::memset(new_map, 0, new_map_size * sizeof(new_map[0]));
#endif

    size_type map_start_position = 0;
    switch (Traits::left_policy) {
    case DequeDetail::DEQUE_KEEP_NONE:
    case DequeDetail::DEQUE_KEEP_ONE:
        map_start_position = min_left_capacity;
        break;
    case DequeDetail::DEQUE_KEEP_ALL:
        switch (Traits::right_policy) {
        case DequeDetail::DEQUE_KEEP_NONE:
            map_start_position = new_map_size + map_begin - min_right_capacity - 1 - map_end;
            break;
        case DequeDetail::DEQUE_KEEP_ONE:
            map_start_position = new_map_size + map_begin - min_right_capacity - 2 - map_end;
            break;
        case DequeDetail::DEQUE_KEEP_ALL:
            map_start_position = map_begin + min_left_capacity;
            break;
        }
        break;
    }

    auto old_map = map;

    std::memcpy(&new_map[map_start_position], &old_map[map_begin], sizeof(map[0]) * (map_end - map_begin));

    map = new_map;
    map_size = new_map_size;
    auto map_offset = map_start_position - map_begin;
    first += map_offset * Traits::items_per_block;
    map_allocator_traits_type::deallocate(_map_alloc(), old_map, old_map_size);
    map_begin += map_offset;
    map_end += map_offset;
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_front_add_capacity()
{
    if (!map_begin) {
        size_type desired_right = 0;
        switch (Traits::right_policy) {
        case DequeDetail::DEQUE_KEEP_NONE:
        {
            desired_right = map_size - 1;
            break;
        }
        case DequeDetail::DEQUE_KEEP_ONE:
        {
            desired_right = map_size - 2;
            break;
        }
        case DequeDetail::DEQUE_KEEP_ALL:
        {
            desired_right = map_size - 1;
            break;
        }
        }
        difference_type desired_left = desired_right + map_begin - map_end - 1;
        if (desired_left < 0) {
            _expand_map(1, 0);
        }
        else {
            auto move_amount = desired_right - map_end;
            auto desired_left = map_begin + move_amount;
            std::memmove(&map[desired_left], &map[map_begin], (map_end - map_begin) * sizeof(map[0]));
            map_begin += move_amount;
            map_end += move_amount;
            first += Traits::items_per_block * move_amount;
        }
    }
    map[--map_begin] = allocator_traits_type::allocate(_alloc(), Traits::items_per_block);
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_back_add_capacity()
{
    if (map_end >= map_size - 1) {
        size_type desired_left = 0;
        switch (Traits::left_policy) {
        case DequeDetail::DEQUE_KEEP_NONE:
        {
            desired_left = 0;
            break;
        }
        case DequeDetail::DEQUE_KEEP_ONE:
        {
            desired_left = 1;
            break;
        }
        case DequeDetail::DEQUE_KEEP_ALL:
        {
            desired_left = map_begin > 0 ? map_begin - 1 : 0;
            break;
        }
        }
        size_type desired_right = desired_left + map_end - map_begin + 1;
        if (desired_right > map_size - 1) {
            _expand_map(0, 1);
        }
        else {
            auto move_amount = map_begin - desired_left;
            std::memmove(&map[desired_left], &map[map_begin], (map_end - map_begin) * sizeof(map[0]));
            map_begin -= move_amount;
            map_end -= move_amount;
            first -= Traits::items_per_block * move_amount;
        }
    }
    map[map_end] = allocator_traits_type::allocate(_alloc(), Traits::items_per_block);
    ++map_end;
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_adjust_left()
{
    // This is only called if the left_policy is not KEEP_ALL and there are
    // one or more unused blocks at the left.
    constexpr auto keep_left = Traits::left_policy == DequeDetail::DEQUE_KEEP_ONE ? 1 : 0;
    auto first_active_deck = first / Traits::items_per_block;
    size_type desired_left = first_active_deck < map_begin + keep_left ? map_begin : first_active_deck - keep_left;
#ifdef GOSS_DEBUG_DEQUE
    for (auto i = map_begin; i < first_active_deck; ++i) {
        std::memset(map[i], 0xCD, sizeof(T) * Traits::items_per_block);
    }
#endif
    for (size_type i = map_begin; i < desired_left; ++i) {
        if (Traits::right_policy == DequeDetail::DEQUE_KEEP_ALL && map_end < map_size - 1) {
#ifdef GOSS_DEBUG_DEQUE
            std::swap(map[map_end++], map[i]);
#else
            map[map_end++] = map[i];
#endif
        }
        else {
            allocator_traits_type::deallocate(_alloc(), map[i], Traits::items_per_block);
#ifdef GOSS_DEBUG_DEQUE
            map[i] = nullptr;
#endif
        }
    }
    map_begin = desired_left;
}

template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_adjust_right()
{
    // This is only called if the right_policy is not KEEP_ALL and there are
    // one or more unused blocks at the right.
    constexpr auto keep_right = Traits::right_policy == DequeDetail::DEQUE_KEEP_ONE ? 1 : 0;
    auto first_inactive_deck = (first + _size() + Traits::items_per_block - 1) / Traits::items_per_block;
    size_type desired_right = std::min(first_inactive_deck + keep_right, map_end);
#ifdef GOSS_DEBUG_DEQUE
    for (auto i = first_inactive_deck; i < map_end; ++i) {
        std::memset(map[i], 0xCD, sizeof(T) * Traits::items_per_block);
    }
#endif
    for (difference_type i = map_end - 1; i >= desired_right; --i) {
        if (Traits::left_policy == DequeDetail::DEQUE_KEEP_ALL && map_begin > 0) {
#ifdef GOSS_DEBUG_DEQUE
            std::swap(map[--map_begin], map[i]);
#else
            map[--map_begin] = map[i];
#endif
        }
        else {
            allocator_traits_type::deallocate(_alloc(), map[i], Traits::items_per_block);
#ifdef GOSS_DEBUG_DEQUE
            map[i] = nullptr;
#endif
        }
    }
    map_end = desired_right;
}


template<typename T, typename Allocator = std::allocator<T>>
class Deque : public DequeBase<T, Allocator, DequeTraitsDefault<T>>
{
public:
    Deque() { }

    template <class InputIterator>
    Deque(InputIterator f, InputIterator l)
        : DequeBase(f, l)
    {
    }

    Deque(const Deque& rhs)
        : DequeBase(rhs)
    {
    }

    Deque(Deque&& rhs)
        : DequeBase(rhs)
    {
    }
};

template<typename T, typename Allocator = std::allocator<T>>
class Stack : public DequeBase<T, Allocator, DequeTraitsStack<T>>
{
public:
    Stack() { }

    template <class InputIterator>
    Stack(InputIterator f, InputIterator l)
        : DequeBase(f, l)
    {
    }

    Stack(const Stack& rhs)
        : DequeBase(rhs)
    {
    }

    Stack(Stack&& rhs)
        : DequeBase(rhs)
    {
    }
};

template<typename T, typename Allocator = std::allocator<T>>
class Queue : public DequeBase<T, Allocator, DequeTraitsQueue<T>>
{
public:
    Queue() { }

    template <class InputIterator>
    Queue(InputIterator f, InputIterator l)
        : DequeBase(f, l)
    {
    }

    Queue(const Queue& rhs)
        : DequeBase(rhs)
    {
    }

    Queue(Queue&& rhs)
        : DequeBase(rhs)
    {
    }
};


#endif // DEQUE_HH