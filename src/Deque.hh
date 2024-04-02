// Copyright (c) 2024 Andrew J. Bromage.
//
// Licensed under the CSIRO Open Source Software License Agreement;
// you may not use this file except in compliance with the License.
// Please see the file LICENSE, included with this distribution.
//
#ifndef DEQUE_HH
#define DEQUE_HH

#ifndef BOOST_COMPRESSED_PAIR_HPP_INCLDED
#include <boost/compressed_pair.hpp>
#define BOOST_COMPRESSED_PAIR_HPP_INCLDED
#endif


namespace DequeDetail {
    constexpr uint64_t _deque_items_per_block(size_t block_size, size_t size) {
        return (size * 16 <= block_size) ? block_size / size : 16;
    }
}

struct DequeTraitsBase {
    enum BlockPolicy {
        DEQUE_KEEP_NONE = 0,
        DEQUE_KEEP_ONE = 1,
        DEQUE_KEEP_ALL = 2
    };

    static const unsigned initial_map_size = 8;
    static const uint64_t block_size = 1024;
    static const BlockPolicy left_policy = DEQUE_KEEP_ONE;
    static const BlockPolicy right_policy = DEQUE_KEEP_ONE;
};


template<typename T>
struct DequeTraitsDefault : public DequeTraitsBase {
    static const uint64_t items_per_block = DequeDetail::_deque_items_per_block(block_size, sizeof(T));
};


template<typename T>
struct DequeTraitsQueue : public DequeTraitsDefault<T> {
    static const BlockPolicy left_policy = DEQUE_KEEP_NONE;
    static const BlockPolicy right_policy = DEQUE_KEEP_ALL;
};

template<typename T>
struct DequeTraitsStack : public DequeTraitsDefault<T> {
    static const BlockPolicy left_policy = DEQUE_KEEP_NONE;
    static const BlockPolicy right_policy = DEQUE_KEEP_ALL;
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
            _ptr = &_iter[Traits::items_per_block];
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
            n += _ptr - *_iter;
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
        return lhs == rhs ? 0 : (lhs._iter - rhs._iter) * Traits::iterms_per_block + (lhs._ptr - *lhs._iter) - (rhs._ptr - *rhs._iter);
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

    explicit DequeIterator(MapPointer iter, Pointer ptr) noexcept
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
    typedef typename std::allocator_traits<_map_allocator_type>::pointer _map_pointer;
    typedef typename std::allocator_traits<_map_allocator_type>::const_pointer _map_const_pointer;

public:
    typedef DequeIterator<value_type, pointer, reference, _map_pointer, difference_type, Traits> iterator;
    typedef DequeIterator<value_type, const_pointer, const_reference, _map_const_pointer, difference_type, Traits> const_iterator;

private:
    T** map;
    size_t first, map_size;
    boost::compressed_pair<size_type, allocator_type> size_and_alloc;

    size_type& _size() noexcept { return size_and_alloc.first(); }
    const size_type& _size() const noexcept { return size_and_alloc.first(); }
    allocator_type& _alloc() noexcept { return size_and_alloc.second(); }
    const allocator_type& _alloc() const noexcept { return size_and_alloc.second(); }

    _map_allocator_type _map_allocator() noexcept 
    {
        return _map_allocator_type(_alloc());
    }

private:
    void _expand_map(size_t min_left_capacity, size_t min_right_capacity);

