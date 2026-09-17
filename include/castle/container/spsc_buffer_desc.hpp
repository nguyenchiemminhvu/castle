// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file spsc_buffer_desc.hpp
 * @brief Fixed-capacity non-owning descriptor ring for single-producer/single-consumer buffer
 * hand-off.
 *
 * Use this container when payload buffers live in caller-managed memory, often DMA-capable
 * storage, and ownership of those buffers must pass deterministically between one producer and
 * one consumer. The descriptor ring itself is fixed-capacity, allocates nothing, and all
 * acquisition, commit, release, and cancel operations run in O(1). With the default
 * `castle::atomic<uint8_t>` state type, each state transition is published with release stores
 * and observed with acquire loads; a plain non-atomic state type is only correct when external
 * synchronization already guarantees ordering.
 *
 * @note Buffer payload memory is supplied by the caller and must remain valid for the lifetime of
 * the descriptor object.
 * @warning This class is SPSC only. `clear()` is a quiescent operation and must not race with
 * DMA, interrupt handlers, writers, or readers.
 */
#ifndef CASTLE_CONTAINER_SPSC_BUFFER_DESC_H
#define CASTLE_CONTAINER_SPSC_BUFFER_DESC_H

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/atomic/atomic.hpp"
#include "castle/container/array.hpp"
#include "castle/container/array_view.hpp"

#include <stdint.h>

namespace castle
{
namespace container
{

namespace detail
{

template <typename T, typename = void>
struct has_ordered_state : castle::meta::false_type
{
};

/**
 * @brief Checks if a state type supports ordered load and store operations.
 * @tparam T State type to check.
 * @return `true` if the state type supports ordered load and store operations, `false` otherwise.
 */
template <typename T>
struct has_ordered_state<T, castle::meta::void_t<
    decltype(castle::meta::declval<CASTLE_CONST T&>().load(castle::memory_order_acquire)),
    decltype(castle::meta::declval<T&>().store(uint8_t(0U), castle::memory_order_release))>>
    : castle::meta::true_type
{
};

} // namespace detail

/**
 * @brief Fixed-capacity descriptor ring for caller-owned buffers in a single-producer/single-
 * consumer pipeline.
 * @tparam TBuffer Element type stored in each payload buffer.
 * @tparam BUFFER_SIZE Number of elements in each payload buffer.
 * @tparam N_BUFFERS Number of descriptors in the ring.
 * @tparam TState Per-slot state type, typically `castle::atomic<uint8_t>`.
 * @note Slot state follows the lifecycle `free -> writing -> ready -> reading -> free`.
 * @warning Multiple independent producers or consumers require external synchronization.
 */
template <typename TBuffer,
          size_type BUFFER_SIZE,
          size_type N_BUFFERS,
          typename TState = castle::atomic<uint8_t>>
class spsc_buffer_desc
{
    static_assert(BUFFER_SIZE > 0U, "spsc_buffer_desc buffer size must be non-zero");
    static_assert(N_BUFFERS > 0U, "spsc_buffer_desc buffer count must be non-zero");
    static_assert(meta::is_default_constructible<TState>::value,
                  "spsc_buffer_desc state type must be default constructible");

private:
    enum class buffer_state : uint8_t
    {
        free = 0U,
        writing,
        ready,
        reading
    };

    enum class descriptor_mode : uint8_t
    {
        none = 0U,
        write,
        read
    };

    struct buffer_slot;

public:
    using value_type      = TBuffer;
    using size_type       = castle::size_type;
    using pointer         = value_type*;
    using const_pointer   = CASTLE_CONST value_type*;
    using buffer_view     = castle::container::array_view<value_type>;
    using const_buffer_view = castle::container::array_view<CASTLE_CONST value_type>;
    using state_type      = TState;

    /** @brief Compile-time descriptor count. */
    static CASTLE_CONSTEXPR size_type static_capacity = N_BUFFERS;

    /** @brief Compile-time payload capacity of each buffer. */
    static CASTLE_CONSTEXPR size_type static_buffer_size = BUFFER_SIZE;

    /**
     * @brief Non-owning handle for one descriptor slot.
     * @note A handle is valid only while its recorded generation matches the slot generation and
     * the slot state matches the handle mode. Copying a handle is allowed, but stale copies stop
     * being valid once the slot is reused.
     */
    class buffer_handle
    {
        friend class spsc_buffer_desc;

