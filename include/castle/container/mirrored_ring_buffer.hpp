#ifndef CASTLE_CONTAINER_MIRRORED_RING_BUFFER_HPP
#define CASTLE_CONTAINER_MIRRORED_RING_BUFFER_HPP

#include "castle/atomic/atomic.hpp"
#include "castle/container/array_view.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"

namespace castle
{
namespace container
{

// Fixed-capacity SPSC ring buffer with a physically mirrored storage area.
//
// The buffer owns 2*N elements. For every logical slot currently in
// [read_index, write_index), its mirror (offset by N) contains the same
// value. This makes the logical sequence contiguous from any starting slot:
// [start, start + size()) is always a single range. Slots outside that range
// (already consumed, not yet written) may have a stale mirror; this is
// harmless because commit_write() unconditionally re-establishes both copies
// from what it just wrote before publishing, regardless of prior contents.
//
// One producer may call write_*()/push_*() and one consumer may call
// read_*()/pop_*(). Each side only ever stores to its own index, so that
// index is read back with memory_order_relaxed; the other side's index is
// read with memory_order_acquire to observe what it published (new data for
// the consumer, reclaimed space for the producer). A stale acquire read can
// only make a side see less room/data than truly exists, never more, so this
// is safe. front()/back()/operator[]/data() do not perform this acquire load
// themselves and must only be called once size()/empty()/end()/read_reserve()
// (or an equivalent) has established that at least one element is available.
//
// The reservation API is intended for DMA/IO use. A reservation must be
// committed by the same side that created it before another reservation is made
// on that side. clear() is a quiescent operation and must not race with either
// producer or consumer activity.
template <typename T, size_type N>
class mirrored_ring_buffer
{
    static_assert(N > 0U, "mirrored_ring_buffer capacity must be non-zero");
    static_assert((N <= (static_cast<castle::size_type>(-1) / 2U)),
                  "mirrored_ring_buffer capacity is too large");
    static_assert(meta::is_trivially_copyable<T>::value,
                  "mirrored_ring_buffer<T,N> requires T to be trivially copyable");
    static_assert(meta::is_trivially_destructible<T>::value,
                  "mirrored_ring_buffer<T,N> requires T to be trivially destructible");

public:
    using value_type      = T;
    using size_type       = castle::size_type;
    using difference_type = castle::difference_type;
    using reference       = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer         = T*;
    using const_pointer   = CASTLE_CONST T*;
    using iterator        = T*;
    using const_iterator  = CASTLE_CONST T*;

    static CASTLE_CONSTEXPR size_type static_capacity = N;

    mirrored_ring_buffer() CASTLE_NOEXCEPT
        : storage_{}, read_index_(0U), write_index_(0U)
    {
    }

