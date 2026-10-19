// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file geohash.hpp
 * @brief Fixed-depth, allocation-free Geohash value that stores base-32, binary, bit, center, and bounds forms.
 *
 * The coordinate representation is configurable through geohash<Depth, T>,
 * with double as the default, and precision is selected at compile time.
 * Supported depths are 1..12 characters;
 * attempting to instantiate a depth outside this range is a compile-time
 * error. Every successful initialization stores all forms of the same cell.
 *
 * @code
 * #include "castle_ext/geodetic/geohash.hpp"
 *
 * CASTLE_CONST castle::geodetic::geo_point<double> position(16.066389, 108.213787);
 * castle::geodetic::geohash<8U> hash; // T defaults to double
 * CASTLE_CONST castle::status encoded = hash.encode(position);
 *
 * hash.base32();   // "w6ugqkyc"
 * hash.binary();   // 40 characters of '0'/'1'
 * hash.bits();     // packed uint64_t
 * hash.center();   // geo_point<double>
 * hash.bounds();   // geohash_bounds<double>
 *
 * castle::geodetic::geohash<8U> parsed;
 * parsed.assign(castle::container::string_view("w6ugqkyc"));
 * parsed.assign_binary(hash.binary().view());
 * parsed.assign_bits(hash.bits());
 * @endcode
 *
 * @note The implementation uses longitude-first bit interleaving and the
 * standard Geohash base-32 alphabet, matching the reference Python encoder.
 * Packed bits occupy the low `Depth * 5` bits of a `uint64_t`. It performs
 * O(Depth) work and uses only bounded object/stack storage. No heap allocation
 * or C++ standard library dependency is required.
 */
#ifndef CASTLE_EXT_GEODETIC_GEOHASH_HPP
#define CASTLE_EXT_GEODETIC_GEOHASH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/bit/bit_core.hpp"
#include "castle/bit/bit_utils.hpp"
#include "castle/error/status.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"
#include "castle_ext/geodetic/coord.hpp"
#include "castle_ext/geodetic/geo_utils.hpp"

namespace castle
{
namespace geodetic
{

/** @brief Maximum supported compile-time Geohash depth, in characters. */
static CASTLE_CONSTEXPR castle::size_type geohash_max_depth = 12U;

/** @brief Number of binary digits in a Geohash of `Depth` base-32 characters. */
template <castle::size_type Depth>
static CASTLE_CONSTEXPR castle::size_type geohash_bit_count = Depth * 5U;

/**
 * @brief Geographic rectangle represented by a decoded Geohash.
 * @tparam T Floating-point coordinate type (defaults to double).
 *
 * The boundaries are in decimal degrees. The represented cell is conceptually
 * half-open at its maximum edges, except where it touches the outer world
 * limits; the stored values are the exact numeric split boundaries.
 */
template <typename T = double>
struct geohash_bounds
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "geohash_bounds requires a floating-point type");

    T min_latitude;
    T max_latitude;
    T min_longitude;
    T max_longitude;

    /** @brief Initializes the bounds to the entire geographic world. */
    CASTLE_CONSTEXPR geohash_bounds() CASTLE_NOEXCEPT
        : min_latitude(static_cast<T>(-90))
        , max_latitude(static_cast<T>(90))
        , min_longitude(static_cast<T>(-180))
        , max_longitude(static_cast<T>(180))
    {
    }

