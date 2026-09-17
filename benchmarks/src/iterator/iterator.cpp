#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include <castle/iterator/iterator.hpp>

namespace
{

constexpr std::size_t N = 255;

// ============================================================================
// distance
// ============================================================================

class iterator_distance : public benchmark::Fixture {};

BENCHMARK_F(iterator_distance, castle)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    state.ResumeTiming();

    for (auto _ : state)
    {
        const auto result = castle::distance(data.data(), data.data() + N);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(iterator_distance, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    state.ResumeTiming();

    for (auto _ : state)
    {
        const auto result = std::distance(data.data(), data.data() + N);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// advance
// ============================================================================

class iterator_advance : public benchmark::Fixture {};

BENCHMARK_F(iterator_advance, castle)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    state.ResumeTiming();

    int* iterator = data.data();

    for (auto _ : state)
    {
        castle::advance(iterator, static_cast<castle::difference_type>(N / 2U));
        benchmark::DoNotOptimize(iterator);

        state.PauseTiming();
        iterator = data.data();
        state.ResumeTiming();
    }

    state.PauseTiming();

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(iterator_advance, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    state.ResumeTiming();

    int* iterator = data.data();

    for (auto _ : state)
    {
        std::advance(iterator, static_cast<std::ptrdiff_t>(N / 2U));
        benchmark::DoNotOptimize(iterator);

        state.PauseTiming();
        iterator = data.data();
        state.ResumeTiming();
    }

    state.PauseTiming();

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// next
// ============================================================================

class iterator_next : public benchmark::Fixture {};

BENCHMARK_F(iterator_next, castle)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    state.ResumeTiming();

    for (auto _ : state)
    {
        int* iterator = castle::next(data.data(), static_cast<castle::difference_type>(N / 2U));
        benchmark::DoNotOptimize(iterator);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(iterator_next, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    state.ResumeTiming();

    for (auto _ : state)
    {
        int* iterator = std::next(data.data(), static_cast<std::ptrdiff_t>(N / 2U));
        benchmark::DoNotOptimize(iterator);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
