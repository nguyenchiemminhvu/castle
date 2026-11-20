// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file count_min_sketch.hpp
 * @brief Fixed-size count-min sketch: approximate per-key counts that never undercount.
 *
 * Use `count_min_sketch<Key, Width, Depth>` when a stream holds too many distinct keys to count
 * each one exactly, but a small overestimate is acceptable: finding a babbling CAN node, counting
 * DTC or event occurrences, rate limiting by source address, picking hot flash pages, or cache
 * admission (TinyLFU). The sketch stores `Depth` rows of `Width` counters and no keys, so keys
 * cannot be enumerated.
 *
 * Each key owns one counter per row. `add()` raises those counters; `estimate()` returns the
 * smallest of them. Let `N = total()` be the sum of every added count, `f(x)` the true count of
 * `x`, `epsilon = e / Width`, and `delta = e^(-Depth)`. Then, while no counter saturates:
 * - `estimate(x) >= f(x)` always (no undercount), and
 * - `estimate(x) <= f(x) + epsilon * N` with probability at least `1 - delta`.
 *
 * `count_min_sketch_for<Key, InverseEpsilon, ConfidenceNines>` sizes the sketch from those two
 * targets. The `Depth` row columns come from one `Hash` call through enhanced double hashing
 * (`castle::detail::double_hash_probe`), the same scheme as `dsa::bloom_filter`.
 *
 * @code
 * // epsilon = 1 / 100 of the stream, 99 % confidence: 272 x 5 counters of 16 bits.
 * castle::dsa::count_min_sketch_for<uint32_t, 100U, 2U, uint16_t> frames;
 * frames.add(0x18DAF110U);
 * uint16_t seen = frames.estimate(0x18DAF110U); // 1, or more only through collisions
 * @endcode
 */
#ifndef CASTLE_DSA_COUNT_MIN_SKETCH_HPP
#define CASTLE_DSA_COUNT_MIN_SKETCH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/grid.hpp"
#include "castle/math/powi.hpp"
#include "castle/math/saturating.hpp"
#include "castle/utility/hash.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Counter update rule of a count-min sketch.
 */
enum class count_min_update : uint8_t
{
    standard = 0U, /**< Add the count to every row counter. Sketches stay exactly linear: merging equals one sketch fed with both streams. */
    conservative   /**< Raise row counters only up to `estimate + count`. Less overestimation for the same memory; merging still never undercounts. */
};

/**
 * @brief Returns the row width that reaches a target relative error.
 * @param inverse_epsilon `1 / epsilon`: the error bound is `N / inverse_epsilon` for a stream total `N`.
 * @return `ceil(e * inverse_epsilon)`, at least 1. Returns 1 when `inverse_epsilon == 0`.
 * @note Integer-only and `constexpr`, so it can feed the `Width` template argument. Uses `e`
 * rounded up, so the result is never smaller than the exact value.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR size_type count_min_width(size_type inverse_epsilon) CASTLE_NOEXCEPT
{
    // e ~= 2718281829 / 10^9 (rounded up); 64-bit products keep the result exact.
    CASTLE_CONST uint64_t scaled = static_cast<uint64_t>(inverse_epsilon) * 2718281829ULL;
    CASTLE_CONST size_type width = static_cast<size_type>((scaled + 999999999ULL) / 1000000000ULL);
    return (width == 0U) ? 1U : width;
}

/**
 * @brief Returns the row count that reaches a target confidence.
 * @param confidence_nines Number of nines `k` in the confidence `1 - 10^(-k)`: 2 means 99 %.
 * @return `ceil(k * ln 10) = ceil(ln(1 / delta))`, at least 1. Returns 1 when `confidence_nines == 0`.
 * @note Integer-only and `constexpr`, so it can feed the `Depth` template argument.
 */
