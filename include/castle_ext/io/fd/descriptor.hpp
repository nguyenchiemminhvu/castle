// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file descriptor.hpp
 * @brief Allocation-free POSIX file-descriptor operations and move-only ownership.
 *
 * This is the common transport primitive used by Linux files, UART, SPI and
 * I2C. The free functions perform one POSIX operation (unless explicitly named
 * `*_all`) and report the transferred byte count. `basic_fd` owns one descriptor
 * and closes it on destruction. No descriptor is shared or reference-counted.
 */
#ifndef CASTLE_EXT_IO_FD_DESCRIPTOR_HPP
#define CASTLE_EXT_IO_FD_DESCRIPTOR_HPP

#include "castle/core/config.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/fd/control.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <limits.h>
    #include <unistd.h>
#endif

namespace castle
{
namespace io
{
namespace fd
{

/** @brief True when descriptor open/read/write operations are available. */
static CASTLE_CONSTEXPR bool descriptor_available = (CASTLE_USING_POSIX_APIS != 0);


/**
 * @brief Opens a path and returns its descriptor through `out_fd`.
 * @param path Null-terminated path; must not be null or empty.
 * @param flags Native POSIX `open()` flags (for example `O_RDWR | O_NOCTTY`).
 * @param out_fd Receives the new descriptor, or -1 on failure.
 * @param permissions Creation permissions, used when `O_CREAT` is present.
 * @param system_error Optional output receiving `errno` for system-call failures.
 * @return `ok`, `invalid_argument`, `system_call_error`, or `not_configured`.
 */
CASTLE_INLINE castle::status open_fd(CASTLE_CONST char* path,
                              int flags,
                              int& out_fd,
                              unsigned int permissions = 0666U,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
{
    out_fd = -1;
    detail::clear_system_error(system_error);
    if (path == nullptr || path[0] == '\0')
    {
        return castle::status::invalid_argument;
    }
#if CASTLE_USING_POSIX_APIS
    CASTLE_CONST int result = ::open(path, flags, static_cast<mode_t>(permissions));
    if (result < 0)
    {
        detail::capture_system_error(system_error);
        return castle::status::system_call_error;
    }
    out_fd = result;
    return castle::status::ok;
#else
    (void)flags;
    (void)permissions;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Performs one POSIX `read()` operation.
 * @param descriptor Open descriptor.
 * @param buffer Destination buffer; may be null only when `capacity` is zero.
 * @param capacity Maximum number of bytes to read.
 * @param bytes_read Receives the byte count (zero on error or end-of-file).
 * @param system_error Optional output receiving `errno` on a system-call failure.
 * @return `ok` when bytes were read, `empty` on EOF/non-blocking no-data,
 *         `invalid_argument`, `system_call_error`, or `not_configured`.
 */
CASTLE_INLINE castle::status read_fd(int descriptor,
                              void* buffer,
                              size_type capacity,
                              size_type& bytes_read,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
{
    bytes_read = 0U;
    detail::clear_system_error(system_error);
    if (descriptor < 0 || (buffer == nullptr && capacity != 0U))
    {
        return castle::status::invalid_argument;
    }
    if (capacity == 0U)
    {
        return castle::status::ok;
    }
#if CASTLE_USING_POSIX_APIS
    if (capacity > static_cast<size_type>(SSIZE_MAX))
    {
        return castle::status::out_of_range;
    }
    CASTLE_CONST ssize_t result = ::read(descriptor, buffer, capacity);
    if (result < 0)
    {
        detail::capture_system_error(system_error);
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return castle::status::empty;
        }
        return castle::status::system_call_error;
    }
    bytes_read = static_cast<size_type>(result);
    return (result == 0) ? castle::status::empty : castle::status::ok;
#else
    (void)buffer;
    (void)capacity;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Performs one POSIX `write()` operation.
 * @param descriptor Open descriptor.
 * @param buffer Source buffer; may be null only when `size` is zero.
 * @param size Maximum number of bytes to write.
 * @param bytes_written Receives the byte count (zero on error).
 * @param system_error Optional output receiving `errno` on a system-call failure.
 * @return `ok` when bytes were written, `full` when a non-blocking descriptor
 *         cannot currently accept data, `invalid_argument`, `system_call_error`,
 *         or `not_configured`.
 */
CASTLE_INLINE castle::status write_fd(int descriptor,
                               CASTLE_CONST void* buffer,
                               size_type size,
                               size_type& bytes_written,
                               int* system_error = nullptr) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    detail::clear_system_error(system_error);
    if (descriptor < 0 || (buffer == nullptr && size != 0U))
    {
        return castle::status::invalid_argument;
    }
    if (size == 0U)
    {
        return castle::status::ok;
    }
#if CASTLE_USING_POSIX_APIS
    if (size > static_cast<size_type>(SSIZE_MAX))
    {
        return castle::status::out_of_range;
    }
    CASTLE_CONST ssize_t result = ::write(descriptor, buffer, size);
    if (result < 0)
    {
        detail::capture_system_error(system_error);
        if (errno == EAGAIN || errno == EWOULDBLOCK) // LCOV_EXCL_BR_LINE
        {
            return castle::status::full;
        }
        return castle::status::system_call_error;
    }
    bytes_written = static_cast<size_type>(result);
    return (result == 0) ? castle::status::full : castle::status::ok; // LCOV_EXCL_BR_LINE
#else
    (void)buffer;
    (void)size;
    return castle::status::not_configured;
#endif
}

/**
 * @brief Reads until `size` bytes arrive or the first non-success result occurs.
 * @note A short read caused by EOF is reported as `data_loss`; the actual count
 *       remains available through `bytes_read`. Non-blocking no-data is `empty`
 *       when no bytes were read and `data_loss` after a partial read.
 */
CASTLE_INLINE castle::status read_exact_fd(int descriptor,
                                    void* buffer,
                                    size_type size,
                                    size_type& bytes_read,
                                    int* system_error = nullptr) CASTLE_NOEXCEPT
{
    bytes_read = 0U;
    detail::clear_system_error(system_error);
    if (descriptor < 0 || (buffer == nullptr && size != 0U)) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_argument;
    }
    while (bytes_read < size)
    {
        size_type count = 0U;
        castle::status result = read_fd(descriptor,
                                        static_cast<unsigned char*>(buffer) + bytes_read,
                                        size - bytes_read,
                                        count,
                                        system_error);
        bytes_read += count;
        if (!castle::succeeded(result))
        {
            if (bytes_read != 0U && (result == castle::status::empty)) // LCOV_EXCL_BR_LINE
            {
                return castle::status::data_loss;
            }
            return result;
        }
    }
    return castle::status::ok;
}

/**
 * @brief Writes until all bytes are accepted or the first non-success result occurs.
 * @note `bytes_written` reports a partial transfer when the descriptor blocks or
 *       an error occurs. The function does not busy-wait or retry `EINTR`.
 */
CASTLE_INLINE castle::status write_all_fd(int descriptor,
                                   CASTLE_CONST void* buffer,
                                   size_type size,
                                   size_type& bytes_written,
                                   int* system_error = nullptr) CASTLE_NOEXCEPT
{
    bytes_written = 0U;
    detail::clear_system_error(system_error);
    if (descriptor < 0 || (buffer == nullptr && size != 0U)) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_argument;
    }
    while (bytes_written < size)
    {
        size_type count = 0U;
        castle::status result = write_fd(descriptor,
                                         static_cast<CASTLE_CONST unsigned char*>(buffer) + bytes_written,
                                         size - bytes_written,
                                         count,
                                         system_error);
        bytes_written += count;
        if (!castle::succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            return result;
        }
    }
    return castle::status::ok;
}

/**
 * @brief Move-only owner of one POSIX file descriptor.
 *
 * The type does not add synchronization. Sharing an instance between threads
 * requires external coordination. Constructing from an integer adopts the
 * descriptor; use `release()` when ownership must be transferred to another API.
 */
class basic_fd
{
public:
    /** @brief Creates an empty owner. */
    CASTLE_CONSTEXPR basic_fd() CASTLE_NOEXCEPT
        : descriptor_(-1)
    {
    }

