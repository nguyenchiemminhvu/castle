// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc8.hpp
 * @brief Streaming CRC-8/SMBUS (CRC-8) checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_CRC8_HPP
#define CASTLE_EXT_CHECKSUMS_CRC8_HPP

#include "castle_ext/checksums/crc_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief CRC-8/SMBUS checksum with polynomial 0x07.
 *
 * The object stores only the current CRC state and can be updated with chunks
 * of a message. No lookup table or dynamic memory is used.
 */
using crc8 = detail::basic_crc<uint8_t,
                               static_cast<uint8_t>(0x07U),
                               static_cast<uint8_t>(0x00U),
                               static_cast<uint8_t>(0x00U),
                               false,
                               false>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_CRC8_HPP
