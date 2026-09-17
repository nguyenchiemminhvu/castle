// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file
 * @brief Aggregates the individual Castle linear algebra building blocks.
 *
 * Use this header when code inside or outside the linear algebra subtree wants
 * a single include that brings in the fixed-size vector, matrix,
 * trigonometric, quaternion, and transform facilities together. The component
 * headers remain allocation-free, exception-free, RTTI-free, virtual-free,
 * STL-free, and deterministic apart from the precision characteristics of the
 * underlying C math sine and cosine entry points. Matrices are stored in
 * row-major order and operate on column vectors.
 *
 * @code
 * #include "castle/math/linalg/linalg.hpp"
 *
 * const castle::math::matrix<float, 3U, 3U> transform =
 *     castle::math::rotation_2d_degrees(90.0F);
 * const castle::math::vector<float, 3U> point(1.0F, 0.0F, 1.0F);
 * const castle::math::vector<float, 3U> rotated = transform * point;
 * @endcode
 */
#ifndef CASTLE_MATH_LINALG_LINALG_HPP
#define CASTLE_MATH_LINALG_LINALG_HPP

#include "castle/math/linalg/vector.hpp"
#include "castle/math/linalg/matrix.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle/math/linalg/quaternion.hpp"
#include "castle/math/linalg/transform.hpp"

#endif
