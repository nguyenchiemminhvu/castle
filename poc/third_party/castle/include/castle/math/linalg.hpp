// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file
 * @brief Umbrella include for Castle's fixed-size linear algebra module.
 *
 * Include this header when a translation unit needs Castle's fixed-size
 * vectors, matrices, trigonometric wrappers, quaternions, and homogeneous
 * transform helpers together. The aggregated APIs are header-only, keep all
 * storage inline, perform no dynamic allocation, throw no exceptions, and do
 * not depend on RTTI, virtual dispatch, or STL containers. Matrix storage is
 * row-major, matrix-vector multiplication treats vectors as columns, and
 * floating-point trigonometric precision comes directly from the platform C
 * math functions used by the trigonometry wrapper layer.
 *
 * @code
 * #include "castle/math/linalg.hpp"
 *
 * const castle::math::vector<float, 3U> axis(0.0F, 0.0F, 1.0F);
 * const castle::math::quaternion<float> rotation =
 *     castle::math::quaternion<float>::from_axis_angle_degrees(axis, 90.0F);
 * const castle::math::vector<float, 3U> result =
 *     rotation.rotate(castle::math::vector<float, 3U>(1.0F, 0.0F, 0.0F));
 * @endcode
 */
#ifndef CASTLE_MATH_LINALG_HPP
#define CASTLE_MATH_LINALG_HPP

#include "castle/math/linalg/linalg.hpp"

#endif
