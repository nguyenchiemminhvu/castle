// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file poll.hpp
 * @brief Fixed-capacity, thread-safe POSIX file-descriptor event dispatcher.
 *
 * `basic_fd_event_manager<Capacity>` stores registrations in a compile-time-sized
 * table and uses `poll()` without allocating memory. Registration operations are
 * protected by CASTLE's mutex. Each wait builds a descriptor snapshot, releases
 * the registration lock while `poll()` blocks, then validates registration
 * generations before queuing callbacks. Callbacks are invoked outside every
 * manager lock, so they may safely add, remove, enable, or disable descriptors.
 *
 * A successful `wait()` queues ready callbacks; `process_events()` dispatches
 * that queue. `wait_and_process()` combines both operations. Only one `wait()`
 * may be active per manager, and a second wait is rejected while previous ready
 * callbacks remain unprocessed.
 *
 * @code
 * #include "castle_ext/io/fd/poll.hpp"
 *
 * castle::io::fd::fd_event_manager manager;
 * manager.add_fd(fd, castle::io::fd::poll_read,
 *     [](int ready_fd, short revents, void* context)
 *     {
 *         // Handle the event. `context` is user-owned.
 *     }, context);
 * castle::size_type dispatched = 0U;
 * manager.wait_and_process(1000, dispatched);
 * @endcode
 */
#ifndef CASTLE_EXT_IO_FD_POLL_HPP
#define CASTLE_EXT_IO_FD_POLL_HPP

#include "castle/core/config.hpp"
#include "castle_ext/io/fd/control.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/sync/mutex.hpp"
#include "castle/sync/scoped_mutex.hpp"
#include "castle/utility/move.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <poll.h>
    #include <stdint.h>
#endif

namespace castle
{
namespace io
{
namespace fd
{

/** @brief True when POSIX `poll()` support is enabled for this target. */
static CASTLE_CONSTEXPR bool poll_available = (CASTLE_USING_POSIX_APIS != 0);

#if CASTLE_USING_POSIX_APIS

/** @brief `poll()` interest bit for readable data (`POLLIN`). */
static CASTLE_CONSTEXPR short poll_read = POLLIN;
/** @brief `poll()` interest bit for writable data (`POLLOUT`). */
static CASTLE_CONSTEXPR short poll_write = POLLOUT;
/** @brief `poll()` interest bit for priority data (`POLLPRI`). */
static CASTLE_CONSTEXPR short poll_priority = POLLPRI;
#ifdef POLLRDNORM
/** @brief `poll()` interest bit for normal readable data where supported. */
static CASTLE_CONSTEXPR short poll_read_normal = POLLRDNORM;
#endif
#ifdef POLLRDBAND
/** @brief `poll()` interest bit for priority-band readable data where supported. */
static CASTLE_CONSTEXPR short poll_read_band = POLLRDBAND;
#endif
#ifdef POLLWRNORM
/** @brief `poll()` interest bit for normal writable data where supported. */
static CASTLE_CONSTEXPR short poll_write_normal = POLLWRNORM;
#endif
#ifdef POLLWRBAND
/** @brief `poll()` interest bit for priority-band writable data where supported. */
static CASTLE_CONSTEXPR short poll_write_band = POLLWRBAND;
#endif
#ifdef POLLMSG
/** @brief `poll()` interest bit for message availability where supported. */
static CASTLE_CONSTEXPR short poll_message = POLLMSG;
#endif
#ifdef POLLREMOVE
/** @brief `poll()` event bit indicating a removed descriptor where supported. */
static CASTLE_CONSTEXPR short poll_remove = POLLREMOVE;
#endif
/** @brief Returned event bit indicating an error (`POLLERR`). */
static CASTLE_CONSTEXPR short poll_error = POLLERR;
/** @brief Returned event bit indicating a hang-up (`POLLHUP`). */
static CASTLE_CONSTEXPR short poll_hangup = POLLHUP;
/** @brief Returned event bit indicating an invalid descriptor (`POLLNVAL`). */
static CASTLE_CONSTEXPR short poll_invalid = POLLNVAL;
#ifdef POLLRDHUP
/** @brief Returned event bit indicating a peer write-side shutdown where supported. */
static CASTLE_CONSTEXPR short poll_read_hangup = POLLRDHUP;
#endif

/**
 * @brief Waits for a single descriptor to report one of the requested events.
 * @param fd Non-negative descriptor to wait on.
 * @param events `poll()` interest mask, e.g. `poll_read` or `poll_write`.
 * @param timeout_ms Wait time in milliseconds: zero checks without blocking, a
 *        negative value waits indefinitely.
 * @param revents Receives the events reported by `poll()`; zero on timeout or error.
 * @param system_error Optional output receiving `errno` when `poll()` fails.
 * @return `ok` when the descriptor is ready (inspect `revents` for `poll_error`/
 *         `poll_hangup`), `time_out` when the timeout expired, `invalid_argument`
 *         or `system_call_error`.
 * @note Interrupted waits (`EINTR`) are reported as `system_call_error` and are
 *       not retried, which keeps the caller in control of the remaining timeout.
 */
CASTLE_INLINE castle::status wait_fd(int fd,
                                     short events,
                                     int timeout_ms,
                                     short& revents,
                                     int* system_error = nullptr) CASTLE_NOEXCEPT
{
    revents = 0;
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }

