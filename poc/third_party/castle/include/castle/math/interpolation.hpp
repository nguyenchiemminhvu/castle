// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file interpolation.hpp
 * @brief Umbrella include for Castle's interpolation facilities.
 * @details Aggregates the scalar kernels (`lerp`, bilinear, cubic Hermite,
 * Catmull-Rom, cubic Bezier) and the table-based interpolators
 * (`linear_table`, `cubic_spline`). All components are header-only,
 * allocation-free, exception-free and use fixed or caller-owned storage.
 *
 * @code
 * #include "castle/math/interpolation.hpp"
 *
 * const float xs[] = {0.0F, 1.0F, 2.0F};
 * const float ys[] = {0.0F, 2.0F, 8.0F};
 * const castle::math::linear_table<float> table(xs, ys, 3U);
 * const float y = table.evaluate(1.5F); // 5
 * @endcode
 */
#ifndef CASTLE_MATH_INTERPOLATION_HPP
#define CASTLE_MATH_INTERPOLATION_HPP

#include "castle/math/interpolation/lerp.hpp"
#include "castle/math/interpolation/knots.hpp"
#include "castle/math/interpolation/curves.hpp"
#include "castle/math/interpolation/linear_table.hpp"
#include "castle/math/interpolation/cubic_spline.hpp"

#endif // CASTLE_MATH_INTERPOLATION_HPP
