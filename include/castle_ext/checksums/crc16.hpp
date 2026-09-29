// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc16.hpp
 * @brief Streaming CRC-16/CCITT-FALSE checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_CRC16_HPP
#define CASTLE_EXT_CHECKSUMS_CRC16_HPP

#include "castle_ext/checksums/crc_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief CRC-16/CCITT-FALSE checksum.
 *
 * Parameters: polynomial 0x1021, initial value 0xFFFF, no reflection,
 * and final XOR value 0.
 */
using crc16 = detail::basic_crc<uint16_t,
                                static_cast<uint16_t>(0x1021U),
                                static_cast<uint16_t>(0xFFFFU),
                                static_cast<uint16_t>(0x0000U),
                                false,
                                false>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_CRC16_HPP