    void _construct()
    {
        map_size = Traits::initial_map_size;
        map_allocator_traits_type::allocate(_map_alloc(), map_size);
        std::memset(map, 0, sizeof(map[0]) * map_size);
        switch constexpr (Traits::left_policy) {
        case DequeTraitsBase::DEQUE_KEEP_NONE:
            first = 0;
            break;
        case DequeTraitsBase::DEQUE_KEEP_ONE:
            first = 1;
            break;
        case DequeTraitsBase::DEQUE_KEEP_ALL:
            switch constexpr (Traits::right_policy) {
            case DequeTraitsBase::DEQUE_KEEP_NONE:
                first = init_map_size;
                break;
            case DequeTraitsBase::DEQUE_KEEP_ONE:
                first = init_map_size - 1;
                break;
            case DequeTraitsBase::DEQUE_KEEP_ALL:
                first = init_map_size >> 1;
                break;
            }
        }
    }

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
        std::allocator_traits<_map_allocator_type>::deallocate(_map_allocator(), map, map_size);
    }

    size_t capacity() const {
        return map_size ? map_size * Traits::items_per_block - 1 : 0;
    }

    bool _has_front_capacity() const
    {
        return first > 0;
    }

    bool _has_back_capacity() const
    {
        return first > 0;
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

public:
    void reserve(uint64_t pSize)
    {
        _back_add_capacity(pSize);
    }

    iterator begin()
    {
        auto iter = &map[first / Traits::items_per_block];
        auto ptr = &iter[first % Traits::items_per_block];
        return iterator(iter, ptr);
    }

    const_iterator begin() const
    {
        auto iter = &map[first / Traits::items_per_block];
        auto ptr = &iter[first % Traits::items_per_block];
        return const_iterator(iter, ptr);
    }

    iterator end()
    {
        auto n = first + _size();
        auto iter = &map[n / Traits::items_per_block];
        auto ptr = &iter[n % Traits::items_per_block];
        return iterator(iter, ptr);
    }

    const_iterator end() const
    {
        auto n = first + _size();
        auto iter = (_map_pointer) &map[n / Traits::items_per_block];
        auto ptr = &iter[n % Traits::items_per_block];
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
        --_start;
    }

    void push_front(value_type&& x)
    {
        if (!_has_front_capacity()) {
            _front_add_capacity();
        }
        allocator_traits_type::construct(_alloc(), std::addressof(*--begin()), std::move(x));
        ++_size();
        --_start;
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
        --_start;
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
        return *-end();
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
        else if constexpr (Traits::left_policy != DequeTraitsBase::DEQUE_KEEP_ALL) {
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
        else if constexpr (Traits::right_policy != DequeTraitsBase::DEQUE_KEEP_ALL) {
            if (!((first + size) % Traits::items_per_block)) {
                _adjust_right();
            }
        }
    }

    size_t size() const
    {
        return _size();
    }

    ~DequeBase()
    {
        _destroy();
    }
};

template <typename T, typename Allocator, typename Traits>
void
DequeBase<T,Allocator,Traits>::_deque_empty()
{
    if constexpr (Traits::left_policy == DequeTraitsBase::DEQUE_KEEP_NONE || Traits::left_policy == DequeTraitsBase::DEQUE_KEEP_ONE) {
        if (!map[0]) {
            size_t s, e;
            for (; s < map_size; ++s) {
                if (map[s]) break;
            }
            for (e = s; e < map_size; ++e) {
                if (!map[e]) break;
            }

            if (s) {
                std::memmove(&map[s], &map[e], (e - s) / sizeof(map[0]));
                std::memset(&map[e - s], 0, map_size + s - e);
            }
            size_t f = Traits::left_policy == DequeTraitsBase::DEQUE_KEEP_ONE && map[0] ? 1 : 0;
            first = f * Traits::items_per_block;
            if (constexpr Traits::right_policy != DequeTraitsBase::DEQUE_KEEP_ALL) {
            }
        }
    }
    else if constexpr (Traits::right_policy == DequeTraitsBase::DEQUE_KEEP_NONE || Traits::right_policy == DequeTraitsBase::DEQUE_KEEP_ONE) {
        if (!map[map_size-1]) {
            size_t s, e;
            for (e = map_size; e > 0; --e) {
                if (map[e-1]) break;
            }
            for (s = e; s > 0; --s) {
                if (!map[s-1]) break;
            }

            if (e != map_size) {
                std::memmove(&map[map_size + s - e], &map[e], (e - s) / sizeof(map[0]));
                std::memset(&map[0], 0, map_size + s - e);
            }
            size_t f = Traits::left_policy == DequeTraitsBase::DEQUE_KEEP_ONE && map[map_size-1] ? map_size-1 : map_size;
            first = f * Traits::items_per_block;
        }
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
DequeBase<T, Allocator, Traits>::_expand_map(size_t min_left_capacity, size_t min_right_capacity)
{
    auto map_begin = first / Traits::items_per_block;
    auto map_end = (first + _size() - 1) / Traits::items_per_block;
    auto deck_offset = first % Traits::items_per_block;

    auto old_map_size = map_size;

    if constexpr (Traits::left_policy == DequeTraitsBase::DEQUE_KEEP_ONE) ++min_left_capacity;
    if constexpr (Traits::right_pollicyh  == DequeTraitsBase::DEQUE_KEEP_ONE) ++min_right_capacity;
    size_t new_map_size = std::max(new_map_size * 2, Gossamer::roundUpToNextPowerOf2(new_map_size + min_left_capacity + min_right_capacity));

    T** new_map = std::allocator_traits<_map_allocator_type>::allocate(_alloc(), new_map_size);
    std::memset(&new_map, 0, new_map_size * sizeof(new_map[0]));

    size_t map_start_position = 0;
    switch constexpr (Traits::left_policy) {
    case DequeTraitsBase::DEQUE_KEEP_NONE:
    case DequeTraitsBase::DEQUE_KEEP_ONE:
        map_start_position = min_left_capacity;
        break;
    case DequeTraitsBase::DEQUE_KEEP_ALL:
        switch constexpr (Traits::right_policy) {
        case DequeTraitsBase::DEQUE_KEEP_NONE:
        case DequeTraitsBase::DEQUE_KEEP_ONE:
            map_start_position = new_map_size - min_right_capacity;
            break;
        case DequeTraitsBase::DEQUE_KEEP_ALL:
            first = std::min(min_left_capacity, map_begin);
            break;
        }
        break;
    }

    std::memcpy(&new_map[map_start_position], &map[map_begin], map_end - map_begin);

    auto old_map = map;
    map = new_map;
    map_size = new_map_size;
    first = new_map_size + deck_offset;
    std::allocator_traits<_map_allocator_type>::deallocate(_alloc(), old_map, old_map_size);
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_front_add_capacity()
{
    auto map_begin = first / Traits::items_per_block;
    auto map_end = (first + _size() - 1) / Traits::items_per_block + 1;
    if (map_begin == 0) {
        if (map_end == map_size) {
            _expand_map(1, 0);
            map_begin = first / Traits::items_per_block;
            map_end = (first + _size() - 1) / Traits::items_per_block + 1;
        }
        else {
            std::memmove(&map[1], &map[0], (map_end - map_begin) * sizeof(map[0]));
            ++map_begin;
            ++map_end;
            first += Traits::items_per_block;
        }
    }
}


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_back_add_capacity()
{
    auto map_begin = first / Traits::items_per_block;
    auto map_end = (first + _size() - 1) / Traits::items_per_block + 1;
    if (map_end == map_size) {
        if (map_begin == 0) {
            _expand_map(0, 1);
            map_begin = first / Traits::items_per_block;
            map_end = (first + _size() - 1) / Traits::items_per_block + 1;
        }
        else {
            std::memmove(&map[map_size - map_begin - 1], &map[map_size - map_begin - 1], (map_end - map_begin) * sizeof(map[0]));
            --map_begin;
            --map_end;
            first -= Traits::items_per_block;
        }
    }
}


#if 0
    allocator_type& a = _alloc();
    if (_back_capacity() >= Traits::items_per_block) {
        first += Traits::items_per_block();
        auto pt = last_block +
            pointer __pt = __base::__map_.back();
        __base::__map_.pop_back();
        __base::__map_.push_front(__pt);
    }
    // Else if __base::__map_.size() < __base::__map_.capacity() then we need to allocate 1 buffer
    else if (__base::__map_.size() < __base::__map_.capacity())
    {   // we can put the new buffer into the map, but don't shift things around
        // until all buffers are allocated.  If we throw, we don't need to fix
        // anything up (any added buffers are undetectible)
        if (__base::__map_.__front_spare() > 0)
            __base::__map_.push_front(__alloc_traits::allocate(__a, __base::__block_size));
        else
        {
            __base::__map_.push_back(__alloc_traits::allocate(__a, __base::__block_size));
            // Done allocating, reorder capacity
            pointer __pt = __base::__map_.back();
            __base::__map_.pop_back();
            __base::__map_.push_front(__pt);
        }
        __base::__start_ = __base::__map_.size() == 1 ?
            __base::__block_size / 2 :
            __base::__start_ + __base::__block_size;
    }
    // Else need to allocate 1 buffer, *and* we need to reallocate __map_.
    else
    {
        __split_buffer<pointer, typename __base::__pointer_allocator&>
            __buf(max<size_type>(2 * __base::__map_.capacity(), 1),
                0, __base::__map_.__alloc());

        typedef __allocator_destructor<_Allocator> _Dp;
        unique_ptr<pointer, _Dp> __hold(
            __alloc_traits::allocate(__a, __base::__block_size),
            _Dp(__a, __base::__block_size));
        __buf.push_back(__hold.get());
        __hold.release();

        for (typename __base::__map_pointer __i = __base::__map_.begin();
            __i != __base::__map_.end(); ++__i)
            __buf.push_back(*__i);
        _VSTD::swap(__base::__map_.__first_, __buf.__first_);
        _VSTD::swap(__base::__map_.__begin_, __buf.__begin_);
        _VSTD::swap(__base::__map_.__end_, __buf.__end_);
        _VSTD::swap(__base::__map_.__end_cap(), __buf.__end_cap());
        __base::__start_ = __base::__map_.size() == 1 ?
            __base::__block_size / 2 :
            __base::__start_ + __base::__block_size;
    }
#endif


template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_adjust_left()
{
}

template <typename T, typename Allocator, typename Traits>
void
DequeBase<T, Allocator, Traits>::_adjust_right()
{
}

template<typename T, typename Allocator = std::allocator<T>>
class Deque : public DequeBase<T, Allocator, DequeTraitsDefault<T>>
{
public:
    Deque() { }
};

template<typename T, typename Allocator = std::allocator<T>>
class Stack : public DequeBase<T, Allocator, DequeTraitsStack<T>>
{
public:
    Stack() { }
};

template<typename T, typename Allocator = std::allocator<T>>
class Queue : public DequeBase<T, Allocator, DequeTraitsQueue<T>>
{
public:
    Queue() { }
};

#endif // DEQUE_HH
