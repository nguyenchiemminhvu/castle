// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fd.hpp
 * @brief Convenience include for CASTLE's POSIX file-descriptor helpers and poll event manager.
 *
 * Include this header when an application needs both descriptor control helpers
 * and the fixed-capacity `poll()` event dispatcher. All public APIs are exposed
 * in the `castle::io::fd` namespace.
 *
 * @code
 * #include "castle_ext/io/fd/fd.hpp"
 *
 * castle::io::fd::fd_event_manager manager;
 * @endcode
 */
#ifndef CASTLE_EXT_IO_FD_FD_HPP
#define CASTLE_EXT_IO_FD_FD_HPP

#include "castle_ext/io/fd/control.hpp"
#include "castle_ext/io/fd/descriptor.hpp"
#include "castle_ext/io/fd/poll.hpp"

#endif // CASTLE_EXT_IO_FD_FD_HPP
