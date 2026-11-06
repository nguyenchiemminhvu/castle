// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file data_slot.hpp
 * @brief Lock-free latest-value cell: one writer publishes, any number of readers hold snapshots.
 *
 * `data_slot<T, MaxReaders>` is the heap-free counterpart of an atomically swapped
 * `shared_ptr<const T>`. It owns `MaxReaders + 2` buffers of `T`. `publish()` copies a value into
 * a buffer that no reader holds and makes it current in one atomic step; `get_snapshot()` pins the
 * current buffer and returns a move-only `snapshot<T>` that reads it in place, without copying.
 * A held snapshot stays valid and unchanged while newer values are published.
 *
 * Readers are wait-free (one atomic increment to acquire, a short CAS or decrement to release).
 * The writer never waits: it reports `status::full` when every spare buffer is still held.
 *
 * @code
 * struct imu
 * {
 *     float accel[3]{};
 * };
 *
 * castle_ext::data::data_slot<imu, 2U> slot;   // two readers may hold old snapshots
 *
 * // Writer (HAL) thread
 * castle::status result = slot.publish(imu{{0.0F, 0.0F, 9.81F}});
 *
 * // Reader thread
 * castle_ext::data::snapshot<imu> latest = slot.get_snapshot();
 * float z = latest->accel[2];
 * @endcode
 */
#ifndef CASTLE_EXT_DATA_DATA_SLOT_HPP
#define CASTLE_EXT_DATA_DATA_SLOT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/sync/atomic.hpp"

#include <stdint.h>

namespace castle_ext
{
namespace data
{

namespace detail
{

/**
 * @brief Layout of a slot's control word.
 *
 * The high byte is the index of the current buffer. The low 24 bits count readers that pinned
 * the current buffer and still hold it. When the writer retires a buffer it moves that count to
 * the buffer's own `holders` counter, so a reader can acquire with a single `fetch_add`.
 */
struct snapshot_control
{
    static CASTLE_CONSTEXPR uint32_t index_shift = 24U;
    static CASTLE_CONSTEXPR uint32_t reader_mask = (1UL << index_shift) - 1U;
    static CASTLE_CONSTEXPR castle::size_type max_buffers = 256U;

    static CASTLE_CONSTEXPR uint32_t index(uint32_t word) CASTLE_NOEXCEPT
    {
        return word >> index_shift;
    }

    static CASTLE_CONSTEXPR uint32_t readers(uint32_t word) CASTLE_NOEXCEPT
    {
        return word & reader_mask;
    }

    static CASTLE_CONSTEXPR uint32_t make(uint32_t buffer_index) CASTLE_NOEXCEPT
    {
        return buffer_index << index_shift;
    }
};

/** @brief One buffer of a slot: value, publication generation, holders of a retired buffer. */
template <typename T>
struct snapshot_buffer
{
    T value{};
    uint32_t generation = 0U;
    castle::atomic_uint32_t holders{0U};
};

} // namespace detail

template <typename T, castle::size_type MaxReaders>
class data_slot;

/**
 * @brief Move-only read handle to one published value, like `shared_ptr<const T>`.
 *
 * @tparam T Value type.
 *
 * While a snapshot is alive its buffer is never rewritten. Destroying or `reset()`-ing the
 * snapshot returns the buffer to the writer. A default-constructed or moved-from snapshot is
 * empty; dereferencing an empty snapshot is a contract violation reported by `CASTLE_ASSERT`.
 *
 * @warning A snapshot must not outlive the slot or pool it came from.
 */
template <typename T>
class snapshot CASTLE_FINAL
{
public:
    using value_type = T;

    /** @brief Constructs an empty snapshot. */
    snapshot() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    snapshot(CASTLE_CONST snapshot&) CASTLE_DELETE;
    snapshot& operator=(CASTLE_CONST snapshot&) CASTLE_DELETE;