    /** @brief Adopts a descriptor; ownership transfers to this object. */
    explicit CASTLE_CONSTEXPR basic_fd(int descriptor) CASTLE_NOEXCEPT
        : descriptor_(descriptor >= 0 ? descriptor : -1)
    {
    }

    /** @brief Closes the owned descriptor, if any. */
    ~basic_fd() CASTLE_NOEXCEPT
    {
        if (descriptor_ >= 0)
        {
            (void)close_fd(descriptor_);
        }
    }

    basic_fd(CASTLE_CONST basic_fd&) CASTLE_DELETE;
    basic_fd& operator=(CASTLE_CONST basic_fd&) CASTLE_DELETE;

    /** @brief Transfers descriptor ownership from another owner. */
    basic_fd(basic_fd&& other) CASTLE_NOEXCEPT
        : descriptor_(other.release())
    {
    }

    /** @brief Closes the current descriptor and takes ownership from `other`. */
    basic_fd& operator=(basic_fd&& other) CASTLE_NOEXCEPT
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            (void)close();
            descriptor_ = other.release();
        }
        return *this;
    }

    /** @brief Opens a path only when this owner is currently empty. */
    castle::status open(CASTLE_CONST char* path,
                        int flags,
                        unsigned int permissions = 0666U,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (is_open())
        {
            detail::clear_system_error(system_error);
            return castle::status::invalid_config;
        }
        return open_fd(path, flags, descriptor_, permissions, system_error);
    }

