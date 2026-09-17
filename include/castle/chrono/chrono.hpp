// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file chrono.hpp
 * @brief Umbrella include for Castle's deterministic chrono facilities.
 *
 * Include this header when a translation unit needs Castle durations, time points,
 * clocks, and user-defined literals together. It is a convenience wrapper only;
 * all semantics come from the included chrono headers.
 *
 * Key constraints:
 * - Tick resolution is defined by each duration period and by clock configuration macros.
 * - Monotonic behavior is provided only by @c steady_clock and depends on the selected backend.
 * - Duration and time-point arithmetic does not check overflow; callers must keep values in range.
 *
 * Example:
 * @code
 * #include "castle/chrono/chrono.hpp"
 *
 * using namespace castle::chrono::literals::chrono_literals;
 *
 * int main()
 * {
 *     castle::chrono::milliseconds timeout = 250_ms;
 *     auto start = castle::chrono::steady_clock::now();
 *     auto deadline = start + timeout;
 *     (void)deadline;
 *     return 0;
 * }
 * @endcode
 */

#ifndef CASTLE_CHRONO_CHRONO_HPP
#define CASTLE_CHRONO_CHRONO_HPP

#include "castle/chrono/duration.hpp"
#include "castle/chrono/time_point.hpp"
#include "castle/chrono/clocks.hpp"
#include "castle/chrono/literals.hpp"

#endif