    /** @brief Takes over the buffer held by `other`, leaving `other` empty. */
    snapshot(snapshot&& other) CASTLE_NOEXCEPT
        : buffer_(other.buffer_),
          control_(other.control_),
          index_(other.index_)
    {
        other.buffer_ = nullptr;
    }

    /** @brief Releases the current buffer and takes over the buffer held by `other`. */
    snapshot& operator=(snapshot&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            reset();
            buffer_ = other.buffer_;
            control_ = other.control_;
            index_ = other.index_;
            other.buffer_ = nullptr;
        }
        return *this;
    }

    /** @brief Releases the held buffer. */
    ~snapshot()
    {
        reset();
    }

    /**
     * @brief Releases the held buffer and leaves the snapshot empty.
     * @note Wait-free in practice: a CAS retried only while the control word changes concurrently.
     */
    void reset() CASTLE_NOEXCEPT
    {
        if (buffer_ == nullptr)
        {
            return;
        }

        uint32_t word = control_->load(castle::memory_order_relaxed);
        bool released = false;
        while (!released)
        {
            if (detail::snapshot_control::index(word) == index_)
            {
                // Still the current buffer: this reader is counted in the control word.
                released = control_->compare_exchange_weak( // LCOV_EXCL_BR_LINE
                    word, word - 1U, castle::memory_order_release, castle::memory_order_relaxed);
            }
            else
            {
                // Retired buffer: the writer moved this reader's count to the buffer itself.
                (void)buffer_->holders.fetch_sub(1U, castle::memory_order_release);
                released = true;
            }
        }
        buffer_ = nullptr;
    }

    /** @brief Returns whether the snapshot holds a value. */
    explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return buffer_ != nullptr;
    }

    /** @brief Returns the held value, or `nullptr` when empty. */
    CASTLE_NODISCARD CASTLE_CONST T* get() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (buffer_ != nullptr) ? &buffer_->value : nullptr;
    }

    /** @brief Returns the held value. The snapshot must not be empty. */
    CASTLE_CONST T& operator*() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(buffer_ != nullptr, CASTLE_ERROR_GENERIC("empty data snapshot")); // LCOV_EXCL_BR_LINE
        return buffer_->value;
    }

    /** @brief Accesses a member of the held value. The snapshot must not be empty. */
    CASTLE_CONST T* operator->() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return &**this;
    }

    /**
     * @brief Returns the publication number of the held value.
     * @return 0 for the initial default value (nothing published yet, or empty snapshot),
     *         otherwise 1, 2, 3, ... per successful `publish()`. After 2^32 - 1 publications the
     *         count wraps to 1, never back to 0.
     */
    CASTLE_NODISCARD uint32_t generation() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (buffer_ != nullptr) ? buffer_->generation : 0U;
    }

private:
    template <typename, castle::size_type>
    friend class data_slot;

    snapshot(detail::snapshot_buffer<T>& buffer, castle::atomic_uint32_t& control, uint32_t index) CASTLE_NOEXCEPT
        : buffer_(&buffer),
          control_(&control),
          index_(index)
    {
    }

    detail::snapshot_buffer<T>* buffer_ = nullptr;
    castle::atomic_uint32_t* control_ = nullptr;
    uint32_t index_ = 0U;
};

/**
 * @brief Single-writer, multi-reader latest-value cell with zero-copy snapshots.
 *
 * @tparam T Value type: a default-constructible, copy-assignable object type. It does not need
 *           to be trivially copyable, so Castle containers and strings may be members.
 * @tparam MaxReaders Number of snapshots of older values that may be held at the same time while
 *         the writer keeps publishing. The slot owns `MaxReaders + 2` buffers of `T`.
 *
 * Construction leaves a default-constructed `T` current with generation 0, so readers always get
 * a valid snapshot.
 *
 * @note One writer at a time. A `publish()` that overlaps another one on the same slot is rejected
 *       with `status::data_loss` instead of corrupting state. Readers may run on any thread.
 */