    /** @brief Closes the owned descriptor and makes this object empty. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        detail::clear_system_error(system_error);
        if (descriptor_ < 0)
        {
            return castle::status::not_configured;
        }
        CASTLE_CONST int closing = descriptor_;
        descriptor_ = -1;
        return close_fd(closing, system_error);
    }

    /** @brief Returns true when a descriptor is currently owned. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return descriptor_ >= 0;
    }

    /** @brief Returns the owned descriptor, or -1 when empty. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return descriptor_;
    }

    /** @brief Releases ownership without closing and returns the descriptor. */
    int release() CASTLE_NOEXCEPT
    {
        CASTLE_CONST int result = descriptor_;
        descriptor_ = -1;
        return result;
    }

    /** @brief Replaces the owned descriptor, closing the previous one first. */
    castle::status reset(int descriptor = -1, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (descriptor < -1)
        {
            detail::clear_system_error(system_error);
            return castle::status::invalid_argument;
        }
        if (descriptor == descriptor_)
        {
            detail::clear_system_error(system_error);
            return castle::status::ok;
        }
        
        // LCOV_EXCL_START
        castle::status result = close(system_error);
        if (result != castle::status::ok && result != castle::status::not_configured)
        {
            return result;
        }
        descriptor_ = descriptor;
        return castle::status::ok;
        // LCOV_EXCL_STOP
    }

    /** @brief Performs one read operation on the owned descriptor. */
    castle::status read_some(void* buffer, size_type capacity, size_type& bytes_read,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_read = 0U;
            detail::clear_system_error(system_error);
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
        return read_fd(descriptor_, buffer, capacity, bytes_read, system_error);
    }

    /** @brief Reads until the requested size is reached or a non-success status occurs. */
    castle::status read_exact(void* buffer, size_type size, size_type& bytes_read,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_read = 0U;
            detail::clear_system_error(system_error);
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
        return read_exact_fd(descriptor_, buffer, size, bytes_read, system_error);
    }

    /** @brief Performs one write operation on the owned descriptor. */
    castle::status write_some(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_written = 0U;
            detail::clear_system_error(system_error);
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
        return write_fd(descriptor_, buffer, size, bytes_written, system_error);
    }

    /** @brief Writes until all bytes are accepted or a non-success status occurs. */
    castle::status write_all(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_written = 0U;
            detail::clear_system_error(system_error);
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
        return write_all_fd(descriptor_, buffer, size, bytes_written, system_error);
    }

    /** @brief Enables or disables non-blocking mode on the owned descriptor. */
    castle::status set_nonblocking(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            detail::clear_system_error(system_error);
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
        return fd::set_nonblocking(descriptor_, enabled, system_error);
    }

    /** @brief Enables or disables close-on-exec on the owned descriptor. */
    castle::status set_close_on_exec(bool enabled = true, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            detail::clear_system_error(system_error);
            return castle::status::not_configured;
        }
        // LCOV_EXCL_STOP
        return fd::set_close_on_exec(descriptor_, enabled, system_error);
    }

private:
    int descriptor_;
};

using descriptor = basic_fd;

} // namespace fd
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_FD_DESCRIPTOR_HPP