    struct pollfd descriptor;
    descriptor.fd = fd;
    descriptor.events = events;
    descriptor.revents = 0;
    CASTLE_CONST int result = ::poll(&descriptor, 1U, timeout_ms);
    if (result < 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    if (result == 0)
    {
        return castle::status::time_out;
    }
    revents = descriptor.revents;
    return castle::status::ok;
}

/**
 * @brief Compile-time-capacity file descriptor event manager.
 * @tparam Capacity Maximum number of simultaneous descriptor registrations.
 * @tparam CallbackStorageSize Inline bytes reserved for each stored callback.
 *
 * Callbacks use `void(int fd, short revents, void* user_data)`. They are stored
 * in `castle::callbacks::function` and therefore must be copy-constructible and
 * fit the configured inline storage. A copy of the callback is queued for each
 * dispatch; place persistent mutable callback state in `user_data` or capture a
 * stable pointer/reference to it.
 *
 * The manager never owns or closes registered descriptors. Names and user-data
 * pointers are non-owning. Registrations are looked up linearly, so add/remove/
 * update operations are O(Capacity); polling is O(number of enabled descriptors)
 * plus the kernel's `poll()` cost.
 */
template <castle::size_type Capacity = 16U,
          castle::size_type CallbackStorageSize = castle::inplace_storage_reserved>
class basic_fd_event_manager
{
    static_assert(Capacity > 0U, "FD event manager capacity must be greater than zero");
    static_assert(Capacity <= static_cast<castle::size_type>(static_cast<nfds_t>(-1)),
                  "FD event manager capacity exceeds the poll() descriptor-count range");
    static_assert(CallbackStorageSize > 0U, "FD event callback storage must not be empty");

public:
    using size_type = castle::size_type;
    using callback_type = castle::callbacks::function<void(int, short, void*), CallbackStorageSize>;

    /** @brief Read-only snapshot of a registration's public metadata. */
    struct registration_info
    {
        int fd;
        short events;
        void* user_data;
        CASTLE_CONST char* name;
        bool enabled;
    };

    /** @brief Constructs an empty manager. */
    basic_fd_event_manager() CASTLE_DEFAULT;

    /** @brief Clears registrations and queued events before destroying the manager. */
    ~basic_fd_event_manager() CASTLE_NOEXCEPT
    {
        clear();
    } // LCOV_EXCL_BR_LINE

    basic_fd_event_manager(CASTLE_CONST basic_fd_event_manager&) CASTLE_DELETE;
    basic_fd_event_manager& operator=(CASTLE_CONST basic_fd_event_manager&) CASTLE_DELETE;
    basic_fd_event_manager(basic_fd_event_manager&&) CASTLE_DELETE;
    basic_fd_event_manager& operator=(basic_fd_event_manager&&) CASTLE_DELETE;

    /**
     * @brief Registers a descriptor and its callback.
     * @param fd Non-negative descriptor. The manager does not take ownership.
     * @param events `poll()` interest mask, e.g. `poll_read | poll_write`.
     * @param callback Copyable callable accepting `(int, short, void*)`.
     * @param user_data Non-owning pointer passed unchanged to each callback.
     * @param name Optional non-owning, null-terminated diagnostic name.
     * @return `ok`, `invalid_argument`, `invalid_callback`, `already_exists`, or `full`.
     */
    castle::status add_fd(int fd,
                          short events,
                          callback_type callback,
                          void* user_data = nullptr,
                          CASTLE_CONST char* name = nullptr)
    {
        if (fd < 0)
        {
            return castle::status::invalid_argument;
        }
        if (!callback)
        {
            return castle::status::invalid_callback;
        }

        castle::scoped_mutex guard(state_mutex_);
        if (find_slot_unlocked(fd) != Capacity)
        {
            return castle::status::already_exists;
        }

        CASTLE_CONST size_type free_slot = find_free_slot_unlocked();
        if (free_slot == Capacity)
        {
            return castle::status::full;
        }

        registration& slot = registrations_[free_slot];
        slot.fd = fd;
        slot.events = events;
        slot.callback = castle::move(callback);
        slot.user_data = user_data;
        slot.name = name;
        slot.enabled = true;
        slot.occupied = true;
        slot.generation = next_generation(slot.generation);
        ++fd_count_;
        ++enabled_fd_count_;
        return castle::status::ok;
    }

