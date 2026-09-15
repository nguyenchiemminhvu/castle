#include <benchmark/benchmark.h>

#include <castle/pair.hpp>

#if CASTLE_BENCHMARK_ENABLE_STL
#include <utility>
#endif

#include <cstdint>

namespace
{

static void BM_CastlePair_Access(
    benchmark::State& state)
{
    castle::pair<
        std::uint32_t,
        std::uint32_t
    > value{10u, 20u};

    std::uint32_t result = 0;

    for (auto _ : state)
    {
        result += value.first;
        result += value.second;
    }

    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_CastlePair_Access);


#if CASTLE_BENCHMARK_ENABLE_STL

static void BM_STLPair_Access(
    benchmark::State& state)
{
    std::pair<
        std::uint32_t,
        std::uint32_t
    > value{10u, 20u};

    std::uint32_t result = 0;

    for (auto _ : state)
    {
        result += value.first;
        result += value.second;
    }

    benchmark::DoNotOptimize(result);
}

BENCHMARK(BM_STLPair_Access);

#endif

} // namespace