    mirrored_ring_buffer(CASTLE_CONST mirrored_ring_buffer&)            CASTLE_DELETE;
    mirrored_ring_buffer& operator=(CASTLE_CONST mirrored_ring_buffer&) CASTLE_DELETE;
    mirrored_ring_buffer(mirrored_ring_buffer&&)                        CASTLE_DELETE;
    mirrored_ring_buffer& operator=(mirrored_ring_buffer&&)             CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    static CASTLE_CONSTEXPR size_type max_size() CASTLE_NOEXCEPT { return N; }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type read_index = read_index_.load(memory_order_acquire);
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_acquire);
        return write_index - read_index;
    }

    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size() == 0U;
    }

    bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return available() == 0U;
    }

    size_type available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return N - size();
    }

    // Clears the logical contents. The operation is intended for a quiescent
    // buffer; it must not race with producer or consumer operations.
    void clear() CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_acquire);
        read_index_.store(write_index, memory_order_release);
    }

    // Returns a contiguous writable reservation. Because of the mirrored
    // storage, the complete free region is contiguous even when the logical
    // write index is near the physical end of the first half.
    array_view<T> write_reserve(size_type max_reserve_size = N) CASTLE_NOEXCEPT
    {
        size_type write_index{0U};
        CASTLE_CONST size_type free_size = writable_size(write_index);
        CASTLE_CONST size_type reserve_size =
            (max_reserve_size < free_size) ? max_reserve_size : free_size;
        return array_view<T>(physical_data(write_index), reserve_size);
    }

    // Returns the whole currently available contiguous writable region when it
    // contains at least min_reserve_size elements; otherwise returns an empty
    // view at the current write position.
    array_view<T> write_reserve_optimal(size_type min_reserve_size = 1U) CASTLE_NOEXCEPT
    {
        size_type write_index{0U};
        CASTLE_CONST size_type free_size = writable_size(write_index);

        if (free_size < min_reserve_size)
        {
            return array_view<T>(physical_data(write_index), 0U);
        }

        return array_view<T>(physical_data(write_index), free_size);
    }

    // Publishes a previously reserved writable region.
    // Returns invalid_argument if reservation is not the currently owned write
    // reservation, full if it is larger than the free region (committing it
    // would overrun data the consumer has not read yet), otherwise ok.
    status write_commit(CASTLE_CONST array_view<T>& reservation) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_relaxed);

        if (reservation.data() != physical_data(write_index))
        {
            return status::invalid_argument;
        }

        if (reservation.empty())
        {
            return status::ok;
        }

        if (reservation.size() > free_size_from(write_index))
        {
            return status::full;
        }

        commit_write(write_index, reservation.size());
        return status::ok;
    }

    // Returns a contiguous readable reservation. As with write_reserve(), the
    // mirrored second half makes a wrapped logical range physically contiguous.
    array_view<T> read_reserve(size_type max_reserve_size = N) CASTLE_NOEXCEPT
    {
        size_type read_index{0U};
        CASTLE_CONST size_type readable = readable_size(read_index);
        CASTLE_CONST size_type reserve_size =
            (max_reserve_size < readable) ? max_reserve_size : readable;
        return array_view<T>(physical_data(read_index), reserve_size);
    }

    // Commits a previously reserved readable region.
    // Returns invalid_argument if reservation is not the currently owned read
    // reservation, empty if it is larger than the readable region, otherwise ok.
    status read_commit(CASTLE_CONST array_view<T>& reservation) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type read_index = read_index_.load(memory_order_relaxed);

        if (reservation.data() != physical_data(read_index))
        {
            return status::invalid_argument;
        }

        if (reservation.empty())
        {
            return status::ok;
        }

        if (reservation.size() > readable_size_from(read_index))
        {
            return status::empty;
        }

        commit_read(read_index, reservation.size());
        return status::ok;
    }

    status push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        size_type write_index{0U};
        if (writable_size(write_index) == 0U)
        {
            return status::full;
        }
        *physical_data(write_index) = value;
        commit_write(write_index, 1U);
        return status::ok;
    }

    status push(value_type&& value) CASTLE_NOEXCEPT
    {
        size_type write_index{0U};
        if (writable_size(write_index) == 0U)
        {
            return status::full;
        }
        *physical_data(write_index) = castle::move(value);
        commit_write(write_index, 1U);
        return status::ok;
    }

    status pop(value_type& out) CASTLE_NOEXCEPT
    {
        size_type read_index{0U};
        if (readable_size(read_index) == 0U)
        {
            return status::empty;
        }
        out = *physical_data(read_index);
        commit_read(read_index, 1U);
        return status::ok;
    }

    status pop() CASTLE_NOEXCEPT
    {
        size_type read_index;
        if (readable_size(read_index) == 0U)
        {
            return status::empty;
        }
        commit_read(read_index, 1U);
        return status::ok;
    }

    status peek(size_type index, value_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type read_index;
        if (index >= readable_size(read_index))
        {
            return status::out_of_range;
        }
        out = *(physical_data(read_index) + index);
        return status::ok;
    }

    size_type push_bulk(CASTLE_CONST value_type* src, size_type max_count) CASTLE_NOEXCEPT
    {
        if (src == nullptr)
        {
            return 0U;
        }

        size_type write_index;
        CASTLE_CONST size_type free_size = writable_size(write_index);
        CASTLE_CONST size_type count = (max_count < free_size) ? max_count : free_size;

        pointer destination = physical_data(write_index);
        for (size_type i = 0U; i < count; ++i)
        {
            destination[i] = src[i];
        }

        commit_write(write_index, count);
        return count;
    }

    size_type pop_bulk(value_type* dst, size_type max_count) CASTLE_NOEXCEPT
    {
        if (dst == nullptr)
        {
            return 0U;
        }

        size_type read_index;
        CASTLE_CONST size_type readable = readable_size(read_index);
        CASTLE_CONST size_type count = (max_count < readable) ? max_count : readable;

        pointer source = physical_data(read_index);
        for (size_type i = 0U; i < count; ++i)
        {
            dst[i] = source[i];
        }

        commit_read(read_index, count);
        return count;
    }

    reference operator[](size_type index) CASTLE_NOEXCEPT
    {
        return *(begin() + index);
    }

    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *(begin() + index);
    }

    reference front() CASTLE_NOEXCEPT { return *begin(); }
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }
    reference back() CASTLE_NOEXCEPT { return *(end() - 1); }
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return *(end() - 1); }

    // begin()/end() expose the logical sequence as one random-access range.
    // end() may be one-past the second physical copy when the buffer is full.
    // read_index_ is only ever stored by the consumer, so begin() only needs
    // a relaxed self-load; end() performs the acquire load of write_index_
    // needed to observe newly published elements.
    iterator begin() CASTLE_NOEXCEPT
    {
        return physical_data(read_index_.load(memory_order_relaxed));
    }

    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return physical_data(read_index_.load(memory_order_relaxed));
    }

    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    iterator end() CASTLE_NOEXCEPT
    {
        size_type read_index;
        CASTLE_CONST size_type count = readable_size(read_index);
        return physical_data(read_index) + count;
    }

    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type read_index;
        CASTLE_CONST size_type count = readable_size(read_index);
        return physical_data(read_index) + count;
    }

    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }


    pointer data() CASTLE_NOEXCEPT { return begin(); }
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

