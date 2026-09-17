#ifndef CASTLE_MATH_RANDOM_H
#define CASTLE_MATH_RANDOM_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

#include <stdint.h>

namespace castle
{
namespace math
{

// ============================================================================
// Deterministic, heap-free pseudo-random number generator for embedded use.
//
// The default engine is xoshiro128**. It has a 128-bit internal state, uses
// only uint32_t arithmetic, has no hidden/global state, and does not depend on
// libc rand()/srand(). The sequence is completely defined by the supplied seed.
//
// range() uses rejection sampling with a multiply-high mapping to avoid the
// modulo bias present in a simple random_value % range implementation.
//
// This is a statistical PRNG, not a cryptographic random source.
// ============================================================================
class random
{
public:
    using result_type = uint32_t;

    // Default construction is deterministic by design. Hardware entropy or
    // timing data should be mixed in by the application when non-deterministic
    // behavior is actually required.
    random() CASTLE_NOEXCEPT
    {
        seed(default_seed());
    }

    explicit random(result_type seed_value) CASTLE_NOEXCEPT
    {
        seed(seed_value);
    }

    // Reinitialises the generator from a single deterministic seed.
    void seed(result_type seed_value) CASTLE_NOEXCEPT
    {
        result_type state = seed_value;
        state_[0] = splitmix32(state);
        state_[1] = splitmix32(state);
        state_[2] = splitmix32(state);
        state_[3] = splitmix32(state);

        // xoshiro128** forbids the all-zero state. The mixer above makes this
        // practically unreachable, but keep the invariant explicit.
        if ((state_[0] | state_[1] | state_[2] | state_[3]) == 0U)
        {
            state_[0] = 1U;
        }
    }

    // Returns the next 32-bit pseudo-random value.
    CASTLE_NODISCARD result_type operator()() CASTLE_NOEXCEPT
    {
        return next();
    }

    // Returns the next 32-bit pseudo-random value.
    CASTLE_NODISCARD result_type next() CASTLE_NOEXCEPT
    {
        // xoshiro128** 1.1 reference output transformation.
        const result_type result = rotate_left(state_[1] * 5U, 7U) * 9U;
        const result_type t = state_[1] << 9U;

        state_[2] ^= state_[0];
        state_[3] ^= state_[1];
        state_[1] ^= state_[2];
        state_[0] ^= state_[3];
        state_[2] ^= t;
        state_[3] = rotate_left(state_[3], 11U);

        return result;
    }

    // Returns a uniform value in [0, bound).
    // Precondition: bound != 0.
    CASTLE_NODISCARD result_type uniform(result_type bound) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(
            bound != 0U,
            CASTLE_ERROR_GENERIC("castle::random::uniform: bound == 0")
        );

        if (bound == 0U)
        {
            return 0U;
        }

        // Lemire's multiply-high reduction. Only the rejection threshold uses
        // division; the normal accepted path uses a 32x32 -> 64 multiplication.
        const result_type threshold = static_cast<result_type>(-bound) % bound;

        for (;;)
        {
            const result_type value = next();
            const uint64_t product = static_cast<uint64_t>(value) *
                                     static_cast<uint64_t>(bound);
            const result_type low = static_cast<result_type>(product);

            if (low >= threshold)
            {
                return static_cast<result_type>(product >> 32U);
            }
        }
    }

    // Returns a uniform value in the inclusive interval [low, high].
    // Precondition: low <= high.
    CASTLE_NODISCARD result_type range(result_type low, result_type high) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(
            low <= high,
            CASTLE_ERROR_GENERIC("castle::random::range: low > high")
        );

        const uint64_t span = static_cast<uint64_t>(high) -
                              static_cast<uint64_t>(low) + 1ULL;
        if (span == static_cast<uint64_t>(max()) + 1ULL)
        {
            // The interval is the complete uint32_t domain.
            return next();
        }

        return low + uniform(static_cast<result_type>(span));
    }

    // Advances the generator by count outputs.
    void discard(uint32_t count) CASTLE_NOEXCEPT
    {
        while (count != 0U)
        {
            static_cast<void>(next());
            --count;
        }
    }

    static CASTLE_CONSTEXPR result_type min() CASTLE_NOEXCEPT
    {
        return static_cast<result_type>(0U);
    }

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

    // SplitMix32-style seed expansion. It turns one seed into four well-mixed
    // state words without requiring 64-bit state or heap-backed seed storage.
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

#endif // CASTLE_MATH_RANDOM_H
