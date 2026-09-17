// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file clang_clock_variant.hpp
 * @brief Clang compiler clock-variant selection shim for Castle chrono clocks.
 *
 * Include this header only when inspecting the backend chosen by
 * @c castle/chrono/clocks.hpp for Clang builds. Clang forwards to the GCC-compatible
 * implementation, so this header contributes selection logic without changing
 * clock semantics.
 *
 * Key constraints:
 * - Tick resolution and monotonicity are inherited from the GCC clock backend.
 * - This header does not provide a separate timer source or conversion policy.
 * - Duration and time-point overflow rules are unchanged because no new arithmetic is added.
 *
 * Example:
 * @code
 * #include "castle/chrono/clock_variants/clang_clock_variant.hpp"
 *
 * int main()
 * {
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CLOCK_VARIANTS_CLANG_CLOCK_VARIANT_HPP
#define CASTLE_CHRONO_CLOCK_VARIANTS_CLANG_CLOCK_VARIANT_HPP

#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"

#endif
