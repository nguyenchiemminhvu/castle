#ifndef CASTLE_UTILITY_SAFE_CAST_H
#define CASTLE_UTILITY_SAFE_CAST_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/type_ranges.h"
#include "castle/math/abs.h"

namespace castle
{

class safe_cast
{
public:
    // bool conversions
    static CASTLE_INLINE int8_t bool_to_int8(bool value) CASTLE_NOEXCEPT { return value ? static_cast<int8_t>(1) : static_cast<int8_t>(0); }
    static CASTLE_INLINE int16_t bool_to_int16(bool value) CASTLE_NOEXCEPT { return value ? static_cast<int16_t>(1) : static_cast<int16_t>(0); }
    static CASTLE_INLINE int32_t bool_to_int32(bool value) CASTLE_NOEXCEPT { return value ? static_cast<int32_t>(1) : static_cast<int32_t>(0); }
    static CASTLE_INLINE int64_t bool_to_int64(bool value) CASTLE_NOEXCEPT { return value ? static_cast<int64_t>(1) : static_cast<int64_t>(0); }
    static CASTLE_INLINE uint8_t bool_to_uint8(bool value) CASTLE_NOEXCEPT { return value ? static_cast<uint8_t>(1U) : static_cast<uint8_t>(0U); }
    static CASTLE_INLINE uint16_t bool_to_uint16(bool value) CASTLE_NOEXCEPT { return value ? static_cast<uint16_t>(1U) : static_cast<uint16_t>(0U); }
    static CASTLE_INLINE uint32_t bool_to_uint32(bool value) CASTLE_NOEXCEPT { return value ? static_cast<uint32_t>(1U) : static_cast<uint32_t>(0U); }
    static CASTLE_INLINE uint64_t bool_to_uint64(bool value) CASTLE_NOEXCEPT { return value ? static_cast<uint64_t>(1U) : static_cast<uint64_t>(0U); }
    static CASTLE_INLINE float bool_to_float(bool value) CASTLE_NOEXCEPT { return value ? 1.0f : 0.0f; }
    static CASTLE_INLINE double bool_to_double(bool value) CASTLE_NOEXCEPT { return value ? 1.0 : 0.0; }
    // int8_t conversions
    static CASTLE_INLINE bool int8_to_bool(int8_t value) CASTLE_NOEXCEPT { return value != 0; }
    static CASTLE_INLINE int16_t int8_to_int16(int8_t value) CASTLE_NOEXCEPT { return static_cast<int16_t>(value); }
    static CASTLE_INLINE int32_t int8_to_int32(int8_t value) CASTLE_NOEXCEPT { return static_cast<int32_t>(value); }
    static CASTLE_INLINE int64_t int8_to_int64(int8_t value) CASTLE_NOEXCEPT { return static_cast<int64_t>(value); }
    static CASTLE_INLINE uint8_t int8_to_uint8(int8_t value) CASTLE_NOEXCEPT { return (value < 0) ? 0U : static_cast<uint8_t>(value); }
    static CASTLE_INLINE uint16_t int8_to_uint16(int8_t value) CASTLE_NOEXCEPT { return (value < 0) ? static_cast<uint16_t>(0U) : static_cast<uint16_t>(value); }
    static CASTLE_INLINE uint32_t int8_to_uint32(int8_t value) CASTLE_NOEXCEPT { return (value < 0) ? 0U : static_cast<uint32_t>(value); }
    static CASTLE_INLINE uint64_t int8_to_uint64(int8_t value) CASTLE_NOEXCEPT { return (value < 0) ? static_cast<uint64_t>(0U) : static_cast<uint64_t>(value); }
    static CASTLE_INLINE float int8_to_float(int8_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double int8_to_double(int8_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // int16_t conversions
    static CASTLE_INLINE bool int16_to_bool(int16_t value) CASTLE_NOEXCEPT { return value != 0; }
    static CASTLE_INLINE int8_t int16_to_int8(int16_t value) CASTLE_NOEXCEPT
    {
        if (value < static_cast<int16_t>(castle::numeric_limits<int8_t>::min())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::min()); }
        if (value > static_cast<int16_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int32_t int16_to_int32(int16_t value) CASTLE_NOEXCEPT { return static_cast<int32_t>(value); }
    static CASTLE_INLINE int64_t int16_to_int64(int16_t value) CASTLE_NOEXCEPT { return static_cast<int64_t>(value); }
    static CASTLE_INLINE uint8_t int16_to_uint8(int16_t value) CASTLE_NOEXCEPT
    {
        if (value < 0) { return 0U; }
        if (value > static_cast<int16_t>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t int16_to_uint16(int16_t value) CASTLE_NOEXCEPT { return (value < 0) ? static_cast<uint16_t>(0U) : static_cast<uint16_t>(value); }
    static CASTLE_INLINE uint32_t int16_to_uint32(int16_t value) CASTLE_NOEXCEPT { return (value < 0) ? 0U : static_cast<uint32_t>(value); }
    static CASTLE_INLINE uint64_t int16_to_uint64(int16_t value) CASTLE_NOEXCEPT { return (value < 0) ? static_cast<uint64_t>(0U) : static_cast<uint64_t>(value); }
    static CASTLE_INLINE float int16_to_float(int16_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double int16_to_double(int16_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }
    // int32_t conversions
    static CASTLE_INLINE bool int32_to_bool(int32_t value) CASTLE_NOEXCEPT { return value != 0; }
    static CASTLE_INLINE int8_t int32_to_int8(int32_t value) CASTLE_NOEXCEPT
    {
        if (value < static_cast<int32_t>(castle::numeric_limits<int8_t>::min())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::min()); }
        if (value > static_cast<int32_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t int32_to_int16(int32_t value) CASTLE_NOEXCEPT
    {
        if (value < static_cast<int32_t>(castle::numeric_limits<int16_t>::min())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::min()); }
        if (value > static_cast<int32_t>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int64_t int32_to_int64(int32_t value) CASTLE_NOEXCEPT { return static_cast<int64_t>(value); }
    static CASTLE_INLINE uint8_t int32_to_uint8(int32_t value) CASTLE_NOEXCEPT
    {
        if (value < 0) { return 0U; }
        if (value > static_cast<int32_t>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t int32_to_uint16(int32_t value) CASTLE_NOEXCEPT
    {
        if (value < 0) { return 0U; }
        if (value > static_cast<int32_t>(castle::numeric_limits<uint16_t>::max())) { return static_cast<uint16_t>(castle::numeric_limits<uint16_t>::max()); }
        return static_cast<uint16_t>(value);
    }
    static CASTLE_INLINE uint32_t int32_to_uint32(int32_t value) CASTLE_NOEXCEPT { return (value < 0) ? 0U : static_cast<uint32_t>(value); }
    static CASTLE_INLINE uint64_t int32_to_uint64(int32_t value) CASTLE_NOEXCEPT { return (value < 0) ? static_cast<uint64_t>(0U) : static_cast<uint64_t>(value); }
    static CASTLE_INLINE float int32_to_float(int32_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double int32_to_double(int32_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // int64_t conversions
    static CASTLE_INLINE bool int64_to_bool(int64_t value) CASTLE_NOEXCEPT { return value != 0; }
    static CASTLE_INLINE int8_t int64_to_int8(int64_t value) CASTLE_NOEXCEPT
    {
        if (value < static_cast<int64_t>(castle::numeric_limits<int8_t>::min())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::min()); }
        if (value > static_cast<int64_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t int64_to_int16(int64_t value) CASTLE_NOEXCEPT
    {
        if (value < static_cast<int64_t>(castle::numeric_limits<int16_t>::min())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::min()); }
        if (value > static_cast<int64_t>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int32_t int64_to_int32(int64_t value) CASTLE_NOEXCEPT
    {
        if (value < static_cast<int64_t>(castle::numeric_limits<int32_t>::min())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::min()); }
        if (value > static_cast<int64_t>(castle::numeric_limits<int32_t>::max())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::max()); }
        return static_cast<int32_t>(value);
    }
    static CASTLE_INLINE uint8_t int64_to_uint8(int64_t value) CASTLE_NOEXCEPT
    {
        if (value < 0) { return 0U; }
        if (value > static_cast<int64_t>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t int64_to_uint16(int64_t value) CASTLE_NOEXCEPT
    {
        if (value < 0) { return 0U; }
        if (value > static_cast<int64_t>(castle::numeric_limits<uint16_t>::max())) { return static_cast<uint16_t>(castle::numeric_limits<uint16_t>::max()); }
        return static_cast<uint16_t>(value);
    }
    static CASTLE_INLINE uint32_t int64_to_uint32(int64_t value) CASTLE_NOEXCEPT
    {
        if (value < 0) { return 0U; }
        if (value > static_cast<int64_t>(castle::numeric_limits<uint32_t>::max())) { return static_cast<uint32_t>(castle::numeric_limits<uint32_t>::max()); }
        return static_cast<uint32_t>(value);
    }
    static CASTLE_INLINE uint64_t int64_to_uint64(int64_t value) CASTLE_NOEXCEPT { return (value < 0) ? static_cast<uint64_t>(0U) : static_cast<uint64_t>(value); }
    static CASTLE_INLINE float int64_to_float(int64_t value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) < static_cast<double>(castle::numeric_limits<float>::lowest())) { return castle::numeric_limits<float>::lowest(); }
        if (static_cast<double>(value) > static_cast<double>(castle::numeric_limits<float>::max())) { return castle::numeric_limits<float>::max(); }
        return static_cast<float>(value);
    }
    static CASTLE_INLINE double int64_to_double(int64_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // uint8_t conversions
    static CASTLE_INLINE bool uint8_to_bool(uint8_t value) CASTLE_NOEXCEPT { return value != 0U; }
    static CASTLE_INLINE int8_t uint8_to_int8(uint8_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint8_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t uint8_to_int16(uint8_t value) CASTLE_NOEXCEPT { return static_cast<int16_t>(value); }
    static CASTLE_INLINE int32_t uint8_to_int32(uint8_t value) CASTLE_NOEXCEPT { return static_cast<int32_t>(value); }
    static CASTLE_INLINE int64_t uint8_to_int64(uint8_t value) CASTLE_NOEXCEPT { return static_cast<int64_t>(value); }
    static CASTLE_INLINE uint16_t uint8_to_uint16(uint8_t value) CASTLE_NOEXCEPT { return static_cast<uint16_t>(value); }
    static CASTLE_INLINE uint32_t uint8_to_uint32(uint8_t value) CASTLE_NOEXCEPT { return static_cast<uint32_t>(value); }
    static CASTLE_INLINE uint64_t uint8_to_uint64(uint8_t value) CASTLE_NOEXCEPT { return static_cast<uint64_t>(value); }
    static CASTLE_INLINE float uint8_to_float(uint8_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double uint8_to_double(uint8_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // uint16_t conversions
    static CASTLE_INLINE bool uint16_to_bool(uint16_t value) CASTLE_NOEXCEPT { return value != 0U; }
    static CASTLE_INLINE int8_t uint16_to_int8(uint16_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint16_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t uint16_to_int16(uint16_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint16_t>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int32_t uint16_to_int32(uint16_t value) CASTLE_NOEXCEPT { return static_cast<int32_t>(value); }
    static CASTLE_INLINE int64_t uint16_to_int64(uint16_t value) CASTLE_NOEXCEPT { return static_cast<int64_t>(value); }
    static CASTLE_INLINE uint8_t uint16_to_uint8(uint16_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint16_t>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint32_t uint16_to_uint32(uint16_t value) CASTLE_NOEXCEPT { return static_cast<uint32_t>(value); }
    static CASTLE_INLINE uint64_t uint16_to_uint64(uint16_t value) CASTLE_NOEXCEPT { return static_cast<uint64_t>(value); }
    static CASTLE_INLINE float uint16_to_float(uint16_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double uint16_to_double(uint16_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // uint32_t conversions
    static CASTLE_INLINE bool uint32_to_bool(uint32_t value) CASTLE_NOEXCEPT { return value != 0U; }
    static CASTLE_INLINE int8_t uint32_to_int8(uint32_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint32_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t uint32_to_int16(uint32_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint32_t>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int32_t uint32_to_int32(uint32_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint32_t>(castle::numeric_limits<int32_t>::max())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::max()); }
        return static_cast<int32_t>(value);
    }
    static CASTLE_INLINE int64_t uint32_to_int64(uint32_t value) CASTLE_NOEXCEPT { return static_cast<int64_t>(value); }
    static CASTLE_INLINE uint8_t uint32_to_uint8(uint32_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint32_t>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t uint32_to_uint16(uint32_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint32_t>(castle::numeric_limits<uint16_t>::max())) { return static_cast<uint16_t>(castle::numeric_limits<uint16_t>::max()); }
        return static_cast<uint16_t>(value);
    }
    static CASTLE_INLINE uint64_t uint32_to_uint64(uint32_t value) CASTLE_NOEXCEPT { return static_cast<uint64_t>(value); }
    static CASTLE_INLINE float uint32_to_float(uint32_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double uint32_to_double(uint32_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // uint64_t conversions
    static CASTLE_INLINE bool uint64_to_bool(uint64_t value) CASTLE_NOEXCEPT { return value != 0U; }
    static CASTLE_INLINE int8_t uint64_to_int8(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t uint64_to_int16(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int32_t uint64_to_int32(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<int32_t>::max())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::max()); }
        return static_cast<int32_t>(value);
    }
    static CASTLE_INLINE int64_t uint64_to_int64(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<int64_t>::max())) { return static_cast<int64_t>(castle::numeric_limits<int64_t>::max()); }
        return static_cast<int64_t>(value);
    }
    static CASTLE_INLINE uint8_t uint64_to_uint8(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t uint64_to_uint16(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<uint16_t>::max())) { return static_cast<uint16_t>(castle::numeric_limits<uint16_t>::max()); }
        return static_cast<uint16_t>(value);
    }
    static CASTLE_INLINE uint32_t uint64_to_uint32(uint64_t value) CASTLE_NOEXCEPT
    {
        if (value > static_cast<uint64_t>(castle::numeric_limits<uint32_t>::max())) { return static_cast<uint32_t>(castle::numeric_limits<uint32_t>::max()); }
        return static_cast<uint32_t>(value);
    }
    static CASTLE_INLINE float uint64_to_float(uint64_t value) CASTLE_NOEXCEPT { return static_cast<float>(value); }
    static CASTLE_INLINE double uint64_to_double(uint64_t value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // float conversions
    static CASTLE_INLINE bool float_to_bool(float value) CASTLE_NOEXCEPT { return castle::math::abs(static_cast<double>(value)) >= meta::floating_epsilon<float>::value; }
    static CASTLE_INLINE int8_t float_to_int8(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= static_cast<double>(castle::numeric_limits<int8_t>::min())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::min()); }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t float_to_int16(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= static_cast<double>(castle::numeric_limits<int16_t>::min())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::min()); }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int32_t float_to_int32(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= static_cast<double>(castle::numeric_limits<int32_t>::min())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::min()); }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<int32_t>::max())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::max()); }
        return static_cast<int32_t>(value);
    }
    static CASTLE_INLINE int64_t float_to_int64(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= static_cast<double>(castle::numeric_limits<int64_t>::min())) { return static_cast<int64_t>(castle::numeric_limits<int64_t>::min()); }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<int64_t>::max())) { return static_cast<int64_t>(castle::numeric_limits<int64_t>::max()); }
        return static_cast<int64_t>(value);
    }
    static CASTLE_INLINE uint8_t float_to_uint8(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= 0.0) { return 0U; }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t float_to_uint16(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= 0.0) { return 0U; }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<uint16_t>::max())) { return static_cast<uint16_t>(castle::numeric_limits<uint16_t>::max()); }
        return static_cast<uint16_t>(value);
    }
    static CASTLE_INLINE uint32_t float_to_uint32(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= 0.0) { return 0U; }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<uint32_t>::max())) { return static_cast<uint32_t>(castle::numeric_limits<uint32_t>::max()); }
        return static_cast<uint32_t>(value);
    }
    static CASTLE_INLINE uint64_t float_to_uint64(float value) CASTLE_NOEXCEPT
    {
        if (static_cast<double>(value) <= 0.0) { return 0U; }
        if (static_cast<double>(value) >= static_cast<double>(castle::numeric_limits<uint64_t>::max())) { return static_cast<uint64_t>(castle::numeric_limits<uint64_t>::max()); }
        return static_cast<uint64_t>(value);
    }
    static CASTLE_INLINE double float_to_double(float value) CASTLE_NOEXCEPT { return static_cast<double>(value); }

    // double conversions
    static CASTLE_INLINE bool double_to_bool(double value) CASTLE_NOEXCEPT { return castle::math::abs(value) >= meta::floating_epsilon<double>::value; }
    static CASTLE_INLINE int8_t double_to_int8(double value) CASTLE_NOEXCEPT
    {
        if (value <= static_cast<double>(castle::numeric_limits<int8_t>::min())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::min()); }
        if (value >= static_cast<double>(castle::numeric_limits<int8_t>::max())) { return static_cast<int8_t>(castle::numeric_limits<int8_t>::max()); }
        return static_cast<int8_t>(value);
    }
    static CASTLE_INLINE int16_t double_to_int16(double value) CASTLE_NOEXCEPT
    {
        if (value <= static_cast<double>(castle::numeric_limits<int16_t>::min())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::min()); }
        if (value >= static_cast<double>(castle::numeric_limits<int16_t>::max())) { return static_cast<int16_t>(castle::numeric_limits<int16_t>::max()); }
        return static_cast<int16_t>(value);
    }
    static CASTLE_INLINE int32_t double_to_int32(double value) CASTLE_NOEXCEPT
    {
        if (value <= static_cast<double>(castle::numeric_limits<int32_t>::min())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::min()); }
        if (value >= static_cast<double>(castle::numeric_limits<int32_t>::max())) { return static_cast<int32_t>(castle::numeric_limits<int32_t>::max()); }
        return static_cast<int32_t>(value);
    }
    static CASTLE_INLINE int64_t double_to_int64(double value) CASTLE_NOEXCEPT
    {
        if (value <= static_cast<double>(castle::numeric_limits<int64_t>::min())) { return static_cast<int64_t>(castle::numeric_limits<int64_t>::min()); }
        if (value >= static_cast<double>(castle::numeric_limits<int64_t>::max())) { return static_cast<int64_t>(castle::numeric_limits<int64_t>::max()); }
        return static_cast<int64_t>(value);
    }
    static CASTLE_INLINE uint8_t double_to_uint8(double value) CASTLE_NOEXCEPT
    {
        if (value <= 0.0) { return 0U; }
        if (value >= static_cast<double>(castle::numeric_limits<uint8_t>::max())) { return static_cast<uint8_t>(castle::numeric_limits<uint8_t>::max()); }
        return static_cast<uint8_t>(value);
    }
    static CASTLE_INLINE uint16_t double_to_uint16(double value) CASTLE_NOEXCEPT
    {
        if (value <= 0.0) { return 0U; }
        if (value >= static_cast<double>(castle::numeric_limits<uint16_t>::max())) { return static_cast<uint16_t>(castle::numeric_limits<uint16_t>::max()); }
        return static_cast<uint16_t>(value);
    }
    static CASTLE_INLINE uint32_t double_to_uint32(double value) CASTLE_NOEXCEPT
    {
        if (value <= 0.0) { return 0U; }
        if (value >= static_cast<double>(castle::numeric_limits<uint32_t>::max())) { return static_cast<uint32_t>(castle::numeric_limits<uint32_t>::max()); }
        return static_cast<uint32_t>(value);
    }
    static CASTLE_INLINE uint64_t double_to_uint64(double value) CASTLE_NOEXCEPT
    {
        if (value <= 0.0) { return 0U; }
        if (value >= static_cast<double>(castle::numeric_limits<uint64_t>::max())) { return static_cast<uint64_t>(castle::numeric_limits<uint64_t>::max()); }
        return static_cast<uint64_t>(value);
    }
    static CASTLE_INLINE float double_to_float(double value) CASTLE_NOEXCEPT
    {
        if (value <= static_cast<double>(castle::numeric_limits<float>::lowest())) { return castle::numeric_limits<float>::lowest(); }
        if (value >= static_cast<double>(castle::numeric_limits<float>::max())) { return castle::numeric_limits<float>::max(); }
        return static_cast<float>(value);
    }
};

template <typename From, typename To>
CASTLE_INLINE To SAFE_CAST(From value)
{
    return static_cast<To>(value);
}

// bool specializations
template <> CASTLE_INLINE int8_t SAFE_CAST<bool, int8_t>(bool value) { return safe_cast::bool_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<bool, int16_t>(bool value) { return safe_cast::bool_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<bool, int32_t>(bool value) { return safe_cast::bool_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<bool, int64_t>(bool value) { return safe_cast::bool_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<bool, uint8_t>(bool value) { return safe_cast::bool_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<bool, uint16_t>(bool value) { return safe_cast::bool_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<bool, uint32_t>(bool value) { return safe_cast::bool_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<bool, uint64_t>(bool value) { return safe_cast::bool_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<bool, float>(bool value) { return safe_cast::bool_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<bool, double>(bool value) { return safe_cast::bool_to_double(value); }

// int8_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<int8_t, bool>(int8_t value) { return safe_cast::int8_to_bool(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<int8_t, int16_t>(int8_t value) { return safe_cast::int8_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<int8_t, int32_t>(int8_t value) { return safe_cast::int8_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<int8_t, int64_t>(int8_t value) { return safe_cast::int8_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<int8_t, uint8_t>(int8_t value) { return safe_cast::int8_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<int8_t, uint16_t>(int8_t value) { return safe_cast::int8_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<int8_t, uint32_t>(int8_t value) { return safe_cast::int8_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<int8_t, uint64_t>(int8_t value) { return safe_cast::int8_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<int8_t, float>(int8_t value) { return safe_cast::int8_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<int8_t, double>(int8_t value) { return safe_cast::int8_to_double(value); }

// int16_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<int16_t, bool>(int16_t value) { return safe_cast::int16_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<int16_t, int8_t>(int16_t value) { return safe_cast::int16_to_int8(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<int16_t, int32_t>(int16_t value) { return safe_cast::int16_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<int16_t, int64_t>(int16_t value) { return safe_cast::int16_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<int16_t, uint8_t>(int16_t value) { return safe_cast::int16_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<int16_t, uint16_t>(int16_t value) { return safe_cast::int16_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<int16_t, uint32_t>(int16_t value) { return safe_cast::int16_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<int16_t, uint64_t>(int16_t value) { return safe_cast::int16_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<int16_t, float>(int16_t value) { return safe_cast::int16_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<int16_t, double>(int16_t value) { return safe_cast::int16_to_double(value); }

// int32_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<int32_t, bool>(int32_t value) { return safe_cast::int32_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<int32_t, int8_t>(int32_t value) { return safe_cast::int32_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<int32_t, int16_t>(int32_t value) { return safe_cast::int32_to_int16(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<int32_t, int64_t>(int32_t value) { return safe_cast::int32_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<int32_t, uint8_t>(int32_t value) { return safe_cast::int32_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<int32_t, uint16_t>(int32_t value) { return safe_cast::int32_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<int32_t, uint32_t>(int32_t value) { return safe_cast::int32_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<int32_t, uint64_t>(int32_t value) { return safe_cast::int32_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<int32_t, float>(int32_t value) { return safe_cast::int32_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<int32_t, double>(int32_t value) { return safe_cast::int32_to_double(value); }

// int64_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<int64_t, bool>(int64_t value) { return safe_cast::int64_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<int64_t, int8_t>(int64_t value) { return safe_cast::int64_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<int64_t, int16_t>(int64_t value) { return safe_cast::int64_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<int64_t, int32_t>(int64_t value) { return safe_cast::int64_to_int32(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<int64_t, uint8_t>(int64_t value) { return safe_cast::int64_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<int64_t, uint16_t>(int64_t value) { return safe_cast::int64_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<int64_t, uint32_t>(int64_t value) { return safe_cast::int64_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<int64_t, uint64_t>(int64_t value) { return safe_cast::int64_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<int64_t, float>(int64_t value) { return safe_cast::int64_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<int64_t, double>(int64_t value) { return safe_cast::int64_to_double(value); }

// uint8_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<uint8_t, bool>(uint8_t value) { return safe_cast::uint8_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<uint8_t, int8_t>(uint8_t value) { return safe_cast::uint8_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<uint8_t, int16_t>(uint8_t value) { return safe_cast::uint8_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<uint8_t, int32_t>(uint8_t value) { return safe_cast::uint8_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<uint8_t, int64_t>(uint8_t value) { return safe_cast::uint8_to_int64(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<uint8_t, uint16_t>(uint8_t value) { return safe_cast::uint8_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<uint8_t, uint32_t>(uint8_t value) { return safe_cast::uint8_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<uint8_t, uint64_t>(uint8_t value) { return safe_cast::uint8_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<uint8_t, float>(uint8_t value) { return safe_cast::uint8_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<uint8_t, double>(uint8_t value) { return safe_cast::uint8_to_double(value); }

// uint16_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<uint16_t, bool>(uint16_t value) { return safe_cast::uint16_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<uint16_t, int8_t>(uint16_t value) { return safe_cast::uint16_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<uint16_t, int16_t>(uint16_t value) { return safe_cast::uint16_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<uint16_t, int32_t>(uint16_t value) { return safe_cast::uint16_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<uint16_t, int64_t>(uint16_t value) { return safe_cast::uint16_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<uint16_t, uint8_t>(uint16_t value) { return safe_cast::uint16_to_uint8(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<uint16_t, uint32_t>(uint16_t value) { return safe_cast::uint16_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<uint16_t, uint64_t>(uint16_t value) { return safe_cast::uint16_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<uint16_t, float>(uint16_t value) { return safe_cast::uint16_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<uint16_t, double>(uint16_t value) { return safe_cast::uint16_to_double(value); }

// uint32_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<uint32_t, bool>(uint32_t value) { return safe_cast::uint32_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<uint32_t, int8_t>(uint32_t value) { return safe_cast::uint32_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<uint32_t, int16_t>(uint32_t value) { return safe_cast::uint32_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<uint32_t, int32_t>(uint32_t value) { return safe_cast::uint32_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<uint32_t, int64_t>(uint32_t value) { return safe_cast::uint32_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<uint32_t, uint8_t>(uint32_t value) { return safe_cast::uint32_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<uint32_t, uint16_t>(uint32_t value) { return safe_cast::uint32_to_uint16(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<uint32_t, uint64_t>(uint32_t value) { return safe_cast::uint32_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<uint32_t, float>(uint32_t value) { return safe_cast::uint32_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<uint32_t, double>(uint32_t value) { return safe_cast::uint32_to_double(value); }

// uint64_t specializations
template <> CASTLE_INLINE bool SAFE_CAST<uint64_t, bool>(uint64_t value) { return safe_cast::uint64_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<uint64_t, int8_t>(uint64_t value) { return safe_cast::uint64_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<uint64_t, int16_t>(uint64_t value) { return safe_cast::uint64_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<uint64_t, int32_t>(uint64_t value) { return safe_cast::uint64_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<uint64_t, int64_t>(uint64_t value) { return safe_cast::uint64_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<uint64_t, uint8_t>(uint64_t value) { return safe_cast::uint64_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<uint64_t, uint16_t>(uint64_t value) { return safe_cast::uint64_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<uint64_t, uint32_t>(uint64_t value) { return safe_cast::uint64_to_uint32(value); }
template <> CASTLE_INLINE float SAFE_CAST<uint64_t, float>(uint64_t value) { return safe_cast::uint64_to_float(value); }
template <> CASTLE_INLINE double SAFE_CAST<uint64_t, double>(uint64_t value) { return safe_cast::uint64_to_double(value); }

// float specializations
template <> CASTLE_INLINE bool SAFE_CAST<float, bool>(float value) { return safe_cast::float_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<float, int8_t>(float value) { return safe_cast::float_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<float, int16_t>(float value) { return safe_cast::float_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<float, int32_t>(float value) { return safe_cast::float_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<float, int64_t>(float value) { return safe_cast::float_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<float, uint8_t>(float value) { return safe_cast::float_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<float, uint16_t>(float value) { return safe_cast::float_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<float, uint32_t>(float value) { return safe_cast::float_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<float, uint64_t>(float value) { return safe_cast::float_to_uint64(value); }
template <> CASTLE_INLINE double SAFE_CAST<float, double>(float value) { return safe_cast::float_to_double(value); }

// double specializations
template <> CASTLE_INLINE bool SAFE_CAST<double, bool>(double value) { return safe_cast::double_to_bool(value); }
template <> CASTLE_INLINE int8_t SAFE_CAST<double, int8_t>(double value) { return safe_cast::double_to_int8(value); }
template <> CASTLE_INLINE int16_t SAFE_CAST<double, int16_t>(double value) { return safe_cast::double_to_int16(value); }
template <> CASTLE_INLINE int32_t SAFE_CAST<double, int32_t>(double value) { return safe_cast::double_to_int32(value); }
template <> CASTLE_INLINE int64_t SAFE_CAST<double, int64_t>(double value) { return safe_cast::double_to_int64(value); }
template <> CASTLE_INLINE uint8_t SAFE_CAST<double, uint8_t>(double value) { return safe_cast::double_to_uint8(value); }
template <> CASTLE_INLINE uint16_t SAFE_CAST<double, uint16_t>(double value) { return safe_cast::double_to_uint16(value); }
template <> CASTLE_INLINE uint32_t SAFE_CAST<double, uint32_t>(double value) { return safe_cast::double_to_uint32(value); }
template <> CASTLE_INLINE uint64_t SAFE_CAST<double, uint64_t>(double value) { return safe_cast::double_to_uint64(value); }
template <> CASTLE_INLINE float SAFE_CAST<double, float>(double value) { return safe_cast::double_to_float(value); }

} // namespace castle

#endif // CASTLE_UTILITY_SAFE_CAST_H
