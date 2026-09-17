#include <benchmark/benchmark.h>

#include <chrono>
#include <cstdint>

#include <castle/chrono/time_point.hpp>

namespace
{

struct test_clock {};
using castle_duration = castle::chrono::milliseconds;
using castle_time_point = castle::chrono::time_point<test_clock, castle_duration>;
using stl_time_point = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;

// ============================================================================
// time_point arithmetic
// ============================================================================

class time_point_add : public benchmark::Fixture {};

BENCHMARK_F(time_point_add, castle)(benchmark::State& state)
{
    castle_time_point value(castle_duration(100));
    castle_duration step(5);

    for (auto _ : state)
    {
        const auto result = value + step;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(time_point_add, stl)(benchmark::State& state)
{
    stl_time_point value(std::chrono::milliseconds(100));
    std::chrono::milliseconds step(5);

    for (auto _ : state)
    {
        const auto result = value + step;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// time_point comparison
// ============================================================================

class time_point_compare : public benchmark::Fixture {};

BENCHMARK_F(time_point_compare, castle)(benchmark::State& state)
{
    const castle_time_point lhs(castle_duration(100));
    const castle_time_point rhs(castle_duration(200));

    for (auto _ : state)
    {
        const bool result = lhs < rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(time_point_compare, stl)(benchmark::State& state)
{
    const stl_time_point lhs(std::chrono::milliseconds(100));
    const stl_time_point rhs(std::chrono::milliseconds(200));

    for (auto _ : state)
    {
        const bool result = lhs < rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// time_point duration difference
// ============================================================================

class time_point_difference : public benchmark::Fixture {};

BENCHMARK_F(time_point_difference, castle)(benchmark::State& state)
{
    const castle_time_point lhs(castle_duration(1000));
    const castle_time_point rhs(castle_duration(250));

    for (auto _ : state)
    {
        const auto result = lhs - rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(time_point_difference, stl)(benchmark::State& state)
{
    const stl_time_point lhs(std::chrono::milliseconds(1000));
    const stl_time_point rhs(std::chrono::milliseconds(250));

    for (auto _ : state)
    {
        const auto result = lhs - rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