template <typename T, castle::size_type MaxReaders = 1U>
class data_slot CASTLE_FINAL
{
    static_assert(castle::meta::is_object<T>::value && !castle::meta::is_array<T>::value,
                  "data_slot value type must be a non-array object type");
    static_assert(!castle::meta::is_const<T>::value && !castle::meta::is_volatile<T>::value,
                  "data_slot value type must not be cv-qualified");
    static_assert(castle::meta::is_default_constructible<T>::value,
                  "data_slot value type must be default-constructible");
    static_assert(castle::meta::is_copy_assignable<T>::value,
                  "data_slot value type must be copy-assignable");
    static_assert(MaxReaders > 0U, "data_slot requires MaxReaders > 0");
    static_assert(MaxReaders + 2U <= detail::snapshot_control::max_buffers,
                  "data_slot supports at most 254 readers");

public:
    using value_type = T;

    /** @brief Constructs the slot with a default-constructed current value (generation 0). */
    data_slot() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    data_slot(CASTLE_CONST data_slot&) CASTLE_DELETE;
    data_slot& operator=(CASTLE_CONST data_slot&) CASTLE_DELETE;

    /**
     * @brief Makes a copy of `value` the current value.
     * @param value Value copied into a free buffer.
     * @return `status::ok`; `status::full` when readers hold every spare buffer (raise `MaxReaders`
     *         or release snapshots sooner); `status::data_loss` when another `publish()` on this
     *         slot is in progress. On failure the current value is unchanged.
     * @note Never blocks. Complexity: one copy of `T` plus O(MaxReaders) atomic loads.
     */
    castle::status publish(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        if (writing_.exchange(true, castle::memory_order_acquire))
        {
            return castle::status::data_loss;
        }

        CASTLE_CONST castle::status result = commit(value);
        writing_.store(false, castle::memory_order_release);
        return result;
    }

    /**
     * @brief Pins the current value.
     * @return A non-empty snapshot of the latest published value.
     * @note Wait-free: one atomic increment.
     */
    CASTLE_NODISCARD snapshot<T> get_snapshot() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint32_t word = control_.fetch_add(1U, castle::memory_order_acquire);
        CASTLE_CONST uint32_t index = detail::snapshot_control::index(word);
        return snapshot<T>{buffers_[index], control_, index};
    }

private:
    static CASTLE_CONSTEXPR uint32_t buffer_count = static_cast<uint32_t>(MaxReaders + 2U);

    castle::status commit(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint32_t next = find_free_buffer();
        if (next == buffer_count)
        {
            return castle::status::full;
        }

        detail::snapshot_buffer<T>& target = buffers_[next];
        target.value = value;
        // Counts 1 .. 2^32 - 1 and wraps to 1, so 0 always means "initial default value".
        generation_ = (generation_ % UINT32_MAX) + 1U;
        target.generation = generation_;

        CASTLE_CONST uint32_t retired = control_.exchange(detail::snapshot_control::make(next),
                                                          castle::memory_order_acq_rel);
        (void)buffers_[current_].holders.fetch_add(detail::snapshot_control::readers(retired),
                                                   castle::memory_order_relaxed);
        current_ = next;
        return castle::status::ok;
    }

    uint32_t find_free_buffer() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (uint32_t i = 0U; i < buffer_count; ++i)
        {
            if ((i != current_) && (buffers_[i].holders.load(castle::memory_order_acquire) == 0U))
            {
                return i;
            }
        }
        return buffer_count;
    }

    CASTLE_MUTABLE detail::snapshot_buffer<T> buffers_[buffer_count]{};
    CASTLE_MUTABLE castle::atomic_uint32_t control_{0U};
    castle::atomic_bool writing_{false};
    uint32_t current_ = 0U;
    uint32_t generation_ = 0U;
};

} // namespace data
} // namespace castle_ext

#endif // CASTLE_EXT_DATA_DATA_SLOT_HPP
