# Geohash

## Overview

`geohash<Depth, T = double>` is a fixed-precision Geohash value and codec for converting geographic coordinates to the standard compact ASCII base-32 representation, to a binary bit form, and back. `Depth` selects the number of base-32 characters at compile time; `T` selects the floating-point coordinate type and defaults to `double`. The public coordinate alias is `point_type = geo_point<T>`, and decoded bounds use the same numeric type through `bounds_type = geohash_bounds<T>`.

An instance stores every form of one cell: the base-32 text (`Depth` characters), the binary text (`bit_count` characters of '0' and '1'), the packed bits, the cell center, and the cell bounds. An instance is initialized by `encode()` from a coordinate, or by `assign()`, `assign_binary()`, or `assign_bits()` from parsed base-32 text, binary text, or packed bits. Each successful call computes all forms together, and a failed call preserves the previous value. The results match the reference Python `geohash.py` encoder and decoder. Geohash is useful for compact location identifiers, spatial bucketing, and prefix-based geographic indexing. A hash identifies a rectangular geographic cell, not an exact point: different coordinates inside the same cell intentionally produce the same hash.

## Header

`#include "castle_ext/geodetic/geohash.hpp"`

## Dependencies

- [`coord.md`](coord.md) — `geo_point<T>` coordinate representation.
- [`geo_utils.md`](geo_utils.md) — latitude and longitude range validation.
- `castle/container/string.hpp` — fixed-capacity owning string storage.
- `castle/container/string_view.hpp` — non-owning text views.
- `castle/error/status.hpp` — deterministic error reporting.
- `castle/core/compiler.hpp`, `castle/core/types.hpp`, and `castle/core/traits.hpp` — compiler annotations, common size types, and floating-point type validation.

The simplified repository does not provide separate Markdown pages for these core headers. No C++ standard-library headers are required by this component.

## Public API

### Compile-time depth and coordinate type

```cpp
template <castle::size_type Depth, typename T = double>
class geohash;
```

`Depth` is the number of base-32 characters and must satisfy `1 <= Depth <= 12`. The limit is checked with `static_assert` when the type is instantiated. Consequently, `geohash<8U>`, `geohash<8U, float>`, and `geohash<12U>` are valid types, while `geohash<13U>` and `geohash<0U>` are compile-time errors.

`T` must be a floating-point type supported by `castle::meta::is_floating_point` (normally `float`, `double`, or `long double`). It defaults to `double`, so existing `geohash<8U>` declarations retain double-precision behavior. Use `geohash<8U, float>` when the application stores coordinates as `float`, or `geohash<8U, long double>` when the target/compiler provides and needs the wider type. `T` is used for input points, bisection arithmetic, decoded bounds, and output cell centers.

There is no runtime precision argument. Changing either template parameter creates a different C++ type and fixes the object's storage and coordinate types at compile time.

### Public aliases and constants

Each `geohash<Depth, T>` specialization exposes:

| Alias / constant | Meaning |
|---|---|
| `point_type` | `geo_point<T>`. |
| `bounds_type` | `geohash_bounds<T>`. |
| `string_type` | `castle::container::basic_string<char, Depth>`. |
| `view_type` | `castle::container::string_view`. |
| `depth` | Compile-time depth, equal to `Depth`. |
| `bits_type` | `uint64_t`, the packed binary representation. |
| `bit_count` | Number of binary digits, equal to `Depth * 5`. |
| `binary_string_type` | `castle::container::basic_string<char, bit_count>` holding '0' and '1' characters. |

### `geohash_bounds<T = double>`

A templated structure describing the rectangle for a decoded hash. `T` must be floating point and defaults to `double` for convenient standalone declarations such as `geohash_bounds<> bounds;` or `geohash_bounds bounds;` when the default is intended.

| Member | Meaning |
|---|---|
| `min_latitude`, `max_latitude` | Numeric latitude split boundaries in degrees, represented as `T`. |
| `min_longitude`, `max_longitude` | Numeric longitude split boundaries in degrees, represented as `T`. |
| `center_latitude()` | Arithmetic center latitude, returned as `T`. |
| `center_longitude()` | Arithmetic center longitude, returned as `T`. |
| `center()` | Cell center as `geo_point<T>`. |
| `latitude_error()` | Half-size of the cell in latitude, equal to the Python `lat_err`. |
| `longitude_error()` | Half-size of the cell in longitude, equal to the Python `lon_err`. |

