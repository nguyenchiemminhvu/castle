// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc24q.hpp
 * @brief Streaming CRC-24Q checksum (RTCM 3.x, SBAS L1/L5, Qualcomm).
 */
#ifndef CASTLE_EXT_CHECKSUMS_CRC24Q_HPP
#define CASTLE_EXT_CHECKSUMS_CRC24Q_HPP

#include "castle_ext/checksums/crc_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief CRC-24Q checksum.
 *
 * Parameters: width 24, polynomial 0x864CFB, initial value 0, no reflection,
 * and final XOR value 0. The result occupies the low 24 bits of a `uint32_t`.
 * This is the frame check sequence of RTCM 3.x messages.
 */
using crc24q = detail::basic_crc<uint32_t,
                                 static_cast<uint32_t>(0x864CFBU),
                                 static_cast<uint32_t>(0x000000U),
                                 static_cast<uint32_t>(0x000000U),
                                 false,
                                 false,
                                 24U>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_CRC24Q_HPP