    /**
     * @brief Removes a descriptor registration without closing the descriptor.
     * @param fd Descriptor to unregister.
     * @return `ok` when removed, otherwise `not_found`.
     * @note A callback already queued by `wait()` remains in that batch; call
     *       `clear()` to discard all queued callbacks before closing its descriptor.
     */
    castle::status remove_fd(int fd) CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        CASTLE_CONST size_type index = find_slot_unlocked(fd);
        if (index == Capacity)
        {
            return castle::status::not_found;
        }

        registration& slot = registrations_[index];
        CASTLE_CONST bool was_enabled = slot.enabled;
        slot.callback = nullptr;
        slot.fd = -1;
        slot.events = 0;
        slot.user_data = nullptr;
        slot.name = nullptr;
        slot.enabled = false;
        slot.occupied = false;
        slot.generation = next_generation(slot.generation);
        --fd_count_;
        if (was_enabled) // LCOV_EXCL_BR_LINE
        {
            --enabled_fd_count_;
        }
        return castle::status::ok;
    }

    /**
     * @brief Enables a registered descriptor for the next wait snapshot.
     * @return `ok` when found, otherwise `not_found`.
     */
    castle::status enable_fd(int fd) CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        CASTLE_CONST size_type index = find_slot_unlocked(fd);
        if (index == Capacity)
        {
            return castle::status::not_found;
        }
        registration& slot = registrations_[index];
        if (!slot.enabled)
        {
            slot.enabled = true;
            slot.generation = next_generation(slot.generation);
            ++enabled_fd_count_;
        }
        return castle::status::ok;
    }

    /**
     * @brief Disables a registration without removing it.
     * @return `ok` when found, otherwise `not_found`.
     * @note A callback already queued by `wait()` remains in that batch.
     */
    castle::status disable_fd(int fd) CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        CASTLE_CONST size_type index = find_slot_unlocked(fd);
        if (index == Capacity)
        {
            return castle::status::not_found;
        }
        registration& slot = registrations_[index];
        if (slot.enabled)
        {
            slot.enabled = false;
            slot.generation = next_generation(slot.generation);
            --enabled_fd_count_;
        }
        return castle::status::ok;
    }

