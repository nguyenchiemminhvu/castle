#include <benchmark/benchmark.h>

#include <cstdint>
#include <utility>

#include <castle/utility/pair.hpp>

namespace
{

// ============================================================================
// pair construction
// ============================================================================

class pair_construct : public benchmark::Fixture {};

BENCHMARK_F(pair_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto value = castle::make_pair(17, 31L);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(pair_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const auto value = std::make_pair(17, 31L);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// pair get
// ============================================================================

class pair_get : public benchmark::Fixture {};

BENCHMARK_F(pair_get, castle)(benchmark::State& state)
{
    const castle::pair<int, int> value(17, 31);

    for (auto _ : state)
    {
        const int result = castle::get<0U>(value) + castle::get<1U>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(pair_get, stl)(benchmark::State& state)
{
    const std::pair<int, int> value(17, 31);

    for (auto _ : state)
    {
        const int result = std::get<0U>(value) + std::get<1U>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// pair comparison
// ============================================================================

class pair_compare : public benchmark::Fixture {};

BENCHMARK_F(pair_compare, castle)(benchmark::State& state)
{
    const castle::pair<int, int> lhs(17, 31);
    const castle::pair<int, int> rhs(23, 11);

    for (auto _ : state)
    {
        const bool result = lhs < rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(pair_compare, stl)(benchmark::State& state)
{
    const std::pair<int, int> lhs(17, 31);
    const std::pair<int, int> rhs(23, 11);

    for (auto _ : state)
    {
        const bool result = lhs < rhs;
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