The maximum edges are the bisection boundaries of the cell. The cell is conceptually half-open on maximum edges except where it meets the outer world bounds.

### `geohash<Depth, T>` methods

| Signature | Description |
|---|---|
| `status encode(const point_type& point)` | Encodes a coordinate and stores every form of its cell. Preserves the previous value if coordinate validation fails. O(Depth). |
| `status assign(view_type encoded)` | Loads base-32 text of exactly `Depth` characters and stores every form of its cell. |
| `status assign(const char* encoded)` | Loads a null-terminated base-32 string of exactly `Depth` characters. |
| `status assign_binary(view_type binary)` | Loads exactly `bit_count` '0'/'1' characters and stores every form of its cell. |
| `status assign_bits(bits_type bits)` | Loads packed bits and stores every form of their cell. Bits at or above `bit_count` yield `status::out_of_range`. |
| `view_type view() const` | Returns the base-32 text as a non-owning view. |
| `const char* c_str() const` | Returns the null-terminated base-32 text. |
| `const string_type& base32() const` | Returns the owned base-32 string. |
| `const binary_string_type& binary() const` | Returns the owned `bit_count` characters of '0' and '1'. |
| `bits_type bits() const` | Returns the packed bits, most significant bit first. |
| `const point_type& center() const` | Returns the center of the stored cell. |
| `const bounds_type& bounds() const` | Returns the boundaries of the stored cell. |
| `bool empty() const` | Tests whether the object contains a cell. A default-constructed object is empty; its center and bounds are not meaningful. |
| `static status encode(const point_type&, char*, castle::size_type)` | Encodes directly into a caller-owned buffer of at least `Depth + 1` bytes. |
| `static status decode(view_type, point_type&, bounds_type* = nullptr)` | Decodes a view without first storing it; its length must equal `Depth`. |
| `static status encode_bits(const point_type&, bits_type&)` | Encodes into packed bits, most significant bit first. The output is unchanged on failure. |
| `static status decode_bits(bits_type, point_type&, bounds_type* = nullptr)` | Decodes packed bits. Any bit at or above position `bit_count` yields `status::out_of_range`. |
| `static status encode_binary(const point_type&, binary_string_type&)` | Encodes into exactly `bit_count` '0'/'1' characters. |
| `static status decode_binary(view_type, point_type&, bounds_type* = nullptr)` | Decodes exactly `bit_count` '0'/'1' characters. |
| `static status base32_to_binary(view_type, binary_string_type&)` | Converts exactly `Depth` base-32 characters into binary text. |
| `static status binary_to_base32(view_type, string_type&)` | Converts exactly `bit_count` binary characters into base-32 text. |

All APIs return `castle::status`; they do not throw exceptions.

### Binary form

The binary form is the same cell written as `bit_count = Depth * 5` bits, most significant bit first. The first bit is the first longitude bit, so the text matches the reference Python `encode_binary` output. Packed bits occupy the low `bit_count` bits of `bits_type`; there are no set bits above that position.

Binary text must have exactly `bit_count` characters. This differs from the Python reference `decode --binary`, which accepts any length, because a castle `geohash<Depth>` is fixed-depth.

### Status behavior

| Operation | Condition | Status |
|---|---|---|
| Encode | Valid coordinate and enough destination space | `status::ok` |
| Encode | Null raw destination or latitude/longitude outside valid ranges | `status::invalid_argument` |
| Encode | Raw destination smaller than `Depth + 1` bytes | `status::full` |
| Decode / assign | Empty base-32 or binary text | `status::empty` |
| Decode / assign | Hash text longer than `Depth` | `status::out_of_range` |
| Decode / assign | Short text, null non-empty view, or invalid alphabet character | `status::invalid_argument` |
| Assign from C string | Null pointer | `status::invalid_argument` |
| Binary decode / assign / convert | Empty binary text | `status::empty` |
| Binary decode / assign / convert | Binary text longer than `bit_count` | `status::out_of_range` |
| Binary decode / assign / convert | Binary text shorter than `bit_count`, or a character other than '0' or '1' | `status::invalid_argument` |
| Decode / assign bits | Bits set at or above position `bit_count` | `status::out_of_range` |

The valid alphabet is `0123456789bcdefghjkmnpqrstuvwxyz`. ASCII uppercase is accepted on input and normalized to lowercase in every stored text form.