    /** @brief Returns the center latitude of this cell. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T center_latitude() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (min_latitude + max_latitude) / static_cast<T>(2);
    }

    /** @brief Returns the center longitude of this cell. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T center_longitude() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (min_longitude + max_longitude) / static_cast<T>(2);
    }

    /** @brief Returns the center of this cell as a geographic point. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR geo_point<T> center() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return geo_point<T>(center_latitude(), center_longitude());
    }

    /** @brief Returns the half-size of this cell in latitude, in degrees. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T latitude_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (max_latitude - min_latitude) / static_cast<T>(2);
    }

    /** @brief Returns the half-size of this cell in longitude, in degrees. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T longitude_error() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (max_longitude - min_longitude) / static_cast<T>(2);
    }
};

namespace detail
{

/** @brief Standard Geohash base-32 alphabet; the index is the 5-bit digit value. */
static CASTLE_CONSTEXPR char geohash_alphabet[] = "0123456789bcdefghjkmnpqrstuvwxyz";
/** @brief Number of symbols in the Geohash base-32 alphabet. */
static CASTLE_CONSTEXPR castle::size_type geohash_radix = 32U;

/** @brief Returns the standard Geohash base-32 digit for a value in [0, 31]. */
CASTLE_CONSTEXPR char geohash_digit(castle::size_type value) CASTLE_NOEXCEPT
{
    return geohash_alphabet[value];
}

/** @brief Maps a standard Geohash character to [0, 31], accepting ASCII uppercase. */
CASTLE_INLINE int geohash_digit_value(char character) CASTLE_NOEXCEPT
{
    if ((character >= 'A') && (character <= 'Z'))
    {
        character = static_cast<char>(character - 'A' + 'a');
    }

    CASTLE_CONST castle::container::string_view alphabet(geohash_alphabet, geohash_radix);
    CASTLE_CONST castle::size_type index = alphabet.find(character);
    return (index == castle::container::string_view::npos) ? -1 : static_cast<int>(index);
}

/** @brief Validates an exact-depth view and its alphabet without changing output. */
template <castle::size_type Depth>
CASTLE_NODISCARD status validate_geohash(castle::container::string_view encoded) CASTLE_NOEXCEPT
{
    if (encoded.empty())
    {
        return status::empty;
    }
    if (encoded.data() == nullptr)
    {
        return status::invalid_argument;
    }
    if (encoded.size() != Depth)
    {
        return (encoded.size() > Depth) ? status::out_of_range : status::invalid_argument;
    }

    for (castle::size_type i = 0U; i < Depth; ++i)
    {
        if (geohash_digit_value(encoded[i]) < 0)
        {
            return status::invalid_argument;
        }
    }

    return status::ok;
}

/** @brief Maps a binary Geohash character '0' or '1' to 0 or 1, otherwise -1. */
CASTLE_CONSTEXPR int geohash_bit_value(char character) CASTLE_NOEXCEPT
{
    return (character == '0') ? 0 : ((character == '1') ? 1 : -1);
}

/** @brief Validates an exact-length binary view and its characters without changing output. */
template <castle::size_type Depth>
CASTLE_NODISCARD status validate_geohash_binary(castle::container::string_view encoded) CASTLE_NOEXCEPT
{
    CASTLE_CONST castle::size_type bit_count = geohash_bit_count<Depth>;
    if (encoded.empty())
    {
        return status::empty;
    }
    if (encoded.data() == nullptr)
    {
        return status::invalid_argument;
    }
    if (encoded.size() != bit_count)
    {
        return (encoded.size() > bit_count) ? status::out_of_range : status::invalid_argument;
    }

    for (castle::size_type i = 0U; i < bit_count; ++i)
    {
        if (geohash_bit_value(encoded[i]) < 0)
        {
            return status::invalid_argument;
        }
    }

    return status::ok;
}

/** @brief Packs an exact-depth base-32 view into MSB-first Geohash bits. */
template <castle::size_type Depth>
CASTLE_NODISCARD status base32_to_bits(castle::container::string_view encoded, uint64_t& bits) CASTLE_NOEXCEPT
{
    CASTLE_CONST status validation = validate_geohash<Depth>(encoded);
    if (validation != status::ok)
    {
        return validation;
    }

    uint64_t packed = 0U;
    for (castle::size_type character_index = 0U; character_index < Depth; ++character_index)
    {
        packed = (packed << 5U) | static_cast<uint64_t>(geohash_digit_value(encoded[character_index]));
    }

    bits = packed;
    return status::ok;
}

/** @brief Packs an exact-length binary view into MSB-first Geohash bits. */
template <castle::size_type Depth>
CASTLE_NODISCARD status binary_to_bits(castle::container::string_view encoded, uint64_t& bits) CASTLE_NOEXCEPT
{
    CASTLE_CONST status validation = validate_geohash_binary<Depth>(encoded);
    if (validation != status::ok)
    {
        return validation;
    }

    uint64_t packed = 0U;
    for (castle::size_type bit_index = 0U; bit_index < geohash_bit_count<Depth>; ++bit_index)
    {
        packed = (packed << 1U) | static_cast<uint64_t>(geohash_bit_value(encoded[bit_index]));
    }

    bits = packed;
    return status::ok;
}

/** @brief Renders packed bits as Depth base-32 characters and a null terminator. */
template <castle::size_type Depth>
CASTLE_NODISCARD status bits_to_base32(uint64_t bits, char* output, castle::size_type output_capacity) CASTLE_NOEXCEPT
{
    if (output == nullptr)
    {
        return status::invalid_argument;
    }
    if (output_capacity < (Depth + 1U))
    {
        return status::full;
    }
    if ((bits >> geohash_bit_count<Depth>) != 0U)
    {
        return status::out_of_range;
    }

    for (castle::size_type character_index = 0U; character_index < Depth; ++character_index)
    {
        CASTLE_CONST uint32_t shift = static_cast<uint32_t>(5U * (Depth - 1U - character_index));
        output[character_index] = geohash_digit(static_cast<castle::size_type>(castle::bit::extract_field(bits, shift, 5U)));
    }

    output[Depth] = '\0';
    return status::ok;
}

/** @brief Renders packed bits as `bit_count` '0'/'1' characters and a null terminator. */
template <castle::size_type Depth>
CASTLE_NODISCARD status bits_to_binary(uint64_t bits, char* output, castle::size_type output_capacity) CASTLE_NOEXCEPT
{
    CASTLE_CONST castle::size_type bit_count = geohash_bit_count<Depth>;
    if (output == nullptr)
    {
        return status::invalid_argument;
    }
    if (output_capacity < (bit_count + 1U))
    {
        return status::full;
    }
    if ((bits >> bit_count) != 0U)
    {
        return status::out_of_range;
    }

    for (castle::size_type bit_index = 0U; bit_index < bit_count; ++bit_index)
    {
        output[bit_index] = castle::bit::test(bits, static_cast<uint32_t>(bit_count - 1U - bit_index)) ? '1' : '0';
    }

    output[bit_count] = '\0';
    return status::ok;
}

/** @brief Encodes a valid coordinate into MSB-first Geohash bits of width `Depth * 5`. */
template <castle::size_type Depth, typename T>
CASTLE_NODISCARD status encode_geohash_bits(CASTLE_CONST geo_point<T>& point, uint64_t& bits) CASTLE_NOEXCEPT
{
    if (!is_valid_latitude(point.latitude()) || !is_valid_longitude(point.longitude()))
    {
        return status::invalid_argument;
    }

    T min_latitude = static_cast<T>(-90);
    T max_latitude = static_cast<T>(90);
    T min_longitude = static_cast<T>(-180);
    T max_longitude = static_cast<T>(180);
    bool use_longitude = true;
    uint64_t packed = 0U;

    for (castle::size_type bit_index = 0U; bit_index < geohash_bit_count<Depth>; ++bit_index)
    {
        packed <<= 1U;

        if (use_longitude)
        {
            CASTLE_CONST T midpoint = (min_longitude + max_longitude) / static_cast<T>(2);
            if (point.longitude() >= midpoint)
            {
                packed |= 1U;
                min_longitude = midpoint;
            }
            else
            {
                max_longitude = midpoint;
            }
        }
        else
        {
            CASTLE_CONST T midpoint = (min_latitude + max_latitude) / static_cast<T>(2);
            if (point.latitude() >= midpoint)
            {
                packed |= 1U;
                min_latitude = midpoint;
            }
            else
            {
                max_latitude = midpoint;
            }
        }

        use_longitude = !use_longitude;
    }

    bits = packed;
    return status::ok;
}

/** @brief Decodes MSB-first Geohash bits into the cell center and bounds. */
template <castle::size_type Depth, typename T>
CASTLE_NODISCARD status decode_geohash_bits(uint64_t bits,
                                             geo_point<T>& center,
                                             geohash_bounds<T>* bounds) CASTLE_NOEXCEPT
{
    if ((bits >> geohash_bit_count<Depth>) != 0U)
    {
        return status::out_of_range;
    }

    geohash_bounds<T> decoded_bounds;
    bool use_longitude = true;

    for (castle::size_type bit_index = 0U; bit_index < geohash_bit_count<Depth>; ++bit_index)
    {
        CASTLE_CONST bool set = castle::bit::test(bits, static_cast<uint32_t>(geohash_bit_count<Depth> - 1U - bit_index));

        if (use_longitude)
        {
            CASTLE_CONST T midpoint = (decoded_bounds.min_longitude + decoded_bounds.max_longitude) / static_cast<T>(2);
            if (set)
            {
                decoded_bounds.min_longitude = midpoint;
            }
            else
            {
                decoded_bounds.max_longitude = midpoint;
            }
        }
        else
        {
            CASTLE_CONST T midpoint = (decoded_bounds.min_latitude + decoded_bounds.max_latitude) / static_cast<T>(2);
            if (set)
            {
                decoded_bounds.min_latitude = midpoint;
            }
            else
            {
                decoded_bounds.max_latitude = midpoint;
            }
        }

        use_longitude = !use_longitude;
    }

    center = decoded_bounds.center();
    if (bounds != nullptr)
    {
        *bounds = decoded_bounds;
    }

    return status::ok;
}

/** @brief Encodes a valid coordinate to a fixed-depth caller-owned base-32 buffer. */
template <castle::size_type Depth, typename T>
CASTLE_NODISCARD status encode_geohash(CASTLE_CONST geo_point<T>& point,
                                        char* output,
                                        castle::size_type output_capacity) CASTLE_NOEXCEPT
{
    if (output == nullptr)
    {
        return status::invalid_argument;
    }
    if (output_capacity < (Depth + 1U))
    {
        return status::full;
    }

    uint64_t bits = 0U;
    CASTLE_CONST status result = encode_geohash_bits<Depth, T>(point, bits);
    if (result != status::ok)
    {
        return result;
    }

    return bits_to_base32<Depth>(bits, output, output_capacity);
}

/** @brief Decodes an exact-depth base-32 Geohash into its cell center and bounds. */
template <castle::size_type Depth, typename T>
CASTLE_NODISCARD status decode_geohash(castle::container::string_view encoded,
                                        geo_point<T>& center,
                                        geohash_bounds<T>* bounds) CASTLE_NOEXCEPT
{
    uint64_t bits = 0U;
    CASTLE_CONST status conversion = base32_to_bits<Depth>(encoded, bits);
    if (conversion != status::ok)
    {
        return conversion;
    }

    return decode_geohash_bits<Depth, T>(bits, center, bounds);
}

} // namespace detail