private:
    static CASTLE_CONSTEXPR size_type physical_index(size_type logical_index) CASTLE_NOEXCEPT
    {
        return meta::is_power_of_two<N>::value
                   ? (logical_index & (N - 1U))
                   : (logical_index % N);
    }

    pointer physical_data(size_type logical_index) CASTLE_NOEXCEPT
    {
        return storage_ + physical_index(logical_index);
    }

    const_pointer physical_data(size_type logical_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return storage_ + physical_index(logical_index);
    }

    // Free space as seen from a given (already read) write_index. Acquires
    // read_index_ so reclaimed space becomes visible before it is reused.
    size_type free_size_from(size_type write_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type read_index = read_index_.load(memory_order_acquire);
        return N - (write_index - read_index);
    }

    // Producer-side query: relaxed self-load of write_index_ (out param, only
    // the producer stores it) plus an acquire load of read_index_ to observe
    // space the consumer has reclaimed.
    size_type writable_size(size_type& write_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        write_index = write_index_.load(memory_order_relaxed);
        return free_size_from(write_index);
    }

    // Readable count as seen from a given (already read) read_index. Acquires
    // write_index_ so newly published data becomes visible before it is read.
    size_type readable_size_from(size_type read_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_acquire);
        return write_index - read_index;
    }

    // Consumer-side query: relaxed self-load of read_index_ (out param, only
    // the consumer stores it) plus an acquire load of write_index_ to observe
    // data the producer has published.
    size_type readable_size(size_type& read_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        read_index = read_index_.load(memory_order_relaxed);
        return readable_size_from(read_index);
    }

    void commit_write(size_type write_index, size_type count) CASTLE_NOEXCEPT
    {
        mirror_range(write_index, count);
        write_index_.store(write_index + count, memory_order_release);
    }

    // Freed slots need no mirror resync here: a reader may freely modify its
    // reservation, but the slots it just released only ever become valid
    // again through a future write, and commit_write() re-mirrors whatever it
    // writes from scratch, independent of any prior state. Advancing the
    // index is therefore all that is required to release the region.
    void commit_read(size_type read_index, size_type count) CASTLE_NOEXCEPT
    {
        read_index_.store(read_index + count, memory_order_release);
    }

    // Publishes [logical_start, logical_start + count) into its mirror slots.
    // The affected physical range crosses the N boundary at most once, so
    // this is split into at most two straight-line copies instead of
    // branching on every element.
    void mirror_range(size_type logical_start, size_type count) CASTLE_NOEXCEPT
    {
        if (count == 0U)
        {
            return;
        }

        CASTLE_CONST size_type start = physical_index(logical_start);
        CASTLE_CONST size_type primary_count = (count < (N - start)) ? count : (N - start);

        // [start, start + primary_count) is in the primary half; publish it
        // into the mirror half so a future wrapped reservation stays contiguous.
        for (size_type i = 0U; i < primary_count; ++i)
        {
            storage_[start + i + N] = storage_[start + i];
        }

        CASTLE_CONST size_type mirror_count = count - primary_count;

        // The remainder was written through the mirror half (used as scratch
        // space by a reservation that crossed the boundary); copy it back so
        // the primary half becomes authoritative again.
        for (size_type i = 0U; i < mirror_count; ++i)
        {
            storage_[i] = storage_[N + i];
        }
    }

    T storage_[2U * N];
    castle::atomic<size_type> read_index_;
    castle::atomic<size_type> write_index_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_MIRRORED_RING_BUFFER_HPP
