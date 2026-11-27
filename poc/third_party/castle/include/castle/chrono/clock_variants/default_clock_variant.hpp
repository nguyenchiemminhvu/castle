// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file default_clock_variant.hpp
 * @brief Fallback clock-variant selection shim for Castle chrono clocks.
 *
 * Include this header only when examining the backend used by
 * @c castle/chrono/clocks.hpp for unknown compiler configurations. The current
 * fallback reuses the GCC-compatible implementation so that a POSIX
 * @c clock_gettime backend remains available without duplicating logic.
 *
 * Key constraints:
 * - Tick resolution and monotonicity are determined by the reused GCC backend.
 * - This header adds no extra wall-clock or steady-clock policy.
 * - Duration overflow behavior remains the same because no arithmetic changes occur here.
 *
 * Example:
 * @code
 * #include "castle/chrono/clock_variants/default_clock_variant.hpp"
 *
 * int main()
 * {
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CLOCK_VARIANTS_DEFAULT_CLOCK_VARIANT_HPP
#define CASTLE_CHRONO_CLOCK_VARIANTS_DEFAULT_CLOCK_VARIANT_HPP

#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"

#endif