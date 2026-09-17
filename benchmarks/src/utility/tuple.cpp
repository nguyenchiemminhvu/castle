#include <benchmark/benchmark.h>

#include <cstdint>
#include <tuple>

#include <castle/utility/tuple.hpp>

namespace
{

// ============================================================================
// tuple construction
// ============================================================================

class tuple_construct : public benchmark::Fixture {};

BENCHMARK_F(tuple_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const castle::tuple<int, float, std::uint32_t> value(17, 3.5F, 31U);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tuple_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const std::tuple<int, float, std::uint32_t> value(17, 3.5F, 31U);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tuple get
// ============================================================================

class tuple_get : public benchmark::Fixture {};

BENCHMARK_F(tuple_get, castle)(benchmark::State& state)
{
    const castle::tuple<int, float, std::uint32_t> value(17, 3.5F, 31U);

    for (auto _ : state)
    {
        const float result = castle::get<1U>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tuple_get, stl)(benchmark::State& state)
{
    const std::tuple<int, float, std::uint32_t> value(17, 3.5F, 31U);

    for (auto _ : state)
    {
        const float result = std::get<1U>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tuple assignment
// ============================================================================

class tuple_assignment : public benchmark::Fixture {};

BENCHMARK_F(tuple_assignment, castle)(benchmark::State& state)
{
    const castle::tuple<int, int, int> source(1, 2, 3);
    castle::tuple<int, int, int> value(4, 5, 6);

    for (auto _ : state)
    {
        value = source;
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(3U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tuple_assignment, stl)(benchmark::State& state)
{
    const std::tuple<int, int, int> source(1, 2, 3);
    std::tuple<int, int, int> value(4, 5, 6);

    for (auto _ : state)
    {
        value = source;
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(3U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