CASTLE_NODISCARD CASTLE_CONSTEXPR size_type count_min_depth(size_type confidence_nines) CASTLE_NOEXCEPT
{
    // ln 10 ~= 2302585093 / 10^9 (rounded up).
    CASTLE_CONST uint64_t scaled = static_cast<uint64_t>(confidence_nines) * 2302585093ULL;
    CASTLE_CONST size_type depth = static_cast<size_type>((scaled + 999999999ULL) / 1000000000ULL);
    return (depth == 0U) ? 1U : depth;
}

/**
 * @brief Fixed-size count-min sketch over `Key` values.
 * @tparam Key Key type passed to `Hash`.
 * @tparam Width Counters per row. Sets the error: `epsilon = e / Width`. Must be positive.
 * @tparam Depth Number of rows, `1 .. Width`. Sets the confidence: `delta = e^(-Depth)`.
 * @tparam Counter Unsigned integral counter type. Counters saturate at its maximum.
 * @tparam Update Counter update rule, see `count_min_update`.
 * @tparam Hash Hash functor returning `size_type`. Its `operator()` must be `const`.
 * @note Storage is one `grid<Counter, Depth, Width>`, one `uint64_t` stream total, and one `Hash`
 * object. The sketch is copyable.
 * @note `add()` and `estimate()` are O(Depth); `merge()`, `halve()`, `load_counters()`,
 * `clear()`, and `operator==` are O(Width * Depth).
 * @warning Counts cannot be removed: a key shares its counters with other keys. Use `clear()` to
 * start over, or `halve()` to age old counts.
 */
template <typename Key,
          size_type Width,
          size_type Depth,
          typename Counter = uint32_t,
          count_min_update Update = count_min_update::standard,
          typename Hash = castle::hash<Key>>
class count_min_sketch
{
    static_assert(Width > 0U, "dsa::count_min_sketch requires a positive width");
    static_assert(Depth > 0U, "dsa::count_min_sketch requires at least one row");
    static_assert(Depth <= Width, "dsa::count_min_sketch depth must not exceed the width");
    static_assert(Width <= static_cast<castle::size_type>(-1) / 2U / Depth, "dsa::count_min_sketch is too large");
    static_assert(meta::is_integral<Counter>::value && meta::is_unsigned<Counter>::value &&
                      !meta::is_same<Counter, bool>::value,
                  "dsa::count_min_sketch requires an unsigned integral counter type");

public:
    using key_type = Key;
    using counter_type = Counter;
    using hasher = Hash;
    using size_type = castle::size_type;
    using grid_type = grid<Counter, Depth, Width>;

    /** @brief Counter update rule selected by the `Update` template argument. */
    static CASTLE_CONSTEXPR count_min_update update_rule = Update;

    /**
     * @brief Constructs an empty sketch with a default-constructed hash functor.
     * @note Complexity: O(Width * Depth).
     */
    count_min_sketch() CASTLE_NOEXCEPT
        : counters_(), total_(0U), hash_()
    {
    }

