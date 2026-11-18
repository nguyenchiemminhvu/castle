// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file statistic.hpp
 * @brief Umbrella header for the `castle::statistic` toolkit.
 *
 * Includes every statistics building block. Include the individual headers
 * instead to keep compile-time dependencies minimal; only `kernel_density.hpp`
 * (Gaussian kernel, bandwidth rule) depends on `<math.h>`.
 *
 * @code
 * #include "castle_ext/statistic/statistic.hpp"
 *
 * castle::statistic::exact_cep<double, 100U> horizontal;
 * castle::statistic::running_moments<double> moments;
 * horizontal.push_distance(1.2);
 * moments.push(1.2);
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_STATISTIC_HPP
#define CASTLE_EXT_STATISTIC_STATISTIC_HPP

#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/sample_window.hpp"
#include "castle_ext/statistic/running_moments.hpp"
#include "castle_ext/statistic/sliding_moments.hpp"
#include "castle_ext/statistic/order_statistics.hpp"
#include "castle_ext/statistic/sorted_window.hpp"
#include "castle_ext/statistic/histogram.hpp"
#include "castle_ext/statistic/sliding_histogram.hpp"
#include "castle_ext/statistic/p2_quantile.hpp"
#include "castle_ext/statistic/cep.hpp"
#include "castle_ext/statistic/ewma.hpp"
#include "castle_ext/statistic/online_regression.hpp"
#include "castle_ext/statistic/kernel_density.hpp"

#endif // CASTLE_EXT_STATISTIC_STATISTIC_HPP
