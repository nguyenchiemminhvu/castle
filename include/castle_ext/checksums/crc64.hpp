// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc64.hpp
 * @brief Streaming CRC-64/ECMA-182 checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_CRC64_HPP
#define CASTLE_EXT_CHECKSUMS_CRC64_HPP

#include "castle_ext/checksums/crc_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief CRC-64/ECMA-182 checksum.
 *
 * Parameters: polynomial 0x42F0E1EBA9EA3693, initial value 0, no reflection,
 * and final XOR value 0.
 */
using crc64 = detail::basic_crc<uint64_t,
                                static_cast<uint64_t>(0x42F0E1EBA9EA3693ULL),
                                static_cast<uint64_t>(0x0000000000000000ULL),
                                static_cast<uint64_t>(0x0000000000000000ULL),
                                false,
                                false>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_CRC64_HPP
