#include <benchmark/benchmark.h>

#include <chrono>
#include <cstdint>

#include <castle/chrono/duration.hpp>

namespace
{

// ============================================================================
// duration addition
// ============================================================================

class duration_add : public benchmark::Fixture {};

BENCHMARK_F(duration_add, castle)(benchmark::State& state)
{
    castle::chrono::milliseconds lhs(12);
    castle::chrono::milliseconds rhs(30);

    for (auto _ : state)
    {
        const auto result = lhs + rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(duration_add, stl)(benchmark::State& state)
{
    std::chrono::milliseconds lhs(12);
    std::chrono::milliseconds rhs(30);

    for (auto _ : state)
    {
        const auto result = lhs + rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// duration cast
// ============================================================================

class duration_cast : public benchmark::Fixture {};

BENCHMARK_F(duration_cast, castle)(benchmark::State& state)
{
    castle::chrono::milliseconds value(12345);

    for (auto _ : state)
    {
        const auto result =
            castle::chrono::duration_cast<castle::chrono::microseconds>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(duration_cast, stl)(benchmark::State& state)
{
    std::chrono::milliseconds value(12345);

    for (auto _ : state)
    {
        const auto result =
            std::chrono::duration_cast<std::chrono::microseconds>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// duration compound update
// ============================================================================

class duration_update : public benchmark::Fixture {};

BENCHMARK_F(duration_update, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle::chrono::milliseconds value(10);
    state.ResumeTiming();

    for (auto _ : state)
    {
        value += castle::chrono::milliseconds(3);
        value *= 2;

        benchmark::DoNotOptimize(value);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(duration_update, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::chrono::milliseconds value(10);
    state.ResumeTiming();

    for (auto _ : state)
    {
        value += std::chrono::milliseconds(3);
        value *= 2;

        benchmark::DoNotOptimize(value);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
