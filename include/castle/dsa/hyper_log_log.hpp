// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hyper_log_log.hpp
 * @brief Fixed-size HyperLogLog sketch: approximate count of distinct keys in constant memory.
 *
 * Use `hyper_log_log<Key, Precision>` when a stream may repeat keys and only the number of
 * distinct keys matters, but remembering every key does not fit in RAM: distinct GNSS positions
 * visited while a vehicle stands still, distinct CAN IDs or MAC addresses seen on a bus, distinct
 * clients per time window, or distinct error codes per drive cycle. The sketch stores
 * `2^Precision` one-byte registers and no keys, so keys cannot be listed or removed.
 *
 * Each key hash picks one register with its top `Precision` bits. The register keeps the largest
 * "rank" seen: one plus the number of leading zeros of the remaining bits. Many distinct keys make
 * long zero runs likely, so the registers encode the cardinality. `estimate()` uses Ertl's improved
 * estimator ("New cardinality estimation algorithms for HyperLogLog sketches", 2017, also used by
 * Redis): one formula for the whole range, with no bias tables and no switch to linear counting.
 *
 * The relative standard error is about `1.04 / sqrt(2^Precision)`: 3.25 % for the default
 * `Precision = 10` (1024 bytes). Duplicates never change the estimate. Two sketches merge
 * losslessly into the sketch of the union of both streams.
 *
 * @code
 * castle::dsa::hyper_log_log<uint64_t> positions; // 1024 registers, about 3 % error
 * positions.add(grid_cell_of(fix));                // repeated cells cost nothing
 * uint64_t distinct = positions.estimate();
 * @endcode
 */
#ifndef CASTLE_DSA_HYPER_LOG_LOG_HPP
#define CASTLE_DSA_HYPER_LOG_LOG_HPP

#include "castle/bit/bit_count.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/array.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle/utility/hash.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Fixed-size HyperLogLog sketch over `Key` values.
 * @tparam Key Key type passed to `Hash`.
 * @tparam Precision Number `p` of hash bits that select a register, `4 .. 16`. The sketch has
 * `2^p` registers of one byte each and a relative standard error of about `1.04 / sqrt(2^p)`.
 * @tparam Hash Hash functor returning `size_type`. Its `operator()` must be `const`.
 * @note Each hash is remixed to 64 bits, so ranks use `64 - p` bits on every target. With a 32-bit
 * `size_type`, at most `2^32` distinct hash values exist: estimates stay accurate up to about
 * `10^8` distinct keys.
 * @note Storage is one `dsa::array<uint8_t, 2^p>` and one `Hash` object. The sketch is copyable.
 * @note `add()` is O(1). `estimate()`, `merge()`, `load_registers()`, `clear()`, `empty()`, and
 * `operator==` are O(2^p).
 */
template <typename Key,
          size_type Precision = 10U,
          typename Hash = castle::hash<Key>>
class hyper_log_log
{
    static_assert(Precision >= 4U && Precision <= 16U, "dsa::hyper_log_log precision must be in 4 .. 16");

public:
    using key_type = Key;
    using hasher = Hash;
    using size_type = castle::size_type;
    using register_type = uint8_t;
    using register_array = array<register_type, (static_cast<size_type>(1U) << Precision)>;

    /** @brief Constructs an empty sketch with a default-constructed hash functor. */
    hyper_log_log() CASTLE_NOEXCEPT
        : registers_(), hash_()
    {
    }

    /**
     * @brief Constructs an empty sketch with a caller-supplied hash functor.
     * @param hash Hash functor copied into the sketch, for example a seeded hasher.
     */
    explicit hyper_log_log(CASTLE_CONST Hash& hash) CASTLE_NOEXCEPT
        : registers_(), hash_(hash)
    {
    }

