// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file linux_file.hpp
 * @brief Allocation-free RAII access to Linux/POSIX files and device nodes.
 *
 * `basic_file` is a small file facade over `fd::basic_fd`. Paths are borrowed
 * null-terminated strings; no strings, streams, filesystem objects or heap
 * storage are used. A file object owns its descriptor and is move-only.
 */
#ifndef CASTLE_EXT_IO_FILE_LINUX_FILE_HPP
#define CASTLE_EXT_IO_FILE_LINUX_FILE_HPP

#include "castle/core/config.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/io/fd/descriptor.hpp"

#include <stdint.h>

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

namespace castle
{
namespace io
{
namespace file
{

/** @brief File seek origin corresponding to the POSIX `lseek()` origins. */
enum class seek_origin : unsigned char
{
    begin = 0U,
    current,
    end
};

/** @brief Fixed-size POSIX file-open configuration. */
struct file_config
{
    CASTLE_CONST char* path;
    bool readable;
    bool writable;
    bool create;
    bool truncate;
    bool append;
    bool exclusive;
    bool non_blocking;
    unsigned int permissions;

    CASTLE_CONSTEXPR explicit file_config(
        CASTLE_CONST char* file_path = nullptr,
        bool allow_read = true,
        bool allow_write = false,
        bool create_if_missing = false,
        bool truncate_existing = false,
        bool append_writes = false,
        bool fail_if_exists = false,
        bool use_non_blocking = false,
        unsigned int create_permissions = 0666U) CASTLE_NOEXCEPT
        : path(file_path)
        , readable(allow_read)
        , writable(allow_write)
        , create(create_if_missing)
        , truncate(truncate_existing)
        , append(append_writes)
        , exclusive(fail_if_exists)
        , non_blocking(use_non_blocking)
        , permissions(create_permissions)
    {
    }
};

/** @brief Validates file access flags without making a system call. */
CASTLE_INLINE castle::status validate_config(file_config CASTLE_CONST& config) CASTLE_NOEXCEPT
{
    if (config.path == nullptr || config.path[0] == '\0' || (!config.readable && !config.writable))
    {
        return castle::status::invalid_argument;
    }
    if ((config.truncate || config.append || config.exclusive) && !config.writable)
    {
        return castle::status::invalid_config;
    }
    if (config.exclusive && !config.create) // LCOV_EXCL_BR_LINE
    {
        return castle::status::invalid_config;
    }
    return castle::status::ok;
}

/**
 * @brief Move-only owner of a POSIX file or device-node descriptor.
 *
 * `read_some`/`write_some` perform one operation and may return a partial byte
 * count. Use `read_exact`/`write_all` when a complete transfer is required.
 * The class is not internally synchronized.
 */
class basic_file
{
public:
    using size_type = castle::size_type;

    /** @brief Constructs an unopened file. */
    basic_file() CASTLE_DEFAULT;

    basic_file(basic_file CASTLE_CONST&) CASTLE_DELETE;
    basic_file& operator=(basic_file CASTLE_CONST&) CASTLE_DELETE;
    basic_file(basic_file&&) CASTLE_DEFAULT;
    basic_file& operator=(basic_file&&) CASTLE_DEFAULT;

    /** @brief Opens this object using a validated file configuration. */
    castle::status open(file_config CASTLE_CONST& config, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        castle::status result = validate_config(config);

        // LCOV_EXCL_START
        if (!castle::succeeded(result))
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return result;
        }
        if (descriptor_.is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP

#if CASTLE_USING_POSIX_APIS
        int flags = 0;
        if (config.readable && config.writable)
        {
            flags |= O_RDWR;
        }
        else if (config.writable)
        {
            flags |= O_WRONLY;
        }
        else
        {
            flags |= O_RDONLY;
        }

        if (config.create)
        {
            flags |= O_CREAT;
        }
        if (config.truncate)
        {
            flags |= O_TRUNC;
        }
        if (config.append) // LCOV_EXCL_BR_LINE
        {
            flags |= O_APPEND;
        }
        if (config.exclusive)
        {
            flags |= O_EXCL;
        }
#ifdef O_CLOEXEC
        flags |= O_CLOEXEC;
#endif
#ifdef O_NONBLOCK
        if (config.non_blocking) // LCOV_EXCL_BR_LINE
        {
            flags |= O_NONBLOCK;
        }
#else
        if (config.non_blocking)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#endif
        result = descriptor_.open(config.path, flags, config.permissions, system_error);
        if (!castle::succeeded(result))
        {
            return result;
        }
#ifndef O_CLOEXEC
        result = descriptor_.set_close_on_exec(true, system_error);
        if (!castle::succeeded(result))
        {
            (void)descriptor_.close();
            return result;
        }
#endif
        config_ = config;
        return castle::status::ok;
#else
        (void)config;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Closes the descriptor owned by this file. */
    castle::status close(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return descriptor_.close(system_error);
    }

    /** @brief Returns whether the file descriptor is open. */
    CASTLE_NODISCARD bool is_open() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.is_open(); }

    /** @brief Returns the native descriptor, or -1 when closed. */
    CASTLE_NODISCARD int native_handle() CASTLE_CONST CASTLE_NOEXCEPT { return descriptor_.native_handle(); }