    /**
     * @brief Replaces the interest mask of a registration.
     * @return `ok` when found, otherwise `not_found`.
     * @note A callback already queued by `wait()` remains in that batch.
     */
    castle::status modify_events(int fd, short events) CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        CASTLE_CONST size_type index = find_slot_unlocked(fd);
        if (index == Capacity)
        {
            return castle::status::not_found;
        }
        registration& slot = registrations_[index];
        if (slot.events != events)
        {
            slot.events = events;
            slot.generation = next_generation(slot.generation);
        }
        return castle::status::ok;
    }

    /**
     * @brief Tests whether a descriptor is registered, enabled or disabled.
     */
    bool has_fd(int fd) CASTLE_CONST CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        return find_slot_unlocked(fd) != Capacity;
    }

    /** @brief Returns the number of active registrations. */
    size_type get_fd_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        return fd_count_;
    }

    /** @brief Returns the number of enabled registrations. */
    size_type get_enabled_fd_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        return enabled_fd_count_;
    }

    /**
     * @brief Retrieves registration metadata without exposing the callback wrapper.
     * @param fd Descriptor to query.
     * @param info Receives metadata on success; unchanged when not found.
     * @return `ok` when found, otherwise `not_found`.
     */
    castle::status get_fd_info(int fd, registration_info& info) CASTLE_CONST CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        CASTLE_CONST size_type index = find_slot_unlocked(fd);
        if (index == Capacity)
        {
            return castle::status::not_found;
        }
        CASTLE_CONST registration& slot = registrations_[index];
        info.fd = slot.fd;
        info.events = slot.events;
        info.user_data = slot.user_data;
        info.name = slot.name;
        info.enabled = slot.enabled;
        return castle::status::ok;
    }

    /**
     * @brief Waits for descriptor readiness and queues callbacks without invoking them.
     * @param timeout_ms Timeout in milliseconds; a negative value means no timeout.
     * @param ready_count Receives the number of still-current ready registrations.
     * @param system_error Optional output receiving `errno` when `poll()` fails.
     * @return `ok` for readiness or timeout, `invalid_config` when a previous
     *         ready batch is pending or another wait is active, or `system_call_error`.
     * @note With no enabled descriptors this returns immediately with zero events.
     */
    castle::status wait(int timeout_ms, size_type& ready_count, int* system_error = nullptr)
    {
        ready_count = 0U;
        // LCOV_EXCL_START
        if (system_error != nullptr)
        {
            *system_error = 0;
        }

        // A blocking wait must never make a second waiter spin on a held mutex.
        if (!wait_mutex_.try_lock())
        {
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP

        struct wait_unlocker
        {
            explicit wait_unlocker(castle::mutex& value) : mutex(value) {}
            ~wait_unlocker() CASTLE_NOEXCEPT { mutex.unlock(); }
            castle::mutex& mutex;
        } unlocker(wait_mutex_);

        size_type poll_count = 0U;
        {
            castle::scoped_mutex guard(state_mutex_);
            if (pending_count_ != 0U)
            {
                return castle::status::invalid_config;
            }
            for (size_type i = 0U; i < Capacity; ++i)
            {
                CASTLE_CONST registration& slot = registrations_[i];
                if (slot.occupied && slot.enabled)
                {
                    poll_descriptors_[poll_count].fd = slot.fd;
                    poll_descriptors_[poll_count].events = slot.events;
                    poll_descriptors_[poll_count].revents = 0;
                    poll_slot_indices_[poll_count] = i;
                    poll_generations_[poll_count] = slot.generation;
                    ++poll_count;
                }
            }
        }

        if (poll_count == 0U)
        {
            return castle::status::ok;
        }

        CASTLE_CONST int poll_result = ::poll(poll_descriptors_, static_cast<nfds_t>(poll_count), timeout_ms);
        // LCOV_EXCL_START
        if (poll_result < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        // LCOV_EXCL_STOP
        if (poll_result == 0)
        {
            return castle::status::time_out;
        }

        {
            castle::scoped_mutex guard(state_mutex_);
            for (size_type i = 0U; i < poll_count; ++i)
            {
                CASTLE_CONST short revents = poll_descriptors_[i].revents;
                // LCOV_EXCL_START
                if (revents == 0)
                {
                    continue;
                }
                // LCOV_EXCL_STOP

                CASTLE_CONST size_type slot_index = poll_slot_indices_[i];
                registration& slot = registrations_[slot_index];
                if (!slot.occupied
                 || !slot.enabled                           // LCOV_EXCL_BR_LINE
                 || slot.fd != poll_descriptors_[i].fd      // LCOV_EXCL_BR_LINE
                 || slot.generation != poll_generations_[i] // LCOV_EXCL_BR_LINE
                 || !slot.callback)                         // LCOV_EXCL_BR_LINE
                {
                    continue; // Registration changed while poll() was blocked.
                }

                pending_event& pending = pending_[pending_count_];
                pending.fd = slot.fd;
                pending.revents = revents;
                pending.user_data = slot.user_data;
                pending.callback = slot.callback;
                ++pending_count_;
            }
            ready_count = pending_count_;
        }
        return castle::status::ok;
    }

    /**
     * @brief Invokes and consumes the callbacks queued by the latest successful wait.
     * @param processed_count Receives the number of callbacks invoked.
     * @return `ok`; callback code is responsible for its own application-level errors.
     * @note The manager lock is not held while user code runs.
     */
    castle::status process_events(size_type& processed_count)
    {
        processed_count = 0U;
        pending_event local_events[Capacity];
        size_type local_count = 0U;
        {
            castle::scoped_mutex guard(state_mutex_);
            local_count = pending_count_;
            for (size_type i = 0U; i < local_count; ++i)
            {
                local_events[i] = castle::move(pending_[i]);
                pending_[i].fd = -1;
                pending_[i].revents = 0;
                pending_[i].user_data = nullptr;
            }
            pending_count_ = 0U;
        }

        for (size_type i = 0U; i < local_count; ++i)
        {
            if (local_events[i].callback) // LCOV_EXCL_BR_LINE
            {
                local_events[i].callback(local_events[i].fd,
                                         local_events[i].revents,
                                         local_events[i].user_data);
                ++processed_count;
            }
        }
        return castle::status::ok;
    }

    /** @brief Convenience overload that dispatches pending callbacks without a count output. */
    castle::status process_events()
    {
        size_type processed_count = 0U;
        return process_events(processed_count);
    }

    /**
     * @brief Waits once and immediately invokes every ready callback.
     * @param timeout_ms Timeout in milliseconds; a negative value means infinite.
     * @param processed_count Receives the number of callbacks invoked.
     * @param system_error Optional output receiving `errno` from a failed `poll()`.
     * @return The status from `wait()` or `process_events()`.
     */
    castle::status wait_and_process(int timeout_ms,
                                    size_type& processed_count,
                                    int* system_error = nullptr)
    {
        processed_count = 0U;
        size_type ready_count = 0U;
        CASTLE_CONST castle::status result = wait(timeout_ms, ready_count, system_error);
        // LCOV_EXCL_START
        if (!castle::succeeded(result))
        {
            return result;
        }
        // LCOV_EXCL_STOP
        return process_events(processed_count);
    }

    /** @brief Convenience overload that waits indefinitely and discards the dispatch count. */
    castle::status wait_and_process()
    {
        size_type processed_count = 0U;
        return wait_and_process(-1, processed_count, nullptr);
    }

    /**
     * @brief Removes every registration and drops any callbacks that have not been dispatched.
     * @note Descriptors themselves remain open; `clear()` only affects manager state.
     */
    void clear() CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        for (size_type i = 0U; i < Capacity; ++i)
        {
            registration& slot = registrations_[i];
            slot.callback = nullptr;
            slot.fd = -1;
            slot.events = 0;
            slot.user_data = nullptr;
            slot.name = nullptr;
            slot.enabled = false;
            slot.occupied = false;
            slot.generation = next_generation(slot.generation);

            pending_[i].callback = nullptr;
            pending_[i].fd = -1;
            pending_[i].revents = 0;
            pending_[i].user_data = nullptr;
        }
        fd_count_ = 0U;
        enabled_fd_count_ = 0U;
        pending_count_ = 0U;
    }

    /** @brief Returns the number of ready callbacks waiting for `process_events()`. */
    size_type pending_event_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        castle::scoped_mutex guard(state_mutex_);
        return pending_count_;
    }