/**
 * @brief Fixed-depth Geohash value that stores every form of one cell.
 *
 * @tparam Depth Number of base-32 characters. Must be in the range 1..12.
 * @tparam T Floating-point coordinate type. Defaults to double.
 *
 * A successful `encode()`, `assign()`, `assign_binary()`, or `assign_bits()`
 * stores the base-32 text, binary text, packed bits, cell center, and cell
 * bounds together. A failed call preserves the previous value.
 *
 * @note A depth outside 1..12 or a non-floating-point T fails at compile time.
 * Runtime inputs must contain exactly `Depth` valid characters.
 */
template <castle::size_type Depth, typename T = double>
class geohash
{
    static_assert(castle::meta::is_floating_point<T>::value,
                  "geohash coordinate type must be a floating-point type");
    static_assert((Depth >= 1U) && (Depth <= geohash_max_depth),
                  "geohash depth must be between 1 and 12 characters");

public:
    using point_type = geo_point<T>;
    using bounds_type = geohash_bounds<T>;
    using bits_type = uint64_t;
    using string_type = castle::container::string<Depth>;
    using binary_string_type = castle::container::string<geohash_bit_count<Depth>>;
    using view_type = castle::container::string_view;

    /** @brief Compile-time precision in base-32 characters. */
    static CASTLE_CONSTEXPR castle::size_type depth = Depth;