    public:
        /**
         * @brief Constructs an invalid handle.
         */
        buffer_handle() CASTLE_NOEXCEPT
            : item_(nullptr)
            , generation_(0U)
            , mode_(descriptor_mode::none)
        {
        }

        /**
         * @brief Constructs a handle from an internal slot, generation, and ownership mode.
         * @param item Descriptor slot.
         * @param generation Slot generation at acquisition time.
         * @param mode Ownership mode, either write or read.
         */
        buffer_handle(buffer_slot* item,
                   uint32_t generation,
                   descriptor_mode mode) CASTLE_NOEXCEPT
            : item_(item)
            , generation_(generation)
            , mode_(mode)
        {
        }

        /**
         * @brief Copy-constructs a handle.
         * @param other Handle to copy.
         */
        buffer_handle(CASTLE_CONST buffer_handle& other) CASTLE_NOEXCEPT
            : item_(other.item_)
            , generation_(other.generation_)
            , mode_(other.mode_)
        {
        }

        /**
         * @brief Copies another handle.
         * @param other Handle to copy.
         * @return Reference to `*this`.
         */
        buffer_handle& operator=(CASTLE_CONST buffer_handle& other) CASTLE_NOEXCEPT
        {
            if (this != &other) // LCOV_EXCL_BR_LINE
            {
                item_ = other.item_;
                generation_ = other.generation_;
                mode_ = other.mode_;
            }
            return *this;
        }

        /**
         * @brief Move-constructs a handle.
         * @param other Handle to move from.
         * @note The moved-from handle becomes invalid.
         */
        buffer_handle(buffer_handle&& other) CASTLE_NOEXCEPT
            : item_(other.item_)
            , generation_(other.generation_)
            , mode_(other.mode_)
        {
            other.item_ = nullptr;
            other.generation_ = 0U;
            other.mode_ = descriptor_mode::none;
        }

        /**
         * @brief Moves another handle into this handle.
         * @param other Handle to move from.
         * @return Reference to `*this`.
         * @note The moved-from handle becomes invalid.
         */
        buffer_handle& operator=(buffer_handle&& other) CASTLE_NOEXCEPT
        {
            if (this != &other) // LCOV_EXCL_BR_LINE
            {
                item_ = other.item_;
                generation_ = other.generation_;
                mode_ = other.mode_;

                other.item_ = nullptr;
                other.generation_ = 0U;
                other.mode_ = descriptor_mode::none;
            }
            return *this;
        }

        /**
         * @brief Tests whether the handle still owns its slot.
         * @return `true` when the slot generation matches and the slot state still corresponds to
         * the handle mode.
         * @note Complexity is O(1). With the default atomic state, the slot state is observed with
         * `memory_order_acquire`.
         */
        CASTLE_NODISCARD bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (item_ == nullptr || generation_ != item_->generation)
            {
                return false;
            }

            CASTLE_CONST buffer_state state = spsc_buffer_desc::load_state(item_->state);
            return (mode_ == descriptor_mode::write && state == buffer_state::writing) ||
                   (mode_ == descriptor_mode::read && state == buffer_state::reading);
        }

        /**
         * @brief Tests whether the handle currently owns a writable slot.
         * @return `true` when the handle is valid and represents a write reservation.
         */
        CASTLE_NODISCARD bool is_write() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return is_valid() && mode_ == descriptor_mode::write;
        }

        /**
         * @brief Tests whether the handle currently owns a readable slot.
         * @return `true` when the handle is valid and represents a read reservation.
         */
        CASTLE_NODISCARD bool is_read() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return is_valid() && mode_ == descriptor_mode::read; // LCOV_EXCL_BR_LINE
        }

