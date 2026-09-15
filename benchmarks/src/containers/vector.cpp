#include <benchmark/benchmark.h>

#include <castle/vector.hpp>

#if CASTLE_BENCHMARK_ENABLE_STL
#include <vector>
#endif

#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::size_t N = 64;

// ============================================================================
// push_back
// ============================================================================

static void BM_CastleVector_PushBack(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::vector<std::uint32_t, N> value;

        for (std::size_t i = 0; i < N; ++i)
        {
            value.push_back(
                static_cast<std::uint32_t>(i)
            );
        }

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(N) *
        state.iterations()
    );
}

BENCHMARK(BM_CastleVector_PushBack);


#if CASTLE_BENCHMARK_ENABLE_STL

static void BM_STLVector_PushBack(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        std::vector<std::uint32_t> value;

        value.reserve(N);

        for (std::size_t i = 0; i < N; ++i)
        {
            value.push_back(
                static_cast<std::uint32_t>(i)
            );
        }

        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(
        static_cast<int64_t>(N) *
        state.iterations()
    );
}

BENCHMARK(BM_STLVector_PushBack);

#endif

} // namespace
