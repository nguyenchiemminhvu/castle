// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file memory.hpp
 * @brief Umbrella include for Castle's low-level memory helpers.
 *
 * Use this header when a translation unit needs several memory primitives at once, such as
 * address acquisition, alignment checks, explicit construction and destruction, or raw
 * storage wrappers. It introduces no allocation policy of its own; every included component
 * remains heap-free and caller-managed.
 *
 * @code
 * #include "castle/memory/memory.hpp"
 * @endcode
 */
#ifndef CASTLE_MEMORY_MEMORY_HPP
#define CASTLE_MEMORY_MEMORY_HPP

#include "castle/memory/addressof.hpp"
#include "castle/memory/alignment.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/lifetime.hpp"
#include "castle/memory/new.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/soo_buffer.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/memory/storage.hpp"

#endif // CASTLE_MEMORY_MEMORY_HPP