        /**
         * @brief Returns the payload capacity of the slot.
         * @return `BUFFER_SIZE`.
         */
        CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return BUFFER_SIZE;
        }

        /**
         * @brief Returns the payload capacity of the slot.
         * @return `BUFFER_SIZE`.
         */
        CASTLE_CONSTEXPR size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return BUFFER_SIZE;
        }

        /**
         * @brief Returns the number of committed readable elements or zero for an invalid handle.
         * @return Committed element count for a read handle, zero for invalid or fresh write
         * handles.
         */
        CASTLE_NODISCARD size_type size() CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (!is_valid())
            {
                return 0U;
            }

            return item_->size;
        }

        /**
         * @brief Returns the raw payload pointer.
         * @return Pointer to the slot payload or `nullptr` when the descriptor was configured with
         * a null buffer base.
         * @warning The handle must be valid; invalid use triggers `CASTLE_ASSERT`.
         */
        pointer data() CASTLE_NOEXCEPT
        {
            CASTLE_ASSERT(is_valid(), CASTLE_ERROR_GENERIC("spsc_buffer_desc: invalid buffer_handle")); // LCOV_EXCL_BR_LINE
            return item_ == nullptr ? nullptr : item_->pbuffer; // LCOV_EXCL_BR_LINE
        }

        /**
         * @brief Returns the raw payload pointer.
         * @return Const pointer to the slot payload or `nullptr` when the descriptor was
         * configured with a null buffer base.
         * @warning The handle must be valid; invalid use triggers `CASTLE_ASSERT`.
         */
        const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT
        {
            CASTLE_ASSERT(is_valid(), CASTLE_ERROR_GENERIC("spsc_buffer_desc: invalid buffer_handle")); // LCOV_EXCL_BR_LINE
            return item_ == nullptr ? nullptr : item_->pbuffer; // LCOV_EXCL_BR_LINE
        }

        /**
         * @brief Returns the full writable payload view of a write reservation.
         * @return Writable `array_view` spanning the entire buffer capacity, or an empty view when
         * the handle is not a valid write handle.
         */
        buffer_view write_view() CASTLE_NOEXCEPT
        {
            if (!is_write())
            {
                return buffer_view();
            }

            return buffer_view(item_->pbuffer, BUFFER_SIZE);
        }

        /**
         * @brief Returns the committed readable payload view of a read reservation.
         * @return Read-only `array_view` spanning `size()` elements, or an empty view when the
         * handle is not a valid read handle.
         */
        const_buffer_view read_view() CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (!is_read())
            {
                return const_buffer_view();
            }

            return const_buffer_view(item_->pbuffer, item_->size);
        }

        /**
         * @brief Publishes a writable buffer as ready for the consumer.
         * @param count Number of valid elements written into the payload.
         * @return `status::ok` on success, `status::invalid_argument` when the handle is not the
         * active write owner, or `status::out_of_range` when `count > BUFFER_SIZE`.
         * @note Successful publication performs a release store to the slot state so the consumer's
         * acquire load observes both `count` and payload contents.
         */
        CASTLE_NODISCARD status commit(size_type count) CASTLE_NOEXCEPT
        {
            if (!is_write())
            {
                return status::invalid_argument;
            }

            if (count > BUFFER_SIZE)
            {
                return status::out_of_range;
            }

            item_->size = count;
            spsc_buffer_desc::store_state(item_->state, buffer_state::ready);
            return status::ok;
        }

        /**
         * @brief Releases a readable slot back to the free pool.
         * @return `status::ok` on success or `status::invalid_argument` when the handle is not the
         * active read owner.
         * @note Successful release performs a release store to the slot state so the producer's
         * acquire load observes the slot as reusable only after consumption is complete.
         */
        CASTLE_NODISCARD status release() CASTLE_NOEXCEPT
        {
            if (!is_read())
            {
                return status::invalid_argument;
            }

            item_->size = 0U;
            spsc_buffer_desc::store_state(item_->state, buffer_state::free);
            return status::ok;
        }

        /**
         * @brief Cancels a writable reservation without publishing it.
         * @return `status::ok` on success or `status::invalid_argument` when the handle is not the
         * active write owner.
         * @note Successful cancellation returns the slot to `free` with release semantics.
         */
        CASTLE_NODISCARD status cancel() CASTLE_NOEXCEPT
        {
            if (!is_write())
            {
                return status::invalid_argument;
            }

            item_->size = 0U;
            spsc_buffer_desc::store_state(item_->state, buffer_state::free);
            return status::ok;
        }

        /**
         * @brief Tests whether the handle is valid.
         * @return Same result as `is_valid()`.
         */
        explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return is_valid();
        }

    private:
        buffer_slot* item_;
        uint32_t generation_;
        descriptor_mode mode_;
    };

    /**
     * @brief Constructs a descriptor ring over caller-owned contiguous payload storage.
     * @param buffers Pointer to `N_BUFFERS * BUFFER_SIZE` contiguous elements.
     * @note Complexity is O(N_BUFFERS) because each descriptor slot is initialized.
     * @warning Passing `nullptr` creates an invalid descriptor that reports
     * `status::invalid_config` from acquisition APIs.
     */
    spsc_buffer_desc(pointer buffers) CASTLE_NOEXCEPT
        : buffers_(buffers)
        , descriptor_items_{}
        , write_index_(0U)
        , read_index_(0U)
    {
        for (size_type i = 0U; i < N_BUFFERS; ++i)
        {
            descriptor_items_[i].pbuffer = buffers_ == nullptr
                                           ? nullptr
                                           : (buffers_ + (i * BUFFER_SIZE));
            descriptor_items_[i].state = static_cast<uint8_t>(buffer_state::free);
            descriptor_items_[i].size = 0U;
            descriptor_items_[i].generation = 0U;
        }
    }

    spsc_buffer_desc(CASTLE_CONST spsc_buffer_desc&)            CASTLE_DELETE;
    spsc_buffer_desc& operator=(CASTLE_CONST spsc_buffer_desc&) CASTLE_DELETE;
    spsc_buffer_desc(spsc_buffer_desc&&)                        CASTLE_DELETE;
    spsc_buffer_desc& operator=(spsc_buffer_desc&&)             CASTLE_DELETE;

    /**
     * @brief Tests whether the descriptor was configured with a valid payload base pointer.
     * @return `true` when the payload base pointer is non-null.
     */
    CASTLE_NODISCARD bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return buffers_ != nullptr;
    }

    /**
     * @brief Returns the number of descriptor slots.
     * @return `N_BUFFERS`.
     */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return N_BUFFERS;
    }

    /**
     * @brief Returns the payload capacity of each slot.
     * @return `BUFFER_SIZE`.
     */
    static CASTLE_CONSTEXPR size_type buffer_capacity() CASTLE_NOEXCEPT
    {
        return BUFFER_SIZE;
    }

    /**
     * @brief Tests whether the next producer slot is available.
     * @return `true` when the descriptor is valid and the slot at the write cursor is `free`.
     * @note With the default atomic state this observes the slot state with acquire semantics.
     */
    CASTLE_NODISCARD bool writable() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_valid() && load_state(descriptor_items_[write_index_].state) == buffer_state::free; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Tests whether the next consumer slot is ready.
     * @return `true` when the descriptor is valid and the slot at the read cursor is `ready`.
     * @note With the default atomic state this observes the slot state with acquire semantics.
     */
    CASTLE_NODISCARD bool readable() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_valid() && load_state(descriptor_items_[read_index_].state) == buffer_state::ready; // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Tests whether the producer cannot acquire another slot.
     * @return `true` when the next write slot is not writable.
     */
    CASTLE_NODISCARD bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !writable();
    }

    /**
     * @brief Tests whether the consumer cannot acquire a ready slot.
     * @return `true` when the next read slot is not readable.
     */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !readable();
    }

    /**
     * @brief Acquires the next free slot for writing.
     * @param out Destination for the acquired handle. Reset to an invalid handle on entry.
     * @return `status::ok` on success, `status::invalid_config` when the descriptor has no valid
     * payload storage, or `status::full` when the next slot is not free.
     * @note Complexity is O(1). Acquisition advances the write cursor immediately.
     */
    CASTLE_NODISCARD status acquire_write(buffer_handle& out) CASTLE_NOEXCEPT
    {
        out = buffer_handle();

        if (!is_valid())
        {
            return status::invalid_config;
        }

        buffer_slot& item = descriptor_items_[write_index_];
        if (load_state(item.state) != buffer_state::free)
        {
            return status::full;
        }

        item.generation = next_generation(item.generation);
        item.size = 0U;
        store_state(item.state, buffer_state::writing);

        out = buffer_handle(&item, item.generation, descriptor_mode::write);
        write_index_ = next_index(write_index_);
        return status::ok;
    }

    /**
     * @brief Acquires the next free slot for writing.
     * @return Valid write handle on success, or an invalid handle when acquisition fails.
     * @note Failures are still distinguishable through `is_valid()` and `writable()`.
     */
    CASTLE_NODISCARD buffer_handle acquire_write() CASTLE_NOEXCEPT
    {
        buffer_handle out;
        (void)acquire_write(out);
        return out;
    }

    /**
     * @brief Acquires the next ready slot for reading.
     * @param out Destination for the acquired handle. Reset to an invalid handle on entry.
     * @return `status::ok` on success, `status::invalid_config` when the descriptor has no valid
     * payload storage, or `status::empty` when the next slot is not ready.
     * @note Complexity is O(1). Acquisition advances the read cursor immediately.
     */
    CASTLE_NODISCARD status acquire_read(buffer_handle& out) CASTLE_NOEXCEPT
    {
        out = buffer_handle();

        if (!is_valid())
        {
            return status::invalid_config;
        }

        buffer_slot& item = descriptor_items_[read_index_];
        if (load_state(item.state) != buffer_state::ready)
        {
            return status::empty;
        }

        store_state(item.state, buffer_state::reading);
        out = buffer_handle(&item, item.generation, descriptor_mode::read);
        read_index_ = next_index(read_index_);
        return status::ok;
    }

    /**
     * @brief Acquires the next ready slot for reading.
     * @return Valid read handle on success, or an invalid handle when acquisition fails.
     */
    CASTLE_NODISCARD buffer_handle acquire_read() CASTLE_NOEXCEPT
    {
        buffer_handle out;
        (void)acquire_read(out);
        return out;
    }

    /**
     * @brief Resets every descriptor slot to `free`.
     * @note Complexity is O(N_BUFFERS). Each slot generation is advanced so any outstanding
     * handles become invalid.
     * @warning This is a quiescent operation and must not race with producer or consumer work.
     */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N_BUFFERS; ++i)
        {
            descriptor_items_[i].size = 0U;
            descriptor_items_[i].generation = next_generation(descriptor_items_[i].generation);
            store_state(descriptor_items_[i].state, buffer_state::free);
        }

        write_index_ = 0U;
        read_index_ = 0U;
    }

