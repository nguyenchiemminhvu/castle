// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bytes.hpp
 * @brief Endian-explicit byte loads/stores, rotations, and secure wiping.
 *
 * Load and store helpers are alignment-independent and byte-order explicit;
 * the caller must guarantee the referenced buffer holds enough bytes. The
 * helpers use fixed-width integer types and do not allocate or throw.
 */
#ifndef CASTLE_UTILITY_BYTES_HPP
#define CASTLE_UTILITY_BYTES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{

/** @brief Reads an 8-bit value; `p` must reference at least 1 byte. */
CASTLE_CONSTEXPR uint8_t read_le8(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return p[0];
}

/** @brief Reads an 8-bit value; `p` must reference at least 1 byte. */
CASTLE_CONSTEXPR uint8_t read_be8(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return p[0];
}

/** @brief Reads a signed 8-bit value; `p` must reference at least 1 byte. */
CASTLE_CONSTEXPR int8_t read_le8s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int8_t>(read_le8(p));
}

/** @brief Reads a signed 8-bit value; `p` must reference at least 1 byte. */
CASTLE_CONSTEXPR int8_t read_be8s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int8_t>(read_be8(p));
}

/** @brief Reads a little-endian 16-bit value; `p` must reference at least 2 bytes. */
CASTLE_CONSTEXPR uint16_t read_le16(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<uint16_t>(static_cast<uint16_t>(p[0]) |
                                 (static_cast<uint16_t>(p[1]) << 8U));
}

/** @brief Reads a big-endian 16-bit value; `p` must reference at least 2 bytes. */
CASTLE_CONSTEXPR uint16_t read_be16(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8U) |
                                 static_cast<uint16_t>(p[1]));
}

/** @brief Reads a signed little-endian 16-bit value; `p` must reference at least 2 bytes. */
CASTLE_CONSTEXPR int16_t read_le16s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int16_t>(read_le16(p));
}

/** @brief Reads a signed big-endian 16-bit value; `p` must reference at least 2 bytes. */
CASTLE_CONSTEXPR int16_t read_be16s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int16_t>(read_be16(p));
}

/** @brief Reads a little-endian 32-bit value; `p` must reference at least 4 bytes. */
CASTLE_CONSTEXPR uint32_t read_le32(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8U) |
           (static_cast<uint32_t>(p[2]) << 16U) |
           (static_cast<uint32_t>(p[3]) << 24U);
}

/** @brief Reads a big-endian 32-bit value; `p` must reference at least 4 bytes. */
CASTLE_CONSTEXPR uint32_t read_be32(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return (static_cast<uint32_t>(p[0]) << 24U) |
           (static_cast<uint32_t>(p[1]) << 16U) |
           (static_cast<uint32_t>(p[2]) << 8U) |
           static_cast<uint32_t>(p[3]);
}

/** @brief Reads a signed little-endian 32-bit value; `p` must reference at least 4 bytes. */
CASTLE_CONSTEXPR int32_t read_le32s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int32_t>(read_le32(p));
}

/** @brief Reads a signed big-endian 32-bit value; `p` must reference at least 4 bytes. */
CASTLE_CONSTEXPR int32_t read_be32s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int32_t>(read_be32(p));
}

/** @brief Reads a little-endian 64-bit value; `p` must reference at least 8 bytes. */
CASTLE_CONSTEXPR uint64_t read_le64(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<uint64_t>(p[0]) |
           (static_cast<uint64_t>(p[1]) << 8U) |
           (static_cast<uint64_t>(p[2]) << 16U) |
           (static_cast<uint64_t>(p[3]) << 24U) |
           (static_cast<uint64_t>(p[4]) << 32U) |
           (static_cast<uint64_t>(p[5]) << 40U) |
           (static_cast<uint64_t>(p[6]) << 48U) |
           (static_cast<uint64_t>(p[7]) << 56U);
}

/** @brief Reads a big-endian 64-bit value; `p` must reference at least 8 bytes. */
CASTLE_CONSTEXPR uint64_t read_be64(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return (static_cast<uint64_t>(p[0]) << 56U) |
           (static_cast<uint64_t>(p[1]) << 48U) |
           (static_cast<uint64_t>(p[2]) << 40U) |
           (static_cast<uint64_t>(p[3]) << 32U) |
           (static_cast<uint64_t>(p[4]) << 24U) |
           (static_cast<uint64_t>(p[5]) << 16U) |
           (static_cast<uint64_t>(p[6]) << 8U) |
           static_cast<uint64_t>(p[7]);
}

/** @brief Reads a signed little-endian 64-bit value; `p` must reference at least 8 bytes. */
CASTLE_CONSTEXPR int64_t read_le64s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int64_t>(read_le64(p));
}

/** @brief Reads a signed big-endian 64-bit value; `p` must reference at least 8 bytes. */
CASTLE_CONSTEXPR int64_t read_be64s(CASTLE_CONST uint8_t* p) CASTLE_NOEXCEPT
{
    return static_cast<int64_t>(read_be64(p));
}

/** @brief Writes an 8-bit value; `p` must reference at least 1 byte. */
inline void write_le8(uint8_t* p, uint8_t value) CASTLE_NOEXCEPT
{
    p[0] = value;
}

/** @brief Writes an 8-bit value; `p` must reference at least 1 byte. */
inline void write_be8(uint8_t* p, uint8_t value) CASTLE_NOEXCEPT
{
    p[0] = value;
}

/** @brief Writes a 16-bit value little-endian; `p` must reference at least 2 bytes. */
inline void write_le16(uint8_t* p, uint16_t value) CASTLE_NOEXCEPT
{
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8U);
}