    /**
     * @brief Constructs an empty sketch with a caller-supplied hash functor.
     * @param hash Hash functor copied into the sketch, for example a seeded hasher.
     * @note Complexity: O(Width * Depth).
     */
    explicit count_min_sketch(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT
        : counters_(), total_(0U), hash_(hash)
    {
    }

    /** @brief Returns the number of counters per row. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type width() CASTLE_NOEXCEPT
    {
        return Width;
    }

    /** @brief Returns the number of rows. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type depth() CASTLE_NOEXCEPT
    {
        return Depth;
    }

    /** @brief Returns the number of counters, `Width * Depth`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type counter_count() CASTLE_NOEXCEPT
    {
        return Width * Depth;
    }

    /** @brief Returns the saturation value of one counter. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR counter_type max_count() CASTLE_NOEXCEPT
    {
        return castle::numeric_limits<Counter>::max();
    }

    /** @brief Returns the relative error `epsilon = e / Width`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR float epsilon() CASTLE_NOEXCEPT
    {
        return 2.7182818F / static_cast<float>(Width);
    }

    /** @brief Returns the failure probability `delta = e^(-Depth)` of the error bound. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR float delta() CASTLE_NOEXCEPT
    {
        return math::powi(0.36787944F, static_cast<unsigned>(Depth));
    }

    /**
     * @brief Adds `count` occurrences of a key.
     * @param key Key to count.
     * @param count Number of occurrences to add. Zero leaves the counters unchanged.
     * @return The new `estimate(key)`.
     * @note Counters and the total saturate instead of wrapping. Complexity: O(Depth).
     */
    counter_type add(CASTLE_CONST key_type& key, counter_type count = 1U) CASTLE_NOEXCEPT
    {
        total_ = math::saturating_add<uint64_t>(total_, static_cast<uint64_t>(count));

        CASTLE_IF_CONSTEXPR (Update == count_min_update::conservative)
        {
            // Raise each counter only as far as the new minimum needs: the estimate stays an
            // upper bound for every key sharing the counter.
            CASTLE_CONST counter_type target = math::saturating_add<counter_type>(estimate(key), count);
            probe column(hash_(key));
            for (size_type row = 0U; row < Depth; ++row)
            {
                counter_type& cell = counters_(row, column.index());
                if (cell < target)
                {
                    cell = target;
                }
                column.advance();
            }
            return target;
        }
        else
        {
            counter_type result = max_count();
            probe column(hash_(key));
            for (size_type row = 0U; row < Depth; ++row)
            {
                counter_type& cell = counters_(row, column.index());
                cell = math::saturating_add<counter_type>(cell, count);
                if (cell < result)
                {
                    result = cell;
                }
                column.advance();
            }
            return result;
        }
    }

    /**
     * @brief Estimates how often a key was added.
     * @param key Key to look up.
     * @return The smallest counter of `key`: never below the true count, unless that count
     * exceeds `max_count()`. Zero means `key` was certainly never added.
     * @note Complexity: O(Depth).
     */
    CASTLE_NODISCARD counter_type estimate(CASTLE_CONST key_type& key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        counter_type result = max_count();
        probe column(hash_(key));
        for (size_type row = 0U; row < Depth; ++row)
        {
            CASTLE_CONST counter_type cell = counters_(row, column.index());
            if (cell < result)
            {
                result = cell;
            }
            column.advance();
        }
        return result;
    }

    /**
     * @brief Adds every count of another sketch.
     * @param other Sketch of the same type built with an equivalent hash functor.
     * @note With `count_min_update::standard`, the result equals one sketch fed with both streams.
     * With `count_min_update::conservative`, estimates still never undercount. Counters saturate.
     * Complexity: O(Width * Depth).
     * @warning Sketches with different hash seeds produce a meaningless sum.
     */
    void merge(CASTLE_CONST count_min_sketch& other) CASTLE_NOEXCEPT
    {
        counter_type* dest = counters_.data();
        CASTLE_CONST counter_type* source = other.counters_.data();
        for (size_type i = 0U; i < counter_count(); ++i)
        {
            dest[i] = math::saturating_add<counter_type>(dest[i], source[i]);
        }
        total_ = math::saturating_add<uint64_t>(total_, other.total_);
    }

    /**
     * @brief Halves every counter and the total, rounding down.
     * @note Ages old counts so recent traffic dominates, for example once per time window.
     * Estimates stay at least `floor(f(x) / 2)` of the counts seen before the call.
     * Complexity: O(Width * Depth).
     */
    void halve() CASTLE_NOEXCEPT
    {
        counter_type* cells = counters_.data();
        for (size_type i = 0U; i < counter_count(); ++i)
        {
            cells[i] = static_cast<counter_type>(cells[i] >> 1U);
        }
        total_ >>= 1U;
    }

    /**
     * @brief Loads raw counters and the stream total, for example after a reset or a transfer.
     * @param counters Source array of `counter_count()` counters in `counters().data()` order.
     * @param count Number of counters in `counters`; must equal `counter_count()`.
     * @param total Stream total saved with the counters, see `total()`.
     * @return `status::ok`, `status::invalid_argument` when `counters == nullptr`, or
     * `status::out_of_range` when `count != counter_count()`. The sketch is unchanged on failure.
     * @note Complexity: O(Width * Depth).
     */
    castle::status load_counters(CASTLE_CONST counter_type* counters, size_type count, uint64_t total) CASTLE_NOEXCEPT
    {
        if (counters == nullptr)
        {
            return castle::status::invalid_argument;
        }
        if (count != counter_count())
        {
            return castle::status::out_of_range;
        }

        counter_type* dest = counters_.data();
        for (size_type i = 0U; i < count; ++i)
        {
            dest[i] = counters[i];
        }
        total_ = total;
        return castle::status::ok;
    }

    /** @brief Removes every count. Complexity: O(Width * Depth). */
    void clear() CASTLE_NOEXCEPT
    {
        counters_.fill(counter_type{0});
        total_ = 0U;
    }

    /** @brief Reports whether nothing was added since construction or `clear()`. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return total_ == 0U;
    }

    /** @brief Returns the sum `N` of every added count, saturated at `UINT64_MAX`. */
    CASTLE_NODISCARD uint64_t total() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return total_;
    }

