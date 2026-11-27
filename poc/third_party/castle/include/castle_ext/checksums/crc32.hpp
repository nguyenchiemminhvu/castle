// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file crc32.hpp
 * @brief Streaming CRC-32/ISO-HDLC checksum.
 */
#ifndef CASTLE_EXT_CHECKSUMS_CRC32_HPP
#define CASTLE_EXT_CHECKSUMS_CRC32_HPP

#include "castle_ext/checksums/crc_common.hpp"

namespace castle
{
namespace checksums
{

/**
 * @brief CRC-32/ISO-HDLC checksum, commonly known as Ethernet/ZIP/PNG CRC-32.
 *
 * Parameters: polynomial 0x04C11DB7, initial value 0xFFFFFFFF, reflected
 * input/output, and final XOR value 0xFFFFFFFF.
 */
using crc32 = detail::basic_crc<uint32_t,
                                static_cast<uint32_t>(0x04C11DB7UL),
                                static_cast<uint32_t>(0xFFFFFFFFUL),
                                static_cast<uint32_t>(0xFFFFFFFFUL),
                                true,
                                true>;

} // namespace checksums
} // namespace castle

#endif // CASTLE_EXT_CHECKSUMS_CRC32_HPP
