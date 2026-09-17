// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mirrored_ring_buffer.hpp
 * @brief Fixed-capacity single-producer/single-consumer ring buffer that mirrors its logical
 * sequence into a second physical half.
 *
 * Use this container when FIFO semantics and contiguous reservations are needed for DMA or other
 * streaming I/O without dynamic allocation. The buffer owns `2 * N` elements internally, keeps
 * the logical sequence mirrored so wrapped readable and writable regions stay physically
 * contiguous, and provides O(1) single-element operations plus O(count) bulk transfers.
 *
 * @note The implementation does not require page alignment or external virtual-memory tricks; it
 * mirrors writes into the second physical half itself. For power-of-two capacities it uses bit
 * masking for logical-to-physical indexing, otherwise modulo arithmetic.
 * @warning This container is designed for exactly one producer and one consumer. `clear()` is a
 * quiescent operation and must not race with producer or consumer activity.
 */
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

/**
 * @brief Fixed-capacity SPSC ring buffer with mirrored contiguous storage.
 * @tparam T Element type.
 * @tparam N Logical element capacity.
 * @note `T` must be trivially copyable and trivially destructible because the buffer copies raw
 * element values between mirrored halves and never runs destructors.
 * @warning The producer may call write/push APIs and the consumer may call read/pop APIs. Mixing
 * multiple writers or readers requires external synchronization.
 */
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

    /** @brief Compile-time logical capacity. */
    static CASTLE_CONSTEXPR size_type static_capacity = N;

    /**
     * @brief Constructs an empty mirrored ring buffer.
     * @note Complexity is O(1).
     */
    mirrored_ring_buffer() CASTLE_NOEXCEPT
        : storage_{}, read_index_(0U), write_index_(0U)
    {
    }

    mirrored_ring_buffer(CASTLE_CONST mirrored_ring_buffer&)            CASTLE_DELETE;
    mirrored_ring_buffer& operator=(CASTLE_CONST mirrored_ring_buffer&) CASTLE_DELETE;
    mirrored_ring_buffer(mirrored_ring_buffer&&)                        CASTLE_DELETE;
    mirrored_ring_buffer& operator=(mirrored_ring_buffer&&)             CASTLE_DELETE;

    /**
     * @brief Returns the logical element capacity.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the logical maximum size.
     * @return `N`.
     */
    static CASTLE_CONSTEXPR size_type max_size() CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the current number of readable elements.
     * @return Occupancy in O(1).
     * @note The method acquires both indices so producer-published writes and consumer-published
     * releases are visible before the result is observed.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type read_index = read_index_.load(memory_order_acquire);
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_acquire);
        return write_index - read_index;
    }

    /**
     * @brief Tests whether the buffer is empty.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size() == 0U;
    }

    /**
     * @brief Tests whether the buffer is full.
     * @return `true` when no writable slots remain.
     */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return available() == 0U;
    }

    /**
     * @brief Returns the number of writable slots.
     * @return `capacity() - size()`.
     */
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return N - size();
    }

    /**
     * @brief Discards all logical contents by advancing the read index to the current write
     * index.
     * @note Complexity is O(1).
     * @warning This is a quiescent operation and must not race with producer or consumer work.
     */
    void clear() CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_acquire);
        read_index_.store(write_index, memory_order_release);
    }

    /**
     * @brief Reserves up to `max_reserve_size` contiguous writable elements.
     * @param max_reserve_size Maximum number of requested writable elements.
     * @return Writable view beginning at the current producer position.
     * @note Complexity is O(1). Because the physical storage is mirrored, the returned view is
     * contiguous even when the logical write position is near the wrap boundary.
     */
    array_view<T> write_reserve(size_type max_reserve_size = N) CASTLE_NOEXCEPT
    {
        size_type write_index{0U};
        CASTLE_CONST size_type free_size = writable_size(write_index);
        CASTLE_CONST size_type reserve_size =
            (max_reserve_size < free_size) ? max_reserve_size : free_size;
        return array_view<T>(physical_data(write_index), reserve_size);
    }

    /**
     * @brief Returns the entire currently writable contiguous region when it is large enough.
     * @param min_reserve_size Minimum acceptable reservation length.
     * @return Writable view spanning all currently available contiguous elements, or an empty
     * view at the current write position when the free region is smaller than
     * `min_reserve_size`.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Publishes a previously reserved writable region.
     * @param reservation Reservation returned by `write_reserve()` or `write_reserve_optimal()`.
     * @return `status::ok` on success, `status::invalid_argument` when the reservation does not
     * belong to the current producer position, or `status::full` when the reservation length
     * exceeds the currently free region.
     * @note Complexity is O(reservation.size()) because the written range is mirrored.
     * @warning A reservation must be committed by the same producer side that created it before
     * requesting another producer reservation.
     */
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

    /**
     * @brief Reserves up to `max_reserve_size` contiguous readable elements.
     * @param max_reserve_size Maximum number of requested readable elements.
     * @return Readable view beginning at the current consumer position.
     * @note Complexity is O(1). Mirrored storage keeps wrapped readable data contiguous.
     */
    array_view<T> read_reserve(size_type max_reserve_size = N) CASTLE_NOEXCEPT
    {
        size_type read_index{0U};
        CASTLE_CONST size_type readable = readable_size(read_index);
        CASTLE_CONST size_type reserve_size =
            (max_reserve_size < readable) ? max_reserve_size : readable;
        return array_view<T>(physical_data(read_index), reserve_size);
    }

    /**
     * @brief Releases a previously reserved readable region.
     * @param reservation Reservation returned by `read_reserve()`.
     * @return `status::ok` on success, `status::invalid_argument` when the reservation does not
     * belong to the current consumer position, or `status::empty` when the reservation length
     * exceeds the currently readable region.
     * @note Complexity is O(1).
     * @warning A reservation must be committed by the same consumer side that created it before
     * requesting another consumer reservation.
     */
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

    /**
     * @brief Pushes one element into the buffer.
     * @param value Element to copy.
     * @return `status::ok` on success or `status::full` when no free slot remains.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Pushes one element into the buffer by move assignment.
     * @param value Element to move.
     * @return `status::ok` on success or `status::full` when no free slot remains.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Pops one element from the buffer into `out`.
     * @param out Destination for the removed element.
     * @return `status::ok` on success or `status::empty` when no readable element exists.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Pops one element and discards it.
     * @return `status::ok` on success or `status::empty` when no readable element exists.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Reads an element at logical offset `index` without consuming it.
     * @param index Zero-based offset from the current front element.
     * @param out Destination for the copied element.
     * @return `status::ok` on success or `status::out_of_range` when `index` is not currently
     * readable.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Copies up to `max_count` elements from `src` into the buffer.
     * @param src Source pointer.
     * @param max_count Maximum number of elements to push.
     * @return Number of elements actually written.
     * @note Complexity is O(return value).
     */
    size_type push_bulk(CASTLE_CONST value_type* src, size_type max_count) CASTLE_NOEXCEPT
    {
        if (src == nullptr)
        {
            return 0U;
        }

        size_type write_index;
        CASTLE_CONST size_type free_size = writable_size(write_index);
        CASTLE_CONST size_type count = (max_count < free_size) ? max_count : free_size; // LCOV_EXCL_BR_LINE

        pointer destination = physical_data(write_index);
        for (size_type i = 0U; i < count; ++i)
        {
            destination[i] = src[i];
        }

        commit_write(write_index, count);
        return count;
    }

    /**
     * @brief Copies up to `max_count` elements from the buffer into `dst`.
     * @param dst Destination pointer.
     * @param max_count Maximum number of elements to pop.
     * @return Number of elements actually read.
     * @note Complexity is O(return value).
     */
    size_type pop_bulk(value_type* dst, size_type max_count) CASTLE_NOEXCEPT
    {
        if (dst == nullptr)
        {
            return 0U;
        }

        size_type read_index;
        CASTLE_CONST size_type readable = readable_size(read_index);
        CASTLE_CONST size_type count = (max_count < readable) ? max_count : readable; // LCOV_EXCL_BR_LINE

        pointer source = physical_data(read_index);
        for (size_type i = 0U; i < count; ++i)
        {
            dst[i] = source[i];
        }

        commit_read(read_index, count);
        return count;
    }

    /**
     * @brief Returns the element at logical offset `index` without bounds checking.
     * @param index Zero-based offset from the front element.
     * @return Reference to the requested element.
     * @warning Callers must ensure `index < size()` and that readable data has been observed
     * through `size()`, `empty()`, `end()`, or `read_reserve()`.
     */
    reference operator[](size_type index) CASTLE_NOEXCEPT
    {
        return *(begin() + index);
    }

    /**
     * @brief Returns the element at logical offset `index` without bounds checking.
     * @param index Zero-based offset from the front element.
     * @return Const reference to the requested element.
     * @warning Callers must ensure `index < size()` and that readable data has been observed
     * through `size()`, `empty()`, `end()`, or `read_reserve()`.
     */
    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *(begin() + index);
    }

    /**
     * @brief Returns the first readable element.
     * @return Reference to the logical front element.
     * @warning The buffer must not be empty and readable data must already be established.
     */
    reference front() CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the first readable element.
     * @return Const reference to the logical front element.
     * @warning The buffer must not be empty and readable data must already be established.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the last readable element.
     * @return Reference to the logical back element.
     * @warning The buffer must not be empty and readable data must already be established.
     */
    reference back() CASTLE_NOEXCEPT { return *(end() - 1); }

    /**
     * @brief Returns the last readable element.
     * @return Const reference to the logical back element.
     * @warning The buffer must not be empty and readable data must already be established.
     */
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return *(end() - 1); }

    /**
     * @brief Returns an iterator to the first readable element.
     * @return Pointer to the current logical front.
     * @note The consumer owns `read_index_`, so this uses a relaxed self-load. Callers are
     * expected to establish readability through `size()`, `empty()`, `end()`, or
     * `read_reserve()` before dereferencing.
     */
    iterator begin() CASTLE_NOEXCEPT
    {
        return physical_data(read_index_.load(memory_order_relaxed));
    }

    /**
     * @brief Returns a const iterator to the first readable element.
     * @return Const pointer to the current logical front.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return physical_data(read_index_.load(memory_order_relaxed));
    }

    /**
     * @brief Returns a const iterator to the first readable element.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns an iterator one past the last readable element.
     * @return End iterator for the current contiguous logical sequence.
     * @note The acquire load performed by `readable_size()` makes newly published producer data
     * visible before the returned range is consumed.
     */
    iterator end() CASTLE_NOEXCEPT
    {
        size_type read_index;
        CASTLE_CONST size_type count = readable_size(read_index);
        return physical_data(read_index) + count;
    }

    /**
     * @brief Returns a const iterator one past the last readable element.
     * @return Const end iterator for the current contiguous logical sequence.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type read_index;
        CASTLE_CONST size_type count = readable_size(read_index);
        return physical_data(read_index) + count;
    }

    /**
     * @brief Returns a const iterator one past the last readable element.
     * @return Const end iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Returns a pointer to the first readable element.
     * @return Pointer equal to `begin()`.
     * @warning Readability must already be established before dereferencing the returned pointer.
     */
    pointer data() CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns a const pointer to the first readable element.
     * @return Const pointer equal to `begin()`.
     * @warning Readability must already be established before dereferencing the returned pointer.
     */
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

