// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fletcher16.hpp
 * @brief Streaming Fletcher-16 checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_FLETCHER16_HPP
#define CASTLE_EXT_CHECKSUMS_FLETCHER16_HPP

#include "castle_ext/checksums/fletcher_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief Fletcher-16 checksum with modulo-255 accumulators over 8-bit blocks.
 *
 * The object stores only the two accumulators and can be updated with chunks
 * of a message.
 */
using fletcher16 = detail::basic_fletcher<uint16_t>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_FLETCHER16_HPP