    /** @brief Returns the last successfully opened configuration. */
    CASTLE_NODISCARD file_config CASTLE_CONST& configuration() CASTLE_CONST CASTLE_NOEXCEPT { return config_; }

    /** @brief Reads up to `capacity` bytes. */
    castle::status read_some(void* buffer, size_type capacity, size_type& bytes_read,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!is_open())
        {
            bytes_read = 0U;
            if (system_error != nullptr) // LCOV_EXCL_BR_LINE
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        if (!config_.readable)
        {
            bytes_read = 0U;
            if (system_error != nullptr) // LCOV_EXCL_BR_LINE
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        return descriptor_.read_some(buffer, capacity, bytes_read, system_error);
    }

    /** @brief Reads exactly `size` bytes unless EOF or an error occurs. */
    castle::status read_exact(void* buffer, size_type size, size_type& bytes_read,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_read = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        if (!config_.readable)
        {
            bytes_read = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP
        return descriptor_.read_exact(buffer, size, bytes_read, system_error);
    }

    /** @brief Writes up to `size` bytes. */
    castle::status write_some(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                              int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_written = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        if (!config_.writable)
        {
            bytes_written = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP
        return descriptor_.write_some(buffer, size, bytes_written, system_error);
    }

    /** @brief Writes all bytes unless an error or non-blocking back-pressure occurs. */
    castle::status write_all(CASTLE_CONST void* buffer, size_type size, size_type& bytes_written,
                             int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            bytes_written = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        if (!config_.writable)
        {
            bytes_written = 0U;
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        // LCOV_EXCL_STOP
        return descriptor_.write_all(buffer, size, bytes_written, system_error);
    }

    /** @brief Changes the current file offset and returns the resulting position. */
    castle::status seek(int64_t offset, seek_origin origin, int64_t& out_position,
                        int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        out_position = -1;
        if (!is_open())
        {
            if (system_error != nullptr) // LCOV_EXCL_BR_LINE
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_POSIX_APIS
        int native_origin = SEEK_SET;
        // LCOV_EXCL_START
        switch (origin)
        {
            case seek_origin::begin:
            {
                native_origin = SEEK_SET;
                break;
            }
            case seek_origin::current:
            {
                native_origin = SEEK_CUR;
                break;
            }
            case seek_origin::end:
            {
                native_origin = SEEK_END;
                break;
            }
            default:
            {
                if (system_error != nullptr)
                {
                    *system_error = 0;
                }
                return castle::status::invalid_argument;
            }
        }
        // LCOV_EXCL_STOP

        CASTLE_CONST off_t native_offset = static_cast<off_t>(offset);

        // LCOV_EXCL_START
        if (static_cast<int64_t>(native_offset) != offset)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::out_of_range;
        }
        CASTLE_CONST off_t result = ::lseek(native_handle(), native_offset, native_origin);
        if (result < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        out_position = static_cast<int64_t>(result);
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
        // LCOV_EXCL_STOP
#else
        (void)offset; (void)origin;
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Returns the current file offset. */
    castle::status tell(int64_t& out_position, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        return seek(0, seek_origin::current, out_position, system_error);
    }

    /** @brief Returns the current file size without changing the file offset. */
    castle::status size(int64_t& out_size, int* system_error = nullptr) CASTLE_CONST CASTLE_NOEXCEPT
    {
        out_size = 0;

        // LCOV_EXCL_START
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_POSIX_APIS
        struct stat info;
        if (::fstat(native_handle(), &info) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (info.st_size < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::data_loss;
        }
        out_size = static_cast<int64_t>(info.st_size);
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::ok;
        // LCOV_EXCL_STOP
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Changes the file length; requires a writable open configuration. */
    castle::status truncate(int64_t length, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
        if (!config_.writable)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_config;
        }
        if (length < 0)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::invalid_argument;
        }
#if CASTLE_USING_POSIX_APIS
        CASTLE_CONST off_t native_length = static_cast<off_t>(length);
        if (static_cast<int64_t>(native_length) != length)
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::out_of_range;
        }
        if (::ftruncate(native_handle(), native_length) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        // LCOV_EXCL_STOP
        return castle::status::ok;
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

    /** @brief Synchronizes file data and metadata using `fsync()`. */
    castle::status sync(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        // LCOV_EXCL_START
        if (!is_open())
        {
            if (system_error != nullptr)
            {
                *system_error = 0;
            }
            return castle::status::not_configured;
        }
#if CASTLE_USING_POSIX_APIS
        if (::fsync(native_handle()) != 0)
        {
            if (system_error != nullptr)
            {
                *system_error = errno;
            }
            return castle::status::system_call_error;
        }
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        // LCOV_EXCL_STOP
        return castle::status::ok;
#else
        if (system_error != nullptr)
        {
            *system_error = 0;
        }
        return castle::status::not_configured;
#endif
    }

private:
    fd::basic_fd descriptor_;
    file_config config_;
};

using linux_file = basic_file;

} // namespace file
} // namespace io
} // namespace castle

#endif // CASTLE_EXT_IO_FILE_LINUX_FILE_HPP
