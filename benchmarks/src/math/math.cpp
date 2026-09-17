#include <benchmark/benchmark.h>

#include <cstdint>
#include <algorithm>
#include <cmath>

#include <castle/math/math.hpp>

namespace
{

volatile std::int32_t integral_inputs[] =
{
    -1000003, -12345, -777, -42, -7, -1, 0, 1, 7, 42, 777, 12345, 1000003
};

volatile double floating_inputs[] =
{
    0.125, 0.75, 1.5, 2.25, 3.5, 7.75, 15.5, 31.25
};

// ============================================================================
// abs
// ============================================================================

class math_abs : public benchmark::Fixture {};

BENCHMARK_F(math_abs, castle)(benchmark::State& state)
{
    const std::int32_t input = integral_inputs[0U];

    for (auto _ : state)
    {
        const auto result = castle::math::abs(input);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(math_abs, stl)(benchmark::State& state)
{
    const std::int32_t input = integral_inputs[0U];

    for (auto _ : state)
    {
        const auto result = std::abs(input);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// sqrt
// ============================================================================

class math_sqrt : public benchmark::Fixture {};

BENCHMARK_F(math_sqrt, castle)(benchmark::State& state)
{
    const double input = floating_inputs[0U];

    for (auto _ : state)
    {
        const auto result = castle::math::sqrt_real(input);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(math_sqrt, stl)(benchmark::State& state)
{
    const double input = floating_inputs[0U];

    for (auto _ : state)
    {
        const auto result = std::sqrt(input);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// clamp
// ============================================================================

class math_clamp : public benchmark::Fixture {};

BENCHMARK_F(math_clamp, castle)(benchmark::State& state)
{
    const std::int32_t input = integral_inputs[0U];

    for (auto _ : state)
    {
        const auto result = castle::math::clamp(input, -100, 100);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(math_clamp, stl)(benchmark::State& state)
{
    const std::int32_t input = integral_inputs[0U];

    for (auto _ : state)
    {
        const auto result = std::clamp(input, -100, 100);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// power-of-two test
// ============================================================================

class math_is_power_of_two : public benchmark::Fixture {};

BENCHMARK_F(math_is_power_of_two, castle)(benchmark::State& state)
{
    const std::uint32_t value = integral_inputs[1U];

    for (auto _ : state)
    {
        const bool result = castle::math::is_power_of_two(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(math_is_power_of_two, stl)(benchmark::State& state)
{
    const std::uint32_t value = integral_inputs[1U];

    for (auto _ : state)
    {
        const bool result = value != 0U && (value & (value - 1U)) == 0U;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// Fibonacci runtime calculation
// ============================================================================

class math_fib : public benchmark::Fixture {};

BENCHMARK_F(math_fib, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto result = castle::math::fib(static_cast<castle::size_type>(24U));
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(math_fib, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::uint32_t a = 0U;
        std::uint32_t b = 1U;
        for (std::uint32_t i = 2U; i <= 24U; ++i)
        {
            const std::uint32_t next = a + b;
            a = b;
            b = next;
        }
        benchmark::DoNotOptimize(b);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// mean over a data set
// ============================================================================

class math_mean : public benchmark::Fixture {};

BENCHMARK_F(math_mean, castle)(benchmark::State& state)
{
    const std::int32_t data[] = {3, 7, 11, 19, 23, 31, 43, 47};

    for (auto _ : state)
    {
        castle::math::mean<std::int32_t> value(data, data + 8U);
        benchmark::DoNotOptimize(value.get_mean());
    }

    state.SetItemsProcessed(static_cast<int64_t>(8U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(math_mean, stl)(benchmark::State& state)
{
    const std::int32_t data[] = {3, 7, 11, 19, 23, 31, 43, 47};

    for (auto _ : state)
    {
        std::int64_t sum = 0;
        for (const std::int32_t value : data)
        {
            sum += value;
        }
        const double result = static_cast<double>(sum) / 8.0;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(8U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