## Usage Example

```cpp
#include <assert.h>
#include "castle_ext/geodetic/geohash.hpp"

int main()
{
    using namespace castle::geodetic;

    const geo_point<double> position(16.066389083862305, 108.21378707885742);

    // Omitting T selects double; encode() stores every form of the cell.
    geohash<8U> hash;
    assert(hash.encode(position) == castle::status::ok);
    assert(hash.base32().view().compare(castle::container::string_view("w6ugqkyc")) == 0);
    assert(hash.binary().view().compare(castle::container::string_view("1110000110110100111110110100101111001011")) == 0);
    assert(hash.bits() == 969404009419ULL);
    assert(hash.bounds().min_latitude <= position.latitude());
    assert(position.latitude() <= hash.bounds().max_latitude);
    assert(hash.center() == hash.bounds().center());

    // Each parsed form initializes the same cell.
    geohash<8U> from_text;
    assert(from_text.assign(castle::container::string_view("W6UGQKYC")) == castle::status::ok);
    assert(from_text.base32() == hash.base32());

    geohash<8U> from_binary;
    assert(from_binary.assign_binary(hash.binary().view()) == castle::status::ok);
    assert(from_binary.bits() == hash.bits());

    geohash<8U> from_bits;
    assert(from_bits.assign_bits(hash.bits()) == castle::status::ok);
    assert(from_bits.center() == hash.center());

    // A different compile-time depth is a different hash type.
    geohash<12U> finer_hash;
    assert(finer_hash.encode(position) == castle::status::ok);

    // The second template parameter selects float coordinates and bounds.
    using float_hash = geohash<8U, float>;
    float_hash float_codec;
    assert(float_codec.encode(float_hash::point_type(16.0664F, 108.2138F)) == castle::status::ok);
    assert(float_codec.bounds().min_latitude <= 16.0664F);
    return 0;
}
```

See `samples/castle_ext/geodetic/geohash.cpp` for a standalone example covering the default `double` type, a `float` specialization, base-32, binary, and packed-bit initialization, raw-buffer encoding, static decoding, and Python reference vectors.

## Geohash Algorithm

### 1. Initial geographic interval

The entire world is the starting rectangle:

- Longitude interval: `[-180, +180]` degrees.
- Latitude interval: `[-90, +90]` degrees.

The algorithm does not project coordinates or calculate distances. It repeatedly bisects these angular intervals using the selected coordinate type `T`.

### 2. Alternate longitude and latitude bits

The first bit always represents longitude, the second latitude, and the choice alternates thereafter:

`longitude, latitude, longitude, latitude, ...`

For the currently selected axis, calculate its midpoint:

```text
midpoint = (minimum + maximum) / 2
```

Then choose one half:

- If the coordinate is greater than or equal to the midpoint, emit bit `1` and replace the interval minimum with the midpoint (east for longitude, north for latitude).
- Otherwise, emit bit `0` and replace the interval maximum with the midpoint (west for longitude, south for latitude).

Using `>=` makes points exactly on an internal split consistently select the upper/eastern half. The current interval on the other axis is left unchanged until its next turn. Every interval and midpoint has type `T` in a `geohash<Depth, T>` specialization.

### 3. Group bits into five-bit values

Five consecutive bits form one Geohash character. The first bit in the group is the most significant bit. For example:

```text
bits:          1 1 0 1 0
bit weights:  16  8  4  2  1
base-10:      16 + 8 + 0 + 2 + 0 = 26
```

The integer value 26 is then looked up in the Geohash alphabet. The mapping is:

```text
value:    0         9 10        17 18        21 22        31
alphabet: 0123456789 b cdefgh j k mn pqrstuvwxyz
```

The letters `a`, `i`, `l`, and `o` are intentionally omitted, avoiding ambiguous characters in the standard encoding. Values range from 0 through 31.

### 4. Stop after the compile-time depth

Each output character consumes five interleaved bits. `geohash<Depth, T>` therefore always emits exactly `5 * Depth` bits and `Depth` ASCII characters. There is no runtime loop bound supplied by the caller and no runtime depth mismatch during encoding. The object storage is `basic_string<char, Depth>`, so its logical character capacity is exactly the selected depth; a raw C buffer needs `Depth + 1` bytes to include the trailing null terminator.