    /** @brief Number of binary digits; each base-32 character carries 5 bits. */
    static CASTLE_CONSTEXPR castle::size_type bit_count = geohash_bit_count<Depth>;

    static_assert(castle::meta::is_valid_integer<bits_type>::value && castle::meta::is_unsigned<bits_type>::value,
                  "geohash bits_type must be an unsigned integer");
    static_assert(sizeof(bits_type) * 8U >= bit_count,
                  "geohash bits_type must hold bit_count bits");

    /** @brief Constructs an empty hash value. */
    geohash() CASTLE_NOEXCEPT : bits_(0U) {}

    /**
     * @brief Encodes a coordinate and stores every form of its cell.
     * @param point Coordinate in decimal degrees.
     * @return `status::ok`, or `status::invalid_argument` for an invalid
     * latitude/longitude. The previous value is preserved on failure.
     * @note O(Depth) time and O(1) additional storage.
     */
    CASTLE_NODISCARD status encode(CASTLE_CONST point_type& point) CASTLE_NOEXCEPT
    {
        bits_type packed = 0U;
        CASTLE_CONST status result = encode_bits(point, packed);
        if (result != status::ok)
        {
            return result;
        }

        return assign_bits(packed);
    }

    /**
     * @brief Loads base-32 text and stores every form of its cell.
     * @param encoded Source text, which must contain exactly `Depth` characters.
     * @return `status::ok` on success, `status::empty` for empty input,
     * `status::out_of_range` for excessive length, and
     * `status::invalid_argument` for short input, a null non-empty view,
     * or a character outside the Geohash alphabet. The previous value is
     * preserved on failure.
     * @note ASCII uppercase is accepted and stored in canonical lowercase.
     */
    CASTLE_NODISCARD status assign(view_type encoded) CASTLE_NOEXCEPT
    {
        bits_type packed = 0U;
        CASTLE_CONST status result = detail::base32_to_bits<Depth>(encoded, packed);
        if (result != status::ok)
        {
            return result;
        }

        return assign_bits(packed);
    }

