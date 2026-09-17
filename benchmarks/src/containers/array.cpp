#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>

#include <castle/container/array.hpp>

#if CASTLE_BENCHMARK_ENABLE_STL
#include <array>
#endif

namespace
{

constexpr std::size_t N = 255;

// ============================================================================
// Array construct
// ============================================================================

class array_construct : public benchmark::Fixture {};

BENCHMARK_F(array_construct, castle) (benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::container::array<int, N> a;
        benchmark::DoNotOptimize(a);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL

BENCHMARK_F(array_construct, stl) (benchmark::State& state)
{
    for (auto _ : state)
    {
        std::array<int, N> a;
        benchmark::DoNotOptimize(a);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// Array access
// ============================================================================

class array_access : public benchmark::Fixture {};

BENCHMARK_F(array_access, castle) (benchmark::State& state)
{
    castle::container::array<int, N> a;
    std::uint64_t sum = 0U;
    for (auto _ : state)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            sum += static_cast<std::uint64_t>(a[i]);
        }

        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL

BENCHMARK_F(array_access, stl) (benchmark::State& state)
{
    std::array<int, N> a;
    std::uint64_t sum = 0U;
    for (auto _ : state)
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            sum += static_cast<std::uint64_t>(a[i]);
        }

        benchmark::DoNotOptimize(sum);
        benchmark::ClobberMemory();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