    /** @brief Returns the number `p` of register-selecting hash bits. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type precision() CASTLE_NOEXCEPT
    {
        return Precision;
    }

    /** @brief Returns the number of registers, `2^p`. It is also the size in bytes. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type register_count() CASTLE_NOEXCEPT
    {
        return static_cast<size_type>(1U) << Precision;
    }

    /** @brief Returns the largest register value, `64 - p + 1`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR register_type max_rank() CASTLE_NOEXCEPT
    {
        return static_cast<register_type>(64U - Precision + 1U);
    }

    /** @brief Returns the relative standard error `1.04 / sqrt(2^p)` of `estimate()`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR float standard_error() CASTLE_NOEXCEPT
    {
        // sqrt(2^p) = 2^(p / 2), times sqrt(2) for odd p.
        return 1.04F / static_cast<float>(static_cast<uint32_t>(1U) << (Precision / 2U)) *
               (((Precision % 2U) != 0U) ? 0.70710678F : 1.0F);
    }

    /**
     * @brief Adds a key to the sketch.
     * @param key Key to add.
     * @return `true` when a register grew, so the estimate may have changed. `false` when the
     * key, or another key with an equal or larger rank in the same register, was seen before.
     * @note Complexity: O(1).
     */
    bool add(CASTLE_CONST key_type& key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST uint64_t mixed = castle::detail::hash_mix64(static_cast<uint64_t>(hash_(key)));
        CASTLE_CONST size_type index = static_cast<size_type>(mixed >> (64U - Precision));
        // The sentinel bit caps the leading-zero count of the remaining 64 - p bits at 64 - p.
        CASTLE_CONST uint64_t rest = (mixed << Precision) | (static_cast<uint64_t>(1U) << (Precision - 1U));
        CASTLE_CONST register_type rank = static_cast<register_type>(bit::count_leading_zeros(rest) + 1U);
        if (rank > registers_[index])
        {
            registers_[index] = rank;
            return true;
        }
        return false;
    }

    /**
     * @brief Estimates the number of distinct keys added.
     * @return Rounded cardinality estimate. Zero when nothing was added. Saturates at `UINT64_MAX`.
     * @note Ertl's estimator: `alpha * m^2 / (m * sigma(C0 / m) + sum + m * tau(1 - Cmax / m) / 2^(64 - p))`,
     * where `C0` and `Cmax` count zero and saturated registers and `sum` adds `2^-r` over the
     * other registers. Uses `double`. Complexity: O(2^p).
     */
    CASTLE_NODISCARD uint64_t estimate() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type zeros = 0U;
        size_type saturated = 0U;
        double sum = 0.0;
        for (size_type i = 0U; i < register_count(); ++i)
        {
            CASTLE_CONST register_type rank = registers_[i];
            if (rank == 0U)
            {
                ++zeros;
            }
            else if (rank == max_rank())
            {
                ++saturated;
            }
            else
            {
                sum += 1.0 / static_cast<double>(static_cast<uint64_t>(1U) << rank);
            }
        }
        if (zeros == register_count())
        {
            return 0U;
        }

