// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fletcher32.hpp
 * @brief Streaming Fletcher-32 checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_FLETCHER32_HPP
#define CASTLE_EXT_CHECKSUMS_FLETCHER32_HPP

#include "castle_ext/checksums/fletcher_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief Fletcher-32 checksum with modulo-65535 accumulators over 16-bit blocks.
 *
 * Blocks are read little-endian by default; a trailing odd byte is zero-padded.
 *
 * @tparam LittleEndian False to read 16-bit blocks big-endian.
 */
template <bool LittleEndian = true>
using basic_fletcher32 = detail::basic_fletcher<uint32_t, LittleEndian>;

using fletcher32 = basic_fletcher32<>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_FLETCHER32_HPP
