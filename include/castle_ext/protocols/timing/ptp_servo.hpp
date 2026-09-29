// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ptp_servo.hpp
 * @brief Deterministic step/slew policy for PTP offset measurements.
 *
 * The servo is intentionally independent of an operating system or clock API.
 * It converts a signed local-minus-master offset and an interval between
 * corrections into a bounded action. A large phase error is stepped; a small
 * phase error is corrected by a proportional frequency offset in parts per
 * billion (ppb). The caller applies that action through a platform adapter.
 */
#ifndef CASTLE_EXT_PROTOCOLS_TIMING_PTP_SERVO_HPP
#define CASTLE_EXT_PROTOCOLS_TIMING_PTP_SERVO_HPP

#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace timing
{
namespace ptp
{

/** @brief Clock correction selected by the servo. */
enum class adjustment_mode : uint8_t
{
    none = 0U,
    step,
    slew
};

/**
 * @brief Bounded clock correction returned by a PTP servo.
 *
 * `offset_nanoseconds` is the amount to add to the local clock for a step.
 * `frequency_ppb` is the signed frequency correction for a slew. A positive
 * value makes the local clock run faster; a negative value makes it slower.
 */
struct clock_adjustment
{
    adjustment_mode mode = adjustment_mode::none;
    int64_t offset_nanoseconds = 0LL;
    int32_t frequency_ppb = 0;
};

/** @brief Configuration for the deterministic proportional PTP servo. */
struct servo_config
{
    /** Absolute offset at or above which the local clock is stepped. */
    int64_t step_threshold_nanoseconds = 100000000LL;
    /** Maximum absolute frequency correction in ppb. */
    int32_t max_slew_ppb = 500000;
    /** Proportional gain numerator. */
    uint32_t gain_numerator = 1U;
    /** Proportional gain denominator. */
    uint32_t gain_denominator = 8U;
};

/**
 * @brief Stateless, fixed-point PTP phase/frequency correction policy.
 *
 * The servo has no heap storage and no history. This makes behavior reproducible
 * across embedded targets and avoids requiring a scheduler or floating-point
 * support in the timing core.
 */
class proportional_servo CASTLE_FINAL
{
public:
    explicit proportional_servo(servo_config CASTLE_CONST& config = servo_config{}) CASTLE_NOEXCEPT
        : config_(config)
    {
    }

    /** @brief Replaces the policy configuration. */
    void configure(servo_config CASTLE_CONST& config) CASTLE_NOEXCEPT
    {
        config_ = config;
    }

    /** @return The active policy configuration. */
    CASTLE_NODISCARD servo_config CASTLE_CONST& config() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return config_;
    }

    /**
     * @brief Convert one PTP offset sample to a bounded step or slew action.
     * @param offset_nanoseconds Local clock minus master clock, in nanoseconds.
     * @param correction_interval_nanoseconds Time from the previous correction
     *        to this correction. It must be non-zero for slew mode.
     * @param[out] output Selected action.
     * @return `ok` on success, `invalid_argument` for an invalid policy/input.
     */
    CASTLE_NODISCARD castle::status update(
        int64_t offset_nanoseconds,
        uint64_t correction_interval_nanoseconds,
        clock_adjustment& output) CASTLE_CONST CASTLE_NOEXCEPT
    {
        output = clock_adjustment{};

        if ((config_.gain_denominator == 0U)
         || (config_.max_slew_ppb < 0)
         || (config_.step_threshold_nanoseconds < 0LL)
         || (correction_interval_nanoseconds == 0U))
        {
            return castle::status::invalid_argument;
        }

        CASTLE_CONST uint64_t magnitude = unsigned_magnitude(offset_nanoseconds);
        CASTLE_CONST uint64_t step_threshold = static_cast<uint64_t>(config_.step_threshold_nanoseconds);
        if (magnitude >= step_threshold && magnitude != 0U)
        {
            output.mode = adjustment_mode::step;
            output.offset_nanoseconds = negate_safely(offset_nanoseconds);
            return castle::status::ok;
        }

        if (offset_nanoseconds == 0LL || config_.max_slew_ppb == 0)
        {
            return castle::status::ok;
        }

        CASTLE_CONST int64_t ppb = proportional_ppb(
            offset_nanoseconds,
            correction_interval_nanoseconds,
            config_.gain_numerator,
            config_.gain_denominator,
            static_cast<uint32_t>(config_.max_slew_ppb));

        output.mode = ppb == 0LL ? adjustment_mode::none : adjustment_mode::slew; // LCOV_EXCL_BR_LINE
        output.frequency_ppb = static_cast<int32_t>(ppb);
        return castle::status::ok;
    }

private:
    static uint64_t unsigned_magnitude(int64_t value) CASTLE_NOEXCEPT
    {
        if (value >= 0LL)
        {
            return static_cast<uint64_t>(value);
        }
        return static_cast<uint64_t>(-(value + 1LL)) + 1ULL;
    }

    static int64_t negate_safely(int64_t value) CASTLE_NOEXCEPT
    {
        if (value == INT64_MIN)
        {
            return INT64_MAX;
        }
        return -value;
    }

    static int64_t proportional_ppb(
        int64_t offset_nanoseconds,
        uint64_t interval_nanoseconds,
        uint32_t gain_numerator,
        uint32_t gain_denominator,
        uint32_t max_slew_ppb) CASTLE_NOEXCEPT
    {
#if defined(__SIZEOF_INT128__)
        __extension__ typedef __int128 wide_signed;
        CASTLE_CONST wide_signed numerator = static_cast<wide_signed>(offset_nanoseconds)
            * static_cast<wide_signed>(1000000000ULL)
            * static_cast<wide_signed>(gain_numerator);
        CASTLE_CONST wide_signed denominator = static_cast<wide_signed>(interval_nanoseconds)
            * static_cast<wide_signed>(gain_denominator);
        wide_signed value = denominator == 0 ? 0 : (numerator / denominator); // LCOV_EXCL_BR_LINE
        if (value > static_cast<wide_signed>(max_slew_ppb))
        {
            value = static_cast<wide_signed>(max_slew_ppb);
        }
        // LCOV_EXCL_START
        if (value < -static_cast<wide_signed>(max_slew_ppb))
        {
            value = -static_cast<wide_signed>(max_slew_ppb);
        }
        // LCOV_EXCL_STOP
        return static_cast<int64_t>(-value);
#else
        CASTLE_CONST int64_t coarse = offset_nanoseconds
            / static_cast<int64_t>(interval_nanoseconds);
        if (coarse != 0LL)
        {
            return coarse > 0LL
                ? -static_cast<int64_t>(max_slew_ppb)
                : static_cast<int64_t>(max_slew_ppb);
        }
        return 0LL;
#endif
    }

    servo_config config_{};
};

/** @brief Apply an adjustment to a user-supplied platform clock adapter. */
template <typename Clock>
CASTLE_NODISCARD castle::status apply_clock_adjustment(
    Clock& clock,
    clock_adjustment CASTLE_CONST& adjustment) CASTLE_NOEXCEPT
{
    switch (adjustment.mode)
    {
        case adjustment_mode::none:
        {
            return castle::status::ok;
        }
        case adjustment_mode::step:
        {
            return clock.step(adjustment.offset_nanoseconds);
        }
        case adjustment_mode::slew:
        {
            return clock.slew(adjustment.frequency_ppb);
        }
        default:
        {
            return castle::status::invalid_argument;
        }
    }
}

} // namespace ptp
} // namespace timing
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_TIMING_PTP_SERVO_HPP