### 5. Decode by reversing each bisection

Decoding starts again with the whole-world intervals. Each input character is mapped back to its integer value from the alphabet. Its five bits are inspected from most significant to least significant. For each bit, the decoder performs the same axis alternation and midpoint calculation as the encoder:

- Bit `0`: retain the lower/western or southern half by moving the interval maximum to the midpoint.
- Bit `1`: retain the upper/eastern or northern half by moving the interval minimum to the midpoint.

After all `5 * Depth` bits have been consumed, the remaining latitude and longitude intervals are the cell bounds. The decoder returns the arithmetic midpoint of each interval as `geo_point<T>` and can copy the four edges into `geohash_bounds<T>`.

Decoding cannot recover the original input coordinate: quantization has discarded the position within the cell. Every point in the same cell has the same hash. Different hashes identify different cells at the same depth, while nearby cells commonly share prefixes.

### 6. How precision controls resolution

The bit count is split between longitude and latitude because the axes alternate, beginning with longitude:

- Longitude bits: `ceil(5 * Depth / 2)`.
- Latitude bits: `floor(5 * Depth / 2)`.
- Longitude cell width: `360 / 2^(longitude bits)` degrees.
- Latitude cell height: `180 / 2^(latitude bits)` degrees.

Each additional character adds five bisection bits, reducing the cell area by a factor of 32 (subject to the alternating distribution between axes). A longer prefix is more spatially precise; a shorter prefix represents a larger parent cell. The physical dimensions in metres vary with latitude because the length of a longitude degree decreases toward the poles.

### 7. Supplied reference coordinate

For latitude `16.066389083862305` and longitude `108.21378707885742`, the default `geohash<8U>` produces:

```text
w6ugqkyc

latitude bounds:
  16.066303253173828 .. 16.066474914550781

longitude bounds:
  108.21361541748047 .. 108.21395874023438
```

The midpoint of this rectangle is returned on decode. Precision 9, 12, and other supported depths follow the same algorithm with smaller cells.

### 8. Binary form and base-32 conversion

The bisection produces `bit_count` bits, which are packed into a `uint64_t` in encoding order. Base-32 character `i` is the five-bit field `(bits >> (5 * (Depth - 1 - i))) & 0x1F`, mapped through the alphabet. Binary text writes the same bits as '0' and '1' characters. Both conversions are O(Depth) and allocation-free, and they are exact inverses for valid input.

## Constraints & Notes

- **Compile-time depth:** valid range is 1 through 12 characters inclusive. `geohash<0U>` and `geohash<13U>` fail via `static_assert` when instantiated.
- **Coordinate type:** `T` defaults to `double` and must be a floating-point type. `point_type` is exactly `geo_point<T>`; `bounds_type` is exactly `geohash_bounds<T>`.
- **Numeric precision:** bisection arithmetic and decoded bounds use `T`. `double` is recommended for precision 9–12. A lower-precision type such as `float` can no longer represent increasingly small cell boundaries at the maximum depths, so its final bounds may collapse to the same representable value and its effective spatial precision is limited by the type.
- **Memory:** each instance stores `Depth` base-32 characters, `bit_count` binary characters, one `uint64_t` of packed bits, and one center point with its bounds, all fixed-size. Text forms carry a null terminator. Encoding uses bounded local buffers. No heap allocation occurs.
- **Canonical text:** the base-32 and binary forms are always derived from the packed bits, so stored text is lowercase and every form of a cell agrees.
- **Dependencies:** only CASTLE headers are used; there is no STL dependency.
- **Runtime model:** deterministic bounded loops, no exceptions, RTTI, virtual functions, or dynamic allocation.
- **Thread-safety:** separate instances can be used independently. Concurrent mutation/read of the same instance requires external synchronization.
- **Input ranges:** latitude must be in `[-90, +90]`; longitude must be in `[-180, +180]`. Coordinates are decimal degrees.
- **Prefix semantics:** a shorter hash represents a larger cell containing the cells represented by its longer extensions. Two different coordinates can have the same hash when they fall in the same cell.
- **Complexity:** encode and decode are O(Depth), where `Depth <= 12`; validation is O(Depth), and object accessors are O(1).
- **Binary width:** `bits_type` is `uint64_t`, which holds the 60 bits of the maximum depth. A static assertion checks that it is unsigned and wide enough for `bit_count`.
