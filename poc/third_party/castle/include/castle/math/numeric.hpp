// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file numeric.hpp
 * @brief Umbrella include for Castle's numerical-methods facilities.
 * @details Aggregates numerical integration, differentiation, root finding
 * and ODE stepping. Every routine is header-only, allocation-free,
 * exception-free and non-recursive, takes callables as templates (no
 * `std::function`) and reports iterative outcomes through
 * `castle::status` / `numeric_result`.
 *
 * @code
 * #include "castle/math/numeric.hpp"
 *
 * auto f = [](double x) { return x * x; };
 * const double area = castle::math::integrate_simpson(f, 0.0, 3.0, 100U);   // 9
 * const double slope = castle::math::derivative_central(f, 3.0, 1e-4);      // 6
 * @endcode
 */
#ifndef CASTLE_MATH_NUMERIC_HPP
#define CASTLE_MATH_NUMERIC_HPP

#include "castle/math/numeric/result.hpp"
#include "castle/math/numeric/integration.hpp"
#include "castle/math/numeric/differentiation.hpp"
#include "castle/math/numeric/root_finding.hpp"
#include "castle/math/numeric/ode.hpp"

#endif // CASTLE_MATH_NUMERIC_HPP
