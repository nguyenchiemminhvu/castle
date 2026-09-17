// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file arm_clock_variant.hpp
 * @brief ARM compiler clock-variant selection shim for Castle chrono clocks.
 *
 * Include this header only when documenting or testing the backend selected by
 * @c castle/chrono/clocks.hpp for ARM compiler builds. The current ARM path reuses
 * the GCC-compatible clock implementation because Castle's ARM compiler detection
 * layers on top of the GCC variant macros.
 *
 * Key constraints:
 * - Tick resolution and monotonicity are defined by the reused GCC clock backend.
 * - No timing policy is introduced here; this header is an include-only selector.
 * - Any overflow considerations remain those of the underlying duration arithmetic.
 *
 * Example:
 * @code
 * #include "castle/chrono/clock_variants/arm_clock_variant.hpp"
 *
 * int main()
 * {
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CLOCK_VARIANTS_ARM_CLOCK_VARIANT_HPP
#define CASTLE_CHRONO_CLOCK_VARIANTS_ARM_CLOCK_VARIANT_HPP

#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"

#endif