    /** @brief Assigns a null-terminated string containing exactly `Depth` characters. */
    CASTLE_NODISCARD status assign(CASTLE_CONST char* encoded) CASTLE_NOEXCEPT
    {
        if (encoded == nullptr)
        {
            return status::invalid_argument;
        }

        castle::size_type length = 0U;
        while ((length <= Depth) && (encoded[length] != '\0'))
        {
            ++length;
        }

        if (length == 0U)
        {
            return status::empty;
        }
        if (length > Depth)
        {
            return status::out_of_range;
        }

        return assign(view_type(encoded, length));
    }

    /** @brief Loads exactly `bit_count` '0' and '1' characters and stores every form of its cell. */
    CASTLE_NODISCARD status assign_binary(view_type binary_text) CASTLE_NOEXCEPT
    {
        bits_type packed = 0U;
        CASTLE_CONST status result = detail::binary_to_bits<Depth>(binary_text, packed);
        if (result != status::ok)
        {
            return result;
        }

        return assign_bits(packed);
    }

    /** @brief Loads packed bits and stores every form of their cell; bits at or above `bit_count` yield `status::out_of_range`. */
    CASTLE_NODISCARD status assign_bits(bits_type packed) CASTLE_NOEXCEPT
    {
        string_type base32_text;
        binary_string_type binary_text;
        point_type cell_center;
        bounds_type cell_bounds;

        status result = store_base32(packed, base32_text);
        if (result == status::ok)
        {
            result = store_binary(packed, binary_text);
        }
        if (result == status::ok)
        {
            result = detail::decode_geohash_bits<Depth, T>(packed, cell_center, &cell_bounds);
        }
        if (result != status::ok)
        {
            return result;
        }

        bits_ = packed;
        base32_ = base32_text;
        binary_ = binary_text;
        center_ = cell_center;
        bounds_ = cell_bounds;
        return status::ok;
    }

    /** @brief Returns the base-32 text as a non-owning view. */
    CASTLE_NODISCARD view_type view() CASTLE_CONST CASTLE_NOEXCEPT { return base32_.view(); }

    /** @brief Returns the null-terminated base-32 text. */
    CASTLE_NODISCARD CASTLE_CONST char* c_str() CASTLE_CONST CASTLE_NOEXCEPT { return base32_.c_str(); }

    /** @brief Returns the owned fixed-capacity base-32 string. */
    CASTLE_NODISCARD CASTLE_CONST string_type& base32() CASTLE_CONST CASTLE_NOEXCEPT { return base32_; }

    /** @brief Returns the owned `bit_count` characters of '0' and '1'. */
    CASTLE_NODISCARD CASTLE_CONST binary_string_type& binary() CASTLE_CONST CASTLE_NOEXCEPT { return binary_; }

    /** @brief Returns the packed bits, most significant bit first. */
    CASTLE_NODISCARD bits_type bits() CASTLE_CONST CASTLE_NOEXCEPT { return bits_; }

    /** @brief Returns the center of the stored cell. */
    CASTLE_NODISCARD CASTLE_CONST point_type& center() CASTLE_CONST CASTLE_NOEXCEPT { return center_; }

    /** @brief Returns the boundaries of the stored cell. */
    CASTLE_NODISCARD CASTLE_CONST bounds_type& bounds() CASTLE_CONST CASTLE_NOEXCEPT { return bounds_; }

    /** @brief Tests whether this object currently contains a Geohash. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return base32_.empty(); }

    /**
     * @brief Encodes directly into a caller-owned null-terminated buffer.
     * @param point Coordinate in decimal degrees.
     * @param output Destination buffer.
     * @param output_capacity Destination size including the null terminator.
     * @return `status::invalid_argument` for a null buffer or invalid point,
     * `status::full` for insufficient capacity, otherwise `status::ok`.
     */
    CASTLE_NODISCARD static status encode(CASTLE_CONST point_type& point,
                                           char* output,
                                           castle::size_type output_capacity) CASTLE_NOEXCEPT
    {
        return detail::encode_geohash<Depth, T>(point, output, output_capacity);
    }

