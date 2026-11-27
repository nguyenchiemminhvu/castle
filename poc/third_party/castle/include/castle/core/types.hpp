// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Fundamental Castle-wide size and difference aliases.
// Include this header when generic code needs ABI-stable size and offset types
// without depending on STL container typedefs. The aliases forward directly to
// the target's fundamental C types, so there is no runtime cost, allocation,
// or initialization behavior.

#ifndef CASTLE_CORE_TYPES_HPP
#define CASTLE_CORE_TYPES_HPP

#include "castle/core/compiler.hpp"

#include <stddef.h>
#include <stdint.h>

namespace castle
{

/// @brief Unsigned size and index type used throughout Castle.
using size_type = size_t;
/// @brief Signed difference type used for distances and offsets.
using difference_type = ptrdiff_t;

} // namespace castle

#endif // CASTLE_CORE_TYPES_HPP
