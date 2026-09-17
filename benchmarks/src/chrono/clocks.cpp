#include <benchmark/benchmark.h>

#include <chrono>
#include <cstdint>

#include <castle/chrono/clocks.hpp>

namespace
{

// ============================================================================
// steady_clock::now
// ============================================================================

class steady_clock_now : public benchmark::Fixture {};

BENCHMARK_F(steady_clock_now, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto result = castle::chrono::steady_clock::now();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(steady_clock_now, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto result = std::chrono::steady_clock::now();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// system_clock::now
// ============================================================================

class system_clock_now : public benchmark::Fixture {};

BENCHMARK_F(system_clock_now, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto result = castle::chrono::system_clock::now();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(system_clock_now, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto result = std::chrono::system_clock::now();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