private:
    /**
     * @brief Converts a logical index to a physical index within the underlying storage array.
     * @param logical_index Logical index to be converted.
     * @return Physical index corresponding to the given logical index.
     */
    static CASTLE_CONSTEXPR size_type physical_index(size_type logical_index) CASTLE_NOEXCEPT
    {
        return meta::is_power_of_two<N>::value
                   ? (logical_index & (N - 1U))
                   : (logical_index % N);
    }

    /**
     * @brief Returns a pointer to the physical storage corresponding to the given logical index.
     * @param logical_index Logical index to be converted to a physical pointer.
     * @return Pointer to the physical storage corresponding to the given logical index.
     */
    pointer physical_data(size_type logical_index) CASTLE_NOEXCEPT
    {
        return storage_ + physical_index(logical_index);
    }

    /**
     * @brief Returns a const pointer to the physical storage corresponding to the given logical index.
     * @param logical_index Logical index to be converted to a physical pointer.
     * @return Const pointer to the physical storage corresponding to the given logical index.
     */
    const_pointer physical_data(size_type logical_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return storage_ + physical_index(logical_index);
    }

    /**
     * @brief Calculates the free space available from the given write index.
     * @param write_index Logical write index from which to calculate free space.
     * @return Free space available from the given write index.
     */
    size_type free_size_from(size_type write_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        // The producer acquires the consumer-owned read index before reusing freed slots.
        CASTLE_CONST size_type read_index = read_index_.load(memory_order_acquire);
        return N - (write_index - read_index);
    }

    /**
     * @brief Calculates the writable size from the current write index.
     * @param write_index Reference to the current logical write index.
     * @return Writable size available from the current write index.
     */
    size_type writable_size(size_type& write_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        // The producer self-load is relaxed because only the producer stores write_index_.
        write_index = write_index_.load(memory_order_relaxed);
        return free_size_from(write_index);
    }

    /**
     * @brief Calculates the readable size from the given read index.
     * @param read_index Logical read index from which to calculate readable size.
     * @return Readable size available from the given read index.
     */
    size_type readable_size_from(size_type read_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        // The consumer acquires the producer-owned write index before reading new data.
        CASTLE_CONST size_type write_index = write_index_.load(memory_order_acquire);
        return write_index - read_index;
    }

    /**
     * @brief Calculates the readable size from the current read index.
     * @param read_index Reference to the current logical read index.
     * @return Readable size available from the current read index.
     */
    size_type readable_size(size_type& read_index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        // The consumer self-load is relaxed because only the consumer stores read_index_.
        read_index = read_index_.load(memory_order_relaxed);
        return readable_size_from(read_index);
    }

    /**
     * @brief Commits a write operation by updating the write index and mirroring the written range.
     * @param write_index Current logical write index before committing.
     * @param count Number of elements written to commit.
     */
    void commit_write(size_type write_index, size_type count) CASTLE_NOEXCEPT
    {
        mirror_range(write_index, count);
        write_index_.store(write_index + count, memory_order_release);
    }

    /**
     * @brief Commits a read operation by updating the read index.
     * @param read_index Current logical read index before committing.
     * @param count Number of elements read to commit.
     */
    void commit_read(size_type read_index, size_type count) CASTLE_NOEXCEPT
    {
        read_index_.store(read_index + count, memory_order_release);
    }

    /**
     * @brief Mirrors a range of elements in the buffer to maintain contiguous access.
     * @param logical_start Logical start index of the range to mirror.
     * @param count Number of elements in the range to mirror.
     */
    void mirror_range(size_type logical_start, size_type count) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (count == 0U)
        {
            return;
        }
        // LCOV_EXCL_STOP

        CASTLE_CONST size_type start = physical_index(logical_start);
        CASTLE_CONST size_type primary_count = (count < (N - start)) ? count : (N - start);

        // Copy the primary-half write into the mirror half so wrapped sequences stay contiguous.
        for (size_type i = 0U; i < primary_count; ++i)
        {
            storage_[start + i + N] = storage_[start + i];
        }

        CASTLE_CONST size_type mirror_count = count - primary_count;

        // Copy any overflow written through the mirror half back into the primary half.
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
