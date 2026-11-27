// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit.hpp
 * @brief Umbrella header that includes every Castle bit-manipulation component.
 *
 * Use this header when a translation unit needs several Castle bit utilities
 * and a single include is more convenient than selecting individual headers.
 *
 * Key constraints:
 * - This header adds no new runtime behavior; it only aggregates other headers.
 * - The included components remain header-only, deterministic, and allocation free.
 * - No exceptions, RTTI, virtual dispatch, or STL facilities are required.
 */
#ifndef CASTLE_BIT_BIT_HPP
#define CASTLE_BIT_BIT_HPP

#include "castle/bit/bit_core.hpp"
#include "castle/bit/bit_count.hpp"
#include "castle/bit/bit_mask.hpp"
#include "castle/bit/bit_math.hpp"
#include "castle/bit/bit_reverse.hpp"
#include "castle/bit/bit_rotate.hpp"
#include "castle/bit/bit_utils.hpp"
#include "castle/bit/flags.hpp"

#endif