        CASTLE_CONST double m = static_cast<double>(register_count());
        CASTLE_CONST double tail = m * tau(1.0 - static_cast<double>(saturated) / m) /
                                   static_cast<double>(static_cast<uint64_t>(1U) << (64U - Precision));
        CASTLE_CONST double denominator = m * sigma(static_cast<double>(zeros) / m) + sum + tail;
        CASTLE_CONST double cardinality = alpha_infinity * m * m / denominator;
        // 2^64: also catches an infinite estimate when every register is saturated.
        if (!(cardinality < 18446744073709551616.0))
        {
            return castle::numeric_limits<uint64_t>::max();
        }
        return static_cast<uint64_t>(cardinality + 0.5);
    }

    /**
     * @brief Adds every key of another sketch (set union).
     * @param other Sketch of the same type built with an equivalent hash functor.
     * @note Keeps the larger value of each register pair. The result equals one sketch fed with
     * both streams. Complexity: O(2^p).
     * @warning Sketches with different hash seeds produce a meaningless union.
     */
    void merge(CASTLE_CONST hyper_log_log& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < register_count(); ++i)
        {
            if (other.registers_[i] > registers_[i])
            {
                registers_[i] = other.registers_[i];
            }
        }
    }

    /**
     * @brief Loads raw registers, for example after a reset or a transfer.
     * @param registers Source array of `register_count()` registers in `registers().data()` order.
     * @param count Number of registers in `registers`; must equal `register_count()`.
     * @return `status::ok`, `status::invalid_argument` when `registers == nullptr` or a register
     * exceeds `max_rank()`, or `status::out_of_range` when `count != register_count()`. The sketch
     * is unchanged on failure.
     * @note Complexity: O(2^p).
     */
    castle::status load_registers(CASTLE_CONST register_type* registers, size_type count) CASTLE_NOEXCEPT
    {
        if (registers == nullptr)
        {
            return castle::status::invalid_argument;
        }
        if (count != register_count())
        {
            return castle::status::out_of_range;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            if (registers[i] > max_rank())
            {
                return castle::status::invalid_argument;
            }
        }
        for (size_type i = 0U; i < count; ++i)
        {
            registers_[i] = registers[i];
        }
        return castle::status::ok;
    }

    /** @brief Forgets every key by zeroing all registers. Complexity: O(2^p). */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < register_count(); ++i)
        {
            registers_[i] = 0U;
        }
    }

    /** @brief Reports whether no key was added since construction or `clear()`. Complexity: O(2^p). */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < register_count(); ++i)
        {
            if (registers_[i] != 0U)
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Returns the registers.
     * @return Read-only register array; `registers().data()` exposes them for persistence or transfer.
     */
    CASTLE_NODISCARD CASTLE_CONST register_array& registers() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return registers_;
    }

    /** @brief Returns the stored hash functor. */
    CASTLE_NODISCARD CASTLE_CONST hasher& hash_function() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return hash_;
    }

    /**
     * @brief Compares two sketches register by register.
     * @note The hash functors are not compared; check `hash_function()` when seeds may differ.
     */
    CASTLE_NODISCARD bool operator==(CASTLE_CONST hyper_log_log& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < register_count(); ++i)
        {
            if (registers_[i] != other.registers_[i])
            {
                return false;
            }
        }
        return true;
    }

    /** @brief Reports whether two sketches differ in any register. */
    CASTLE_NODISCARD bool operator!=(CASTLE_CONST hyper_log_log& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    /** @brief `1 / (2 ln 2)`, the bias constant of the estimator for `m -> infinity`. */
    static CASTLE_CONSTEXPR double alpha_infinity = 0.72134752044448170368;

    /**
     * @brief Ertl's `sigma(x) = x + sum_{k >= 1} x^(2^k) 2^(k - 1)` for `x` in `[0, 1)`.
     * @note Stops when the sum no longer changes in `double`; `x^(2^k)` decays doubly exponentially.
     */
    static double sigma(double x) CASTLE_NOEXCEPT
    {
        double power = x;
        double weight = 1.0;
        double sum = x;
        double previous = 0.0;
        do
        {
            power *= power;
            previous = sum;
            sum += power * weight;
            weight += weight;
        } while (sum != previous);
        return sum;
    }

    /**
     * @brief Ertl's `tau(x) = (1 - x - sum_{k >= 1} (1 - x^(2^-k))^2 2^-k) / 3` for `x` in `[0, 1]`.
     * @note `tau(1) = 0`, so a sketch without saturated registers ends after one step.
     */
    static double tau(double x) CASTLE_NOEXCEPT
    {
        double root = x;
        double weight = 1.0;
        double sum = 1.0 - x;
        double previous = 0.0;
        do
        {
            root = math::sqrt_real(root);
            previous = sum;
            weight *= 0.5;
            sum -= (1.0 - root) * (1.0 - root) * weight;
        } while (sum != previous);
        return sum / 3.0;
    }

    register_array registers_;
    Hash hash_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_HYPER_LOG_LOG_HPP
