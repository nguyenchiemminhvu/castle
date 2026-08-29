#pragma once

#include "castle/types/traits.h"

#include <cstdint>
#include <climits>

using namespace castle::types;

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// byte_swap — reverse the byte order of an integral value.
// Essential for converting between little-endian and big-endian
// representations in embedded communication protocols.
// ──────────────────────────────────────────────────────────────

constexpr std::uint8_t byte_swap(std::uint8_t v) noexcept
{
    return v; // single byte — nothing to swap
}

constexpr std::uint16_t byte_swap(std::uint16_t v) noexcept
{
    return static_cast<std::uint16_t>(
        ((v & 0x00FFU) << 8) |
        ((v & 0xFF00U) >> 8)
    );
}

constexpr std::uint32_t byte_swap(std::uint32_t v) noexcept
{
    v = ((v & 0x00FF00FFU) << 8)  | ((v & 0xFF00FF00U) >> 8);
    v = ((v & 0x0000FFFFU) << 16) | ((v & 0xFFFF0000U) >> 16);
    return v;
}

constexpr std::uint64_t byte_swap(std::uint64_t v) noexcept
{
    v = ((v & 0x00FF00FF00FF00FFULL) << 8)  | ((v & 0xFF00FF00FF00FF00ULL) >> 8);
    v = ((v & 0x0000FFFF0000FFFFULL) << 16) | ((v & 0xFFFF0000FFFF0000ULL) >> 16);
    v = ((v & 0x00000000FFFFFFFFULL) << 32) | ((v & 0xFFFFFFFF00000000ULL) >> 32);
    return v;
}

// Generic dispatcher for signed / other integral types.
template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
byte_swap(T value) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    return static_cast<T>(byte_swap(static_cast<UnsignedT>(value)));
}

} // namespace bit
} // namespace castle