    /**
     * @brief Decodes an exact-depth Geohash view without first storing it.
     * @param encoded Text containing exactly `Depth` valid characters.
     * @param center Receives the center coordinate.
     * @param bounds Optional destination for the cell boundaries.
     * @return See `assign()` for malformed-input statuses.
     */
    CASTLE_NODISCARD static status decode(view_type encoded,
                                          point_type& center,
                                          bounds_type* bounds = nullptr) CASTLE_NOEXCEPT
    {
        return detail::decode_geohash<Depth, T>(encoded, center, bounds);
    }

    /** @brief Encodes a coordinate into packed bits; only the low `bit_count` bits are significant. */
    CASTLE_NODISCARD static status encode_bits(CASTLE_CONST point_type& point, bits_type& bits) CASTLE_NOEXCEPT
    {
        return detail::encode_geohash_bits<Depth, T>(point, bits);
    }

    /** @brief Decodes packed bits into the cell center and optional bounds. */
    CASTLE_NODISCARD static status decode_bits(bits_type bits,
                                               point_type& center,
                                               bounds_type* bounds = nullptr) CASTLE_NOEXCEPT
    {
        return detail::decode_geohash_bits<Depth, T>(bits, center, bounds);
    }

    /** @brief Encodes a coordinate into a `bit_count`-character string of '0' and '1'. */
    CASTLE_NODISCARD static status encode_binary(CASTLE_CONST point_type& point, binary_string_type& binary) CASTLE_NOEXCEPT
    {
        bits_type bits = 0U;
        CASTLE_CONST status result = encode_bits(point, bits);
        if (result != status::ok)
        {
            return result;
        }

        return store_binary(bits, binary);
    }

    /** @brief Decodes exactly `bit_count` '0' and '1' characters into the cell center and bounds. */
    CASTLE_NODISCARD static status decode_binary(view_type binary,
                                                 point_type& center,
                                                 bounds_type* bounds = nullptr) CASTLE_NOEXCEPT
    {
        bits_type bits = 0U;
        CASTLE_CONST status result = detail::binary_to_bits<Depth>(binary, bits);
        if (result != status::ok)
        {
            return result;
        }

        return decode_bits(bits, center, bounds);
    }

    /** @brief Converts exactly Depth base-32 characters into `bit_count` binary characters. */
    CASTLE_NODISCARD static status base32_to_binary(view_type encoded, binary_string_type& binary) CASTLE_NOEXCEPT
    {
        bits_type bits = 0U;
        CASTLE_CONST status result = detail::base32_to_bits<Depth>(encoded, bits);
        if (result != status::ok)
        {
            return result;
        }

        return store_binary(bits, binary);
    }

    /** @brief Converts exactly `bit_count` binary characters into Depth base-32 characters. */
    CASTLE_NODISCARD static status binary_to_base32(view_type binary, string_type& encoded) CASTLE_NOEXCEPT
    {
        bits_type bits = 0U;
        CASTLE_CONST status result = detail::binary_to_bits<Depth>(binary, bits);
        if (result != status::ok)
        {
            return result;
        }

        return store_base32(bits, encoded);
    }

private:
    /** @brief Stores packed bits as `bit_count` binary characters. */
    static status store_binary(bits_type bits, binary_string_type& binary) CASTLE_NOEXCEPT
    {
        char text[bit_count + 1U];
        CASTLE_CONST status result = detail::bits_to_binary<Depth>(bits, text, sizeof(text));
        if (result != status::ok)
        {
            return result;
        }

        return binary.assign(view_type(text, bit_count));
    }

    /** @brief Stores packed bits as Depth base-32 characters. */
    static status store_base32(bits_type bits, string_type& encoded) CASTLE_NOEXCEPT
    {
        char text[Depth + 1U];
        CASTLE_CONST status result = detail::bits_to_base32<Depth>(bits, text, sizeof(text));
        if (result != status::ok)
        {
            return result;
        }

        return encoded.assign(view_type(text, Depth));
    }

    bits_type bits_;
    string_type base32_;
    binary_string_type binary_;
    point_type center_;
    bounds_type bounds_;
};

} // namespace geodetic
} // namespace castle

#endif // CASTLE_EXT_GEODETIC_GEOHASH_HPP
