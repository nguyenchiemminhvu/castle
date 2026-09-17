#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <map>

#include <castle/container/map.hpp>

namespace
{

constexpr std::size_t N = 127;

using castle_map = castle::container::map<int, int, N>;

void fill_castle(castle_map& map)
{
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        static_cast<void>(map.insert(static_cast<int>(i), static_cast<int>(i * 3U)));
    }
}

#if CASTLE_BENCHMARK_ENABLE_STL
void fill_stl(std::map<int, int>& map)
{
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        map.emplace(static_cast<int>(i), static_cast<int>(i * 3U));
    }
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree map insert
// ============================================================================

class tree_map_insert : public benchmark::Fixture {};

BENCHMARK_F(tree_map_insert, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle_map map;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                static_cast<void>(map.insert(static_cast<int>(i), static_cast<int>(i * 3U)));
            }

            benchmark::DoNotOptimize(map);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_map_insert, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::map<int, int> map;
            state.ResumeTiming();

            fill_stl(map);

            benchmark::DoNotOptimize(map);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree map find
// ============================================================================

class tree_map_find : public benchmark::Fixture {};

BENCHMARK_F(tree_map_find, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_map map;
    fill_castle(map);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = map.find(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_map_find, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::map<int, int> map;
    fill_stl(map);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = map.find(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree map lower_bound
// ============================================================================

class tree_map_lower_bound : public benchmark::Fixture {};

BENCHMARK_F(tree_map_lower_bound, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_map map;
    fill_castle(map);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = map.lower_bound(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_map_lower_bound, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::map<int, int> map;
    fill_stl(map);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = map.lower_bound(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree map erase
// ============================================================================

class tree_map_erase : public benchmark::Fixture {};

BENCHMARK_F(tree_map_erase, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle_map map;
            fill_castle(map);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                static_cast<void>(map.erase(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(map);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_map_erase, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::map<int, int> map;
            fill_stl(map);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                static_cast<void>(map.erase(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(map);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
