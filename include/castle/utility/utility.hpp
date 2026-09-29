// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file utility.hpp
 * @brief Umbrella include for Castle's utility-layer building blocks.
 *
 * Use this header when a translation unit needs the full utility surface in one
 * include rather than pulling individual headers. It introduces no new runtime
 * logic of its own; it simply re-exports the utility components listed below.
 *
 * @note This header declares no additional public functions or types beyond the
 * headers it includes.
 */
#ifndef CASTLE_UTILITY_UTILITY_HPP
#define CASTLE_UTILITY_UTILITY_HPP

#include "castle/utility/macros.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/swap.hpp"
#include "castle/utility/bytes.hpp"
#include "castle/utility/bit_cast.hpp"
#include "castle/utility/tuple.hpp"
#include "castle/utility/pair.hpp"
#include "castle/utility/variant.hpp"
#include "castle/utility/optional.hpp"
#include "castle/utility/bitset.hpp"
#include "castle/utility/hash.hpp"

#endif // CASTLE_UTILITY_UTILITY_HPP
