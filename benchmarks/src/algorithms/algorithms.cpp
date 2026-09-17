#include <benchmark/benchmark.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>

#include <castle/algorithm/algorithm.hpp>
#include <castle/container/array.hpp>

namespace
{

constexpr std::size_t N = 255;

using castle_array = castle::container::array<int, N>;

castle_array make_castle_data()
{
    castle_array data{};
    for (std::size_t i = 0U; i < N; ++i)
    {
        data[i] = static_cast<int>((i * 37U) & 0xFFU);
    }
    return data;
}

std::array<int, N> make_stl_data()
{
    std::array<int, N> data{};
    for (std::size_t i = 0U; i < N; ++i)
    {
        data[i] = static_cast<int>((i * 37U) & 0xFFU);
    }
    return data;
}

template <typename Container>
void reset_data(Container& data)
{
    for (std::size_t i = 0U; i < N; ++i)
    {
        data[i] = static_cast<int>((i * 37U) & 0xFFU);
    }
}

// ============================================================================
// find
// ============================================================================

class algorithm_find : public benchmark::Fixture {};

BENCHMARK_F(algorithm_find, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_array data = make_castle_data();
    const int value = 251;
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = castle::find(data.begin(), data.end(), value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(algorithm_find, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data = make_stl_data();
    const int value = 251;
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = std::find(data.begin(), data.end(), value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// sort
// ============================================================================

class algorithm_sort : public benchmark::Fixture {};

BENCHMARK_F(algorithm_sort, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_array data = make_castle_data();
    state.ResumeTiming();

    for (auto _ : state)
    {
        state.PauseTiming();
        reset_data(data);
        state.ResumeTiming();

        castle::sort(data.begin(), data.end());
        benchmark::DoNotOptimize(data);

        state.PauseTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(algorithm_sort, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data = make_stl_data();
    state.ResumeTiming();

    for (auto _ : state)
    {
        state.PauseTiming();
        reset_data(data);
        state.ResumeTiming();

        std::sort(data.begin(), data.end());
        benchmark::DoNotOptimize(data);

        state.PauseTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// transform
// ============================================================================

class algorithm_transform : public benchmark::Fixture {};

BENCHMARK_F(algorithm_transform, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_array input = make_castle_data();
    castle_array output{};
    state.ResumeTiming();

    for (auto _ : state)
    {
        castle::transform(
            input.begin(),
            input.end(),
            output.begin(),
            [](int value) { return value + 3; }
        );
        benchmark::DoNotOptimize(output);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(algorithm_transform, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> input = make_stl_data();
    std::array<int, N> output{};
    state.ResumeTiming();

    for (auto _ : state)
    {
        std::transform(
            input.begin(),
            input.end(),
            output.begin(),
            [](int value) { return value + 3; }
        );
        benchmark::DoNotOptimize(output);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// lower_bound
// ============================================================================

class algorithm_lower_bound : public benchmark::Fixture {};

BENCHMARK_F(algorithm_lower_bound, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_array data{};
    for (std::size_t i = 0U; i < N; ++i)
    {
        data[i] = static_cast<int>(i * 2U);
    }

    const int value = 173;
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = castle::lower_bound(data.begin(), data.end(), value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(algorithm_lower_bound, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::array<int, N> data{};
    for (std::size_t i = 0U; i < N; ++i)
    {
        data[i] = static_cast<int>(i * 2U);
    }

    const int value = 173;
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = std::lower_bound(data.begin(), data.end(), value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
