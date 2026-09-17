// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file random.hpp
 * @brief Deterministic pseudo-random number generator for embedded use.
 *
 * This header implements a heap-free `uint32_t` PRNG based on xoshiro128**
 * with SplitMix32-style seed expansion. The complete output sequence is
 * determined by the seed stored in each object, which makes the generator
 * reproducible, testable, and free of hidden global state.
 *
 * Example:
 * @code
 * castle::math::random a(1234U);
 * castle::math::random b(1234U);
 * const uint32_t first_a = a.next();
 * const uint32_t first_b = b.next();
 * const uint32_t bounded = a.range(10U, 20U);
 * (void)first_a;
 * (void)first_b;
 * (void)bounded;
 * @endcode
 *
 * @note `uniform()` uses Lemire-style multiply-high rejection sampling to
 *       avoid modulo bias for bounded results.
 * @warning This generator is deterministic and statistical, not cryptographic.
 */
#ifndef CASTLE_MATH_RANDOM_HPP
#define CASTLE_MATH_RANDOM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{
namespace math
{

/**
 * @brief Deterministic xoshiro128** pseudo-random number generator.
 * @note State is stored entirely inside the object as four 32-bit words.
 * @warning The generated sequence is reproducible and must not be used for
 *          cryptographic secrets.
 */
class random
{
public:
    using result_type = uint32_t;

    /**
     * @brief Constructs the generator with the library default seed.
     * @note The default seed is deterministic so default-constructed engines
     *       produce the same sequence on every run.
     */
    random() CASTLE_NOEXCEPT
    {
        seed(default_seed());
    }

    /**
     * @brief Constructs the generator from an explicit seed.
     * @param seed_value Deterministic seed value.
     * @note Identical seeds always produce identical output sequences.
     */
    explicit random(result_type seed_value) CASTLE_NOEXCEPT
    {
        seed(seed_value);
    }

    /**
     * @brief Reinitializes the generator from a single seed value.
     * @param seed_value Deterministic seed value.
     * @note The seed is expanded into the 128-bit xoshiro state using the
     *       private SplitMix32-style mixer.
     */
    void seed(result_type seed_value) CASTLE_NOEXCEPT
    {
        result_type state = seed_value;
        state_[0] = splitmix32(state);
        state_[1] = splitmix32(state);
        state_[2] = splitmix32(state);
        state_[3] = splitmix32(state);

        // LCOV_EXCL_BR_START
        // Keep the xoshiro state valid even if seed expansion ever produced all zeros.
        if ((state_[0] | state_[1] | state_[2] | state_[3]) == 0U)
        {
            state_[0] = 1U;
        }
        // LCOV_EXCL_BR_END
    }

    /**
     * @brief Generates the next 32-bit pseudo-random value.
     * @return Next generator output.
     * @note Equivalent to `next()`.
     */
    CASTLE_NODISCARD result_type operator()() CASTLE_NOEXCEPT
    {
        return next();
    }

    /**
     * @brief Generates the next 32-bit pseudo-random value.
     * @return Next generator output in the closed interval `[min(), max()]`.
     * @note Uses the xoshiro128** 1.1 output transform and state transition.
     */
    CASTLE_NODISCARD result_type next() CASTLE_NOEXCEPT
    {
        CASTLE_CONST result_type result = rotate_left(state_[1] * 5U, 7U) * 9U;
        CASTLE_CONST result_type t = state_[1] << 9U;

        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = rotate_left(state_[3], 11U);

        return result;
    }

    /**
     * @brief Generates a uniform value in `[0, bound)`.
     * @param bound Exclusive upper bound.
     * @return Uniformly distributed value less than `bound`.
     * @note The bounded mapping is deterministic and avoids modulo bias.
     * @warning `bound` must not be zero.
     */
    CASTLE_NODISCARD result_type uniform(result_type bound) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(bound != 0U, CASTLE_ERROR_GENERIC("castle::random::uniform: bound == 0")); // LCOV_EXCL_BR_LINE

        // LCOV_EXCL_BR_START
        if (bound == 0U)
        {
            return 0U;
        }
        // LCOV_EXCL_BR_END

        // Multiply-high reduction rejects only the low tail that would introduce bias.
        CASTLE_CONST result_type threshold = static_cast<result_type>(-bound) % bound;

        for (;;)
        {
            CASTLE_CONST result_type value = next();
            CASTLE_CONST uint64_t product = static_cast<uint64_t>(value) *
                                     static_cast<uint64_t>(bound);
            CASTLE_CONST result_type low = static_cast<result_type>(product);

            // LCOV_EXCL_BR_START
            if (low >= threshold)
            {
                return static_cast<result_type>(product >> 32U);
            }
            // LCOV_EXCL_BR_END
        } // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Generates a uniform value in the inclusive interval `[low, high]`.
     * @param low Inclusive lower bound.
     * @param high Inclusive upper bound.
     * @return Uniformly distributed value between `low` and `high`.
     * @note When the requested interval spans the full `uint32_t` domain, this
     *       function returns `next()` directly.
     * @warning `low` must be less than or equal to `high`.
     */
    CASTLE_NODISCARD result_type range(result_type low, result_type high) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(low <= high, CASTLE_ERROR_GENERIC("castle::random::range: low > hi.hpp")); // LCOV_EXCL_BR_LINE

        CASTLE_CONST uint64_t span = static_cast<uint64_t>(high) -
                              static_cast<uint64_t>(low) + 1ULL;
        if (span == static_cast<uint64_t>(max()) + 1ULL)
        {
            return next();
        }

        return low + uniform(static_cast<result_type>(span));
    }

    /**
     * @brief Advances the generator by discarding outputs.
     * @param count Number of generated values to skip.
     * @note Complexity is linear in `count`.
     */
    void discard(uint32_t count) CASTLE_NOEXCEPT
    {
        while (count != 0U)
        {
            static_cast<void>(next());
            --count;
        }
    }

    /**
     * @brief Returns the minimum value this generator can produce.
     * @return `0`.
     */
    static CASTLE_CONSTEXPR result_type min() CASTLE_NOEXCEPT
    {
        return static_cast<result_type>(0U);
    }

    /**
     * @brief Returns the maximum value this generator can produce.
     * @return All bits set in `result_type`.
     */
    static CASTLE_CONSTEXPR result_type max() CASTLE_NOEXCEPT
    {
        return static_cast<result_type>(~static_cast<result_type>(0U));
    }

private:
    static CASTLE_CONSTEXPR result_type default_seed() CASTLE_NOEXCEPT
    {
        return UINT32_C(0x6D2B79F5);
    }

    static result_type rotate_left(result_type value, result_type shift) CASTLE_NOEXCEPT
    {
        return static_cast<result_type>(
            (value << shift) | (value >> (32U - shift))
        );
    }

    static result_type splitmix32(result_type& state) CASTLE_NOEXCEPT
    {
        state += UINT32_C(0x9E3779B9);

        result_type value = state;
        value ^= value >> 16U;
        value *= UINT32_C(0x85EBCA6B);
        value ^= value >> 13U;
        value *= UINT32_C(0xC2B2AE35);
        value ^= value >> 16U;

        return value;
    }

    result_type state_[4];
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_RANDOM_HPP
