#include <benchmark/benchmark.h>

#include <castle/array.hpp>

#if CASTLE_BENCHMARK_ENABLE_STL
#include <array>
#endif

#include <cstddef>
#include <cstdint>

namespace
{

constexpr std::size_t N = 128;

// ============================================================================
// CASTLE
// ============================================================================

static void BM_CastleArray_Construct(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::array<std::uint32_t, N> value;

        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK(BM_CastleArray_Construct);


// ============================================================================
// STL
// ============================================================================

#if CASTLE_BENCHMARK_ENABLE_STL

static void BM_STLArray_Construct(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        std::array<std::uint32_t, N> value;

        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK(BM_STLArray_Construct);

#endif


// ============================================================================
// Element access
// ============================================================================

static void BM_CastleArray_Access(
    benchmark::State& state)
{
    castle::array<std::uint32_t, N> value{};

    std::uint32_t result = 0;

    for (auto _ : state)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            result += value[i];
        }
    }

    benchmark::DoNotOptimize(result);

    state.SetItemsProcessed(
        static_cast<int64_t>(N) *
        state.iterations()
    );
}

BENCHMARK(BM_CastleArray_Access);


#if CASTLE_BENCHMARK_ENABLE_STL

static void BM_STLArray_Access(
    benchmark::State& state)
{
    std::array<std::uint32_t, N> value{};

    std::uint32_t result = 0;

    for (auto _ : state)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            result += value[i];
        }
    }

    benchmark::DoNotOptimize(result);

    state.SetItemsProcessed(
        static_cast<int64_t>(N) *
        state.iterations()
    );
}

BENCHMARK(BM_STLArray_Access);

#endif

} // namespace
