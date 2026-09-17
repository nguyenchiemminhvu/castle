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

// Detects a TState that exposes explicit-memory-order load()/store(), i.e.
// a castle::atomic<...>-shaped type. A plain integral TState (only valid
// when the caller already guarantees synchronization, e.g. a critical
// section) falls back to an ordinary load/store in spsc_buffer_desc.
template <typename T, typename = void>
struct has_ordered_state : castle::meta::false_type
{
};

template <typename T>
struct has_ordered_state<T, castle::meta::void_t<
    decltype(castle::meta::declval<CASTLE_CONST T&>().load(castle::memory_order_acquire)),
    decltype(castle::meta::declval<T&>().store(uint8_t(0U), castle::memory_order_release))>>
    : castle::meta::true_type
{
};

} // namespace detail

// ============================================================================
// spsc_buffer_desc
//
// Fixed-capacity, non-owning DMA buffer descriptor ring.
//
// The class owns only descriptor metadata. The caller owns the actual
// contiguous storage passed to the constructor.
//
// Buffer lifecycle:
//
//     free -> writing -> ready -> reading -> free
//
// The write side acquires one or more buffers in FIFO order, fills them (often
// by DMA), then commits the number of valid elements. The read side acquires
// committed buffers in the same order and releases them when processing is
// complete.
//
// The default state type is castle::atomic<uint8_t>, which makes the state
// hand-off suitable for a single producer/single consumer arrangement such as
// a DMA/ISR producer and a foreground task consumer. Multiple producers or
// consumers require an external synchronization mechanism.
// ============================================================================
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

    static CASTLE_CONSTEXPR size_type static_capacity = N_BUFFERS;
    static CASTLE_CONSTEXPR size_type static_buffer_size = BUFFER_SIZE;

    // ========================================================================
    // buffer_handle
    //
    // A small non-owning ownership handle for one buffer slot.
    // Generation checking prevents an old copied handle from operating on
    // the slot after it has been released and reused.
    // ========================================================================
    class buffer_handle
    {
        friend class spsc_buffer_desc;

    public:
        buffer_handle() CASTLE_NOEXCEPT
            : item_(nullptr)
            , generation_(0U)
            , mode_(descriptor_mode::none)
        {
        }

        buffer_handle(buffer_slot* item,
                   uint32_t generation,
                   descriptor_mode mode) CASTLE_NOEXCEPT
            : item_(item)
            , generation_(generation)
            , mode_(mode)
        {
        }

        buffer_handle(CASTLE_CONST buffer_handle& other) CASTLE_NOEXCEPT
            : item_(other.item_)
            , generation_(other.generation_)
            , mode_(other.mode_)
        {
        }

        buffer_handle& operator=(CASTLE_CONST buffer_handle& other) CASTLE_NOEXCEPT
        {
            if (this != &other)
            {
                item_ = other.item_;
                generation_ = other.generation_;
                mode_ = other.mode_;
            }
            return *this;
        }

        buffer_handle(buffer_handle&& other) CASTLE_NOEXCEPT
            : item_(other.item_)
            , generation_(other.generation_)
            , mode_(other.mode_)
        {
            other.item_ = nullptr;
            other.generation_ = 0U;
            other.mode_ = descriptor_mode::none;
        }

        buffer_handle& operator=(buffer_handle&& other) CASTLE_NOEXCEPT
        {
            if (this != &other)
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

        CASTLE_NODISCARD bool is_write() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return is_valid() && mode_ == descriptor_mode::write;
        }

        CASTLE_NODISCARD bool is_read() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return is_valid() && mode_ == descriptor_mode::read;
        }

        CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return BUFFER_SIZE;
        }

        CASTLE_CONSTEXPR size_type max_size() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return BUFFER_SIZE;
        }

        CASTLE_NODISCARD size_type size() CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (!is_valid())
            {
                return 0U;
            }

            return item_->size;
        }

        pointer data() CASTLE_NOEXCEPT
        {
            CASTLE_ASSERT(is_valid(), CASTLE_ERROR_GENERIC("spsc_buffer_desc: invalid buffer_handle"));
            return item_ == nullptr ? nullptr : item_->pbuffer;
        }

        const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT
        {
            CASTLE_ASSERT(is_valid(), CASTLE_ERROR_GENERIC("spsc_buffer_desc: invalid buffer_handle"));
            return item_ == nullptr ? nullptr : item_->pbuffer;
        }

        buffer_view write_view() CASTLE_NOEXCEPT
        {
            if (!is_write())
            {
                return buffer_view();
            }

            return buffer_view(item_->pbuffer, BUFFER_SIZE);
        }

        const_buffer_view read_view() CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (!is_read())
            {
                return const_buffer_view();
            }

            return const_buffer_view(item_->pbuffer, item_->size);
        }

        // Publish the number of valid elements written to the buffer.
        // After a successful commit the write buffer_handle becomes invalid;
        // ownership moves to the read side of the buffer_handle ring.
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

        // Release a readable buffer back to the free pool.
        // A second release is rejected because is_valid() becomes false after
        // the first release.
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

        // Cancel an acquired write buffer without publishing it.
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

        explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return is_valid();
        }

    private:
        buffer_slot* item_;
        uint32_t generation_;
        descriptor_mode mode_;
    };

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

    CASTLE_NODISCARD bool is_valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return buffers_ != nullptr;
    }

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return N_BUFFERS;
    }

    static CASTLE_CONSTEXPR size_type buffer_capacity() CASTLE_NOEXCEPT
    {
        return BUFFER_SIZE;
    }

    // True when the next FIFO slot is available to the producer.
    CASTLE_NODISCARD bool writable() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_valid() && load_state(descriptor_items_[write_index_].state) == buffer_state::free;
    }

    // True when the next FIFO slot contains a committed buffer.
    CASTLE_NODISCARD bool readable() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_valid() && load_state(descriptor_items_[read_index_].state) == buffer_state::ready;
    }

    CASTLE_NODISCARD bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !writable();
    }

    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !readable();
    }

    // Acquire the next contiguous buffer for writing or DMA reception.
    // The returned buffer_handle owns that slot until commit() or cancel().
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

    // An invalid buffer_handle is returned when no buffer is available.
    CASTLE_NODISCARD buffer_handle acquire_write() CASTLE_NOEXCEPT
    {
        buffer_handle out;
        (void)acquire_write(out);
        return out;
    }

    // Acquire the next committed buffer for reading/processing.
    // The returned buffer_handle owns that slot until release().
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

    // An invalid buffer_handle is returned when no buffer is ready.
    CASTLE_NODISCARD buffer_handle acquire_read() CASTLE_NOEXCEPT
    {
        buffer_handle out;
        (void)acquire_read(out);
        return out;
    }

    // Release all buffer_handle slots. This is a quiescent operation and must not
    // race with DMA, an ISR, or a foreground reader/writer.
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

    // Acquire/release pairing at the free<->writing and ready<->reading hand-offs
    // is what actually publishes a slot's buffer contents between producer and
    // consumer; a plain (non-atomic) TState is only safe when the caller already
    // guarantees ordering externally, so it falls back to an ordinary access.
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

    static void store_state(TState& state, buffer_state value) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (detail::has_ordered_state<TState>::value)
        {
            state.store(static_cast<uint8_t>(value), castle::memory_order_release);
        }
        else
        {
            state = static_cast<uint8_t>(value);
        }
    }

    static size_type next_index(size_type index) CASTLE_NOEXCEPT
    {
        ++index;
        return index == N_BUFFERS ? 0U : index;
    }

    static uint32_t next_generation(uint32_t generation) CASTLE_NOEXCEPT
    {
        ++generation;
        return generation == 0U ? 1U : generation;
    }

    pointer buffers_;
    castle::container::array<buffer_slot, N_BUFFERS> descriptor_items_;
    size_type write_index_;
    size_type read_index_;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_SPSC_BUFFER_DESC_H