private:
    struct buffer_slot
    {
        pointer   pbuffer;
        TState    state;
        size_type size;
        uint32_t  generation;
    };

    /**
     * @brief Loads the state of a buffer slot.
     * @param state Reference to the state variable.
     * @return Current state of the buffer slot.
     */
    static buffer_state load_state(CASTLE_CONST TState& state) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (detail::has_ordered_state<TState>::value)
        {
            return static_cast<buffer_state>(static_cast<uint8_t>(state.load(castle::memory_order_acquire)));
        }
        else
        {
            return static_cast<buffer_state>(static_cast<uint8_t>(state));
        }
    }

    /**
     * @brief Stores a new state into a buffer slot.
     * @param state Reference to the state variable.
     * @param value New state to store in the buffer slot.
     */
    static void store_state(TState& state, buffer_state value) CASTLE_NOEXCEPT
    {
        // Release ordering publishes payload contents and committed size before the new state.
        CASTLE_IF_CONSTEXPR (detail::has_ordered_state<TState>::value)
        {
            state.store(static_cast<uint8_t>(value), castle::memory_order_release);
        }
        else
        {
            state = static_cast<uint8_t>(value);
        }
    }

    /**
     * @brief Calculates the next index in a circular buffer.
     * @param index Current index in the circular buffer.
     * @return Next index in the circular buffer.
     */
    static size_type next_index(size_type index) CASTLE_NOEXCEPT
    {
        ++index;
        return index == N_BUFFERS ? 0U : index;
    }

    /**
     * @brief Calculates the next generation number for a buffer slot.
     * @param generation Current generation number of the buffer slot.
     * @return Next generation number for the buffer slot.
     */
    static uint32_t next_generation(uint32_t generation) CASTLE_NOEXCEPT
    {
        ++generation;
        return generation == 0U ? 1U : generation; // LCOV_EXCL_BR_LINE
    }

    pointer buffers_;
    castle::container::array<buffer_slot, N_BUFFERS> descriptor_items_;
    size_type write_index_;
    size_type read_index_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_SPSC_BUFFER_DESC_H