/** @brief Writes a 16-bit value big-endian; `p` must reference at least 2 bytes. */
inline void write_be16(uint8_t* p, uint16_t value) CASTLE_NOEXCEPT
{
    p[0] = static_cast<uint8_t>(value >> 8U);
    p[1] = static_cast<uint8_t>(value);
}

/** @brief Writes a 32-bit value little-endian; `p` must reference at least 4 bytes. */
inline void write_le32(uint8_t* p, uint32_t value) CASTLE_NOEXCEPT
{
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8U);
    p[2] = static_cast<uint8_t>(value >> 16U);
    p[3] = static_cast<uint8_t>(value >> 24U);
}

/** @brief Writes a 32-bit value big-endian; `p` must reference at least 4 bytes. */
inline void write_be32(uint8_t* p, uint32_t value) CASTLE_NOEXCEPT
{
    p[0] = static_cast<uint8_t>(value >> 24U);
    p[1] = static_cast<uint8_t>(value >> 16U);
    p[2] = static_cast<uint8_t>(value >> 8U);
    p[3] = static_cast<uint8_t>(value);
}

/** @brief Writes a 64-bit value little-endian; `p` must reference at least 8 bytes. */
inline void write_le64(uint8_t* p, uint64_t value) CASTLE_NOEXCEPT
{
    p[0] = static_cast<uint8_t>(value);
    p[1] = static_cast<uint8_t>(value >> 8U);
    p[2] = static_cast<uint8_t>(value >> 16U);
    p[3] = static_cast<uint8_t>(value >> 24U);
    p[4] = static_cast<uint8_t>(value >> 32U);
    p[5] = static_cast<uint8_t>(value >> 40U);
    p[6] = static_cast<uint8_t>(value >> 48U);
    p[7] = static_cast<uint8_t>(value >> 56U);
}

/** @brief Writes a 64-bit value big-endian; `p` must reference at least 8 bytes. */
inline void write_be64(uint8_t* p, uint64_t value) CASTLE_NOEXCEPT
{
    p[0] = static_cast<uint8_t>(value >> 56U);
    p[1] = static_cast<uint8_t>(value >> 48U);
    p[2] = static_cast<uint8_t>(value >> 40U);
    p[3] = static_cast<uint8_t>(value >> 32U);
    p[4] = static_cast<uint8_t>(value >> 24U);
    p[5] = static_cast<uint8_t>(value >> 16U);
    p[6] = static_cast<uint8_t>(value >> 8U);
    p[7] = static_cast<uint8_t>(value);
}

/** @brief Writes a signed 8-bit value in little-endian order. */
inline void write_le8s(uint8_t* p, int8_t value) CASTLE_NOEXCEPT
{
    write_le8(p, static_cast<uint8_t>(value));
}

/** @brief Writes a signed 8-bit value in big-endian order. */
inline void write_be8s(uint8_t* p, int8_t value) CASTLE_NOEXCEPT
{
    write_be8(p, static_cast<uint8_t>(value));
}

/** @brief Writes a signed 16-bit value in little-endian order. */
inline void write_le16s(uint8_t* p, int16_t value) CASTLE_NOEXCEPT
{
    write_le16(p, static_cast<uint16_t>(value));
}

/** @brief Writes a signed 16-bit value in big-endian order. */
inline void write_be16s(uint8_t* p, int16_t value) CASTLE_NOEXCEPT
{
    write_be16(p, static_cast<uint16_t>(value));
}

/** @brief Writes a signed 32-bit value in little-endian order. */
inline void write_le32s(uint8_t* p, int32_t value) CASTLE_NOEXCEPT
{
    write_le32(p, static_cast<uint32_t>(value));
}

/** @brief Writes a signed 32-bit value in big-endian order. */
inline void write_be32s(uint8_t* p, int32_t value) CASTLE_NOEXCEPT
{
    write_be32(p, static_cast<uint32_t>(value));
}

/** @brief Writes a signed 64-bit value in little-endian order. */
inline void write_le64s(uint8_t* p, int64_t value) CASTLE_NOEXCEPT
{
    write_le64(p, static_cast<uint64_t>(value));
}

/** @brief Writes a signed 64-bit value in big-endian order. */
inline void write_be64s(uint8_t* p, int64_t value) CASTLE_NOEXCEPT
{
    write_be64(p, static_cast<uint64_t>(value));
}

/** @brief Rotates a 32-bit value left; `count` must be in 1..31. */
CASTLE_CONSTEXPR uint32_t rotl32(uint32_t value, uint32_t count) CASTLE_NOEXCEPT
{
    return static_cast<uint32_t>((value << count) | (value >> (32U - count)));
}

/** @brief Rotates a 32-bit value right; `count` must be in 1..31. */
CASTLE_CONSTEXPR uint32_t rotr32(uint32_t value, uint32_t count) CASTLE_NOEXCEPT
{
    return static_cast<uint32_t>((value >> count) | (value << (32U - count)));
}

/** @brief Rotates a 64-bit value right; `count` must be in 1..63. */
CASTLE_CONSTEXPR uint64_t rotr64(uint64_t value, uint32_t count) CASTLE_NOEXCEPT
{
    return static_cast<uint64_t>((value >> count) | (value << (64U - count)));
}

/**
 * @brief Zeroes a buffer through a volatile pointer so the writes are not optimized away.
 * @param address Buffer to wipe.
 * @param size    Number of bytes.
 */
inline void secure_zero(void* address, size_type size) CASTLE_NOEXCEPT
{
    volatile uint8_t* bytes = static_cast<volatile uint8_t*>(address);
    for (size_type i = 0U; i < size; ++i)
    {
        bytes[i] = 0U;
    }
}

} // namespace castle

#endif // CASTLE_UTILITY_BYTES_HPP