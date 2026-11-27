// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cache.hpp
 * @brief Umbrella include for Castle cache headers.
 *
 * Include this header when a translation unit needs several cache policies and a single include
 * is more convenient than spelling each one. The header introduces no storage or runtime behavior
 * of its own; it exposes `basic_cache` and the FIFO, LIFO, LRU, MRU, and LFU policies.
 *
 * @code
 * #include "castle/cache/cache.hpp"
 *
 * castle::cache::lru<uint16_t, int32_t, 8U> recent;
 * castle::cache::lfu<uint16_t, int32_t, 8U> popular;
 * @endcode
 */
#ifndef CASTLE_CACHE_CACHE_HPP
#define CASTLE_CACHE_CACHE_HPP

#include "castle/cache/basic_cache.hpp"
#include "castle/cache/fifo.hpp"
#include "castle/cache/lifo.hpp"
#include "castle/cache/lru.hpp"
#include "castle/cache/mru.hpp"
#include "castle/cache/lfu.hpp"

#endif // CASTLE_CACHE_CACHE_HPP