    /**
     * @brief Returns the additive error bound `epsilon * total()`.
     * @note `estimate(x) <= f(x) + error_bound()` holds with probability at least `1 - delta()`.
     */
    CASTLE_NODISCARD float error_bound() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return epsilon() * static_cast<float>(total_);
    }

    /**
     * @brief Returns the counter storage.
     * @return Read-only `Depth x Width` grid; `counters().data()` exposes the counters for
     * persistence or transfer.
     */
    CASTLE_NODISCARD CASTLE_CONST grid_type& counters() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return counters_;
    }

    /** @brief Returns the stored hash functor. */
    CASTLE_NODISCARD CASTLE_CONST hasher& hash_function() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_;
    }

    /**
     * @brief Compares two sketches by their counters and totals.
     * @note The hash functors are not compared; check `hash_function()` when seeds may differ.
     */
    CASTLE_NODISCARD bool operator==(CASTLE_CONST count_min_sketch& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (total_ == other.total_) && (counters_ == other.counters_);
    }

    /** @brief Reports whether two sketches differ in counters or totals. */
    CASTLE_NODISCARD bool operator!=(CASTLE_CONST count_min_sketch& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    using probe = castle::detail::double_hash_probe<Width>;

    grid_type counters_;
    uint64_t total_;
    Hash hash_;
};

/**
 * @brief Count-min sketch sized from an error and a confidence target.
 * @tparam Key Key type passed to `Hash`.
 * @tparam InverseEpsilon `1 / epsilon`: estimates exceed true counts by at most `N / InverseEpsilon`.
 * @tparam ConfidenceNines `k` in the confidence `1 - 10^(-k)`. 2 gives 99 %.
 * @tparam Counter Unsigned integral counter type.
 * @tparam Update Counter update rule.
 * @tparam Hash Hash functor returning `size_type`.
 * @note Uses `Width = count_min_width(InverseEpsilon)` and `Depth = count_min_depth(ConfidenceNines)`.
 */
template <typename Key,
          size_type InverseEpsilon,
          size_type ConfidenceNines = 2U,
          typename Counter = uint32_t,
          count_min_update Update = count_min_update::standard,
          typename Hash = castle::hash<Key>>
using count_min_sketch_for = count_min_sketch<Key,
                                              count_min_width(InverseEpsilon),
                                              count_min_depth(ConfidenceNines),
                                              Counter,
                                              Update,
                                              Hash>;

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_COUNT_MIN_SKETCH_HPP
