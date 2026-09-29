// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file control.hpp
 * @brief Allocation-free helpers for common POSIX file-descriptor flags and control.
 *
 * The functions in this header translate POSIX `fcntl()` and `close()` operations
 * into `castle::status` results. They preserve unrelated descriptor/status bits,
 * expose the platform error number through an optional output pointer, and do not
 * allocate memory or take ownership of descriptors except for `close_fd()`.
 *
 * POSIX declarations and headers are enabled only when
 * `CASTLE_USING_POSIX_APIS` is true. On other targets the same API is available
 * and reports `castle::status::not_configured`.
 *
 * @code
 * #include "castle_ext/io/fd/control.hpp"
 *
 * int flags = 0;
 * if (castle::io::fd::get_status_flags(fd, flags) == castle::status::ok)
 * {
 *     (void)castle::io::fd::set_nonblocking(fd);
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_IO_FD_CONTROL_HPP
#define CASTLE_EXT_IO_FD_CONTROL_HPP

#include "castle/core/config.hpp"
#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace castle
{
namespace io
{
namespace fd
{

/** @brief True when this translation unit has the POSIX descriptor APIs. */
static CASTLE_CONSTEXPR bool posix_available = (CASTLE_USING_POSIX_APIS != 0);

namespace detail
{

/** @brief Clears an optional system-error output before an operation. */
CASTLE_INLINE void clear_system_error(int* system_error) CASTLE_NOEXCEPT
{
    if (system_error != nullptr)
    {
        *system_error = 0;
    }
}

#if CASTLE_USING_POSIX_APIS
/** @brief Copies the current errno value into an optional output location. */
CASTLE_INLINE void capture_system_error(int* system_error) CASTLE_NOEXCEPT
{
    if (system_error != nullptr)
    {
        *system_error = errno;
    }
}
#endif

} // namespace detail

/**
 * @brief Reads the file status flags of a descriptor (`F_GETFL`).
 * @param fd Descriptor to query; must be non-negative.
 * @param flags Receives the flags on success, otherwise reset to zero.
 * @param system_error Optional output receiving `errno` when the system call fails.
 * @return `ok`, `invalid_argument`, `system_call_error`, or `not_configured`.
 */
CASTLE_INLINE castle::status get_status_flags(int fd, int& flags, int* system_error = nullptr) CASTLE_NOEXCEPT
{
    flags = 0;
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
#if CASTLE_USING_POSIX_APIS
    CASTLE_CONST int result = ::fcntl(fd, F_GETFL);
    if (result < 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    flags = result;
    return castle::status::ok;
#else
    (void)fd;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Replaces the file status flags of a descriptor (`F_SETFL`).
 * @param fd Descriptor to update; must be non-negative.
 * @param flags Complete status-flag value supplied to `F_SETFL`.
 * @param system_error Optional output receiving `errno` when the system call fails.
 * @return `ok`, `invalid_argument`, `system_call_error`, or `not_configured`.
 * @note Callers should normally use `set_nonblocking()` to preserve unrelated bits.
 */
CASTLE_INLINE castle::status set_status_flags(int fd, int flags, int* system_error = nullptr) CASTLE_NOEXCEPT
{
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
#if CASTLE_USING_POSIX_APIS
    if (::fcntl(fd, F_SETFL, flags) < 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    return castle::status::ok;
#else
    (void)fd;
    (void)flags;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Enables or disables `O_NONBLOCK` while preserving other status flags.
 * @param fd Descriptor to update.
 * @param enabled True to enable non-blocking mode; false to disable it.
 * @param system_error Optional output receiving `errno` on a failed `fcntl()`.
 * @return The status from the required `fcntl()` operations.
 * @note The read-modify-write sequence is not atomic against other code changing
 *       the same descriptor flags; externally coordinate such updates when needed.
 */
CASTLE_INLINE castle::status set_nonblocking(int fd, bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
{
#if CASTLE_USING_POSIX_APIS
    int flags = 0;
    castle::status result = get_status_flags(fd, flags, system_error);
    if (!castle::succeeded(result))
    {
        return result;
    }
    CASTLE_CONST int updated = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return (updated == flags) ? castle::status::ok : set_status_flags(fd, updated, system_error);
#else
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
    (void)enabled;
    return castle::status::not_configured;
#endif
}

/** @brief Disables `O_NONBLOCK` while preserving every other file status flag. */
CASTLE_INLINE castle::status set_blocking(int fd, int* system_error = nullptr) CASTLE_NOEXCEPT
{
    return set_nonblocking(fd, false, system_error);
}

/**
 * @brief Reads descriptor flags (`F_GETFD`).
 * @param fd Descriptor to query.
 * @param flags Receives descriptor flags on success, otherwise reset to zero.
 * @param system_error Optional output receiving `errno` on a failed `fcntl()`.
 * @return `ok`, `invalid_argument`, `system_call_error`, or `not_configured`.
 */
CASTLE_INLINE castle::status get_descriptor_flags(int fd, int& flags, int* system_error = nullptr) CASTLE_NOEXCEPT
{
    flags = 0;
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
#if CASTLE_USING_POSIX_APIS
    CASTLE_CONST int result = ::fcntl(fd, F_GETFD);
    if (result < 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    flags = result;
    return castle::status::ok;
#else
    (void)fd;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Replaces descriptor flags (`F_SETFD`).
 * @param fd Descriptor to update.
 * @param flags Complete descriptor-flag value supplied to `F_SETFD`.
 * @param system_error Optional output receiving `errno` on a failed `fcntl()`.
 * @return `ok`, `invalid_argument`, `system_call_error`, or `not_configured`.
 */
CASTLE_INLINE castle::status set_descriptor_flags(int fd, int flags, int* system_error = nullptr) CASTLE_NOEXCEPT
{
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
#if CASTLE_USING_POSIX_APIS
    if (::fcntl(fd, F_SETFD, flags) < 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    return castle::status::ok;
#else
    (void)fd;
    (void)flags;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Enables or disables `FD_CLOEXEC`, preserving other descriptor flags.
 * @param fd Descriptor to update.
 * @param enabled True to set close-on-exec; false to clear it.
 * @param system_error Optional output receiving `errno` on a failed `fcntl()`.
 * @return The status from the required `fcntl()` operations.
 */
CASTLE_INLINE castle::status set_close_on_exec(int fd, bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
{
#if CASTLE_USING_POSIX_APIS
    int flags = 0;
    castle::status result = get_descriptor_flags(fd, flags, system_error);
    if (!castle::succeeded(result))
    {
        return result;
    }
    CASTLE_CONST int updated = enabled ? (flags | FD_CLOEXEC) : (flags & ~FD_CLOEXEC);
    return (updated == flags) ? castle::status::ok : set_descriptor_flags(fd, updated, system_error);
#else
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
    (void)enabled;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Tests whether a descriptor currently refers to an open file description.
 * @param fd Descriptor to test.
 * @return True when `F_GETFD` succeeds; false for invalid descriptors or non-POSIX targets.
 */
CASTLE_INLINE bool is_valid_fd(int fd) CASTLE_NOEXCEPT
{
    int flags = 0;
    return castle::succeeded(get_descriptor_flags(fd, flags));
}

/**
 * @brief Closes a descriptor using one POSIX `close()` call.
 * @param fd Descriptor to close; must be non-negative.
 * @param system_error Optional output receiving `errno` when `close()` fails.
 * @return `ok`, `invalid_argument`, `system_call_error`, or `not_configured`.
 * @warning The descriptor is not retried after `EINTR`: on some systems its state
 *          is unspecified, and retrying can close a newly reused descriptor.
 */
CASTLE_INLINE castle::status close_fd(int fd, int* system_error = nullptr) CASTLE_NOEXCEPT
{
    detail::clear_system_error(system_error);
    if (fd < 0)
    {
        return castle::status::invalid_argument;
    }
#if CASTLE_USING_POSIX_APIS
    if (::close(fd) != 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    return castle::status::ok;
#else
    return castle::status::not_configured;
#endif
}

} // namespace fd
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_FD_CONTROL_HPP