private:
    struct registration
    {
        int fd = -1;
        short events = 0;
        void* user_data = nullptr;
        CASTLE_CONST char* name = nullptr;
        bool enabled = false;
        bool occupied = false;
        uint32_t generation = 0U;
        callback_type callback;
    };

    struct pending_event
    {
        int fd = -1;
        short revents = 0;
        void* user_data = nullptr;
        callback_type callback;
    };

    static uint32_t next_generation(uint32_t value) CASTLE_NOEXCEPT
    {
        ++value;
        return (value == 0U) ? 1U : value; // LCOV_EXCL_BR_LINE
    }

    size_type find_slot_unlocked(int fd) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Capacity; ++i)
        {
            if (registrations_[i].occupied && registrations_[i].fd == fd)
            {
                return i;
            }
        }
        return Capacity;
    }

    size_type find_free_slot_unlocked() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Capacity; ++i)
        {
            if (!registrations_[i].occupied)
            {
                return i;
            }
        }
        return Capacity;
    }

    mutable castle::mutex state_mutex_;
    castle::mutex wait_mutex_;
    registration registrations_[Capacity];
    pending_event pending_[Capacity];
    struct pollfd poll_descriptors_[Capacity];
    size_type poll_slot_indices_[Capacity]{};
    uint32_t poll_generations_[Capacity]{};
    size_type fd_count_ = 0U;
    size_type enabled_fd_count_ = 0U;
    size_type pending_count_ = 0U;
};

/** @brief Convenience manager with a fixed default capacity of 16 descriptors. */
using fd_event_manager = basic_fd_event_manager<>;

#else // CASTLE_USING_POSIX_APIS

/**
 * @brief POSIX poll support is unavailable for this target.
 * @note File-descriptor control functions remain available as status-returning stubs.
 */
static CASTLE_CONSTEXPR bool poll_requires_posix = true;

#endif // CASTLE_USING_POSIX_APIS

} // namespace fd
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_FD_POLL_HPP
