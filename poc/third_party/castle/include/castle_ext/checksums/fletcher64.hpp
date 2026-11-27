// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fletcher64.hpp
 * @brief Streaming Fletcher-64 checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_FLETCHER64_HPP
#define CASTLE_EXT_CHECKSUMS_FLETCHER64_HPP

#include "castle_ext/checksums/fletcher_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief Fletcher-64 checksum with modulo-(2^32 - 1) accumulators over 32-bit blocks.
 *
 * Blocks are read little-endian by default; a trailing partial block is zero-padded.
 *
 * @tparam LittleEndian False to read 32-bit blocks big-endian.
 */
template <bool LittleEndian = true>
using basic_fletcher64 = detail::basic_fletcher<uint64_t, LittleEndian>;

using fletcher64 = basic_fletcher64<>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_FLETCHER64_HPP
