#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include <castle/container/hash_map.hpp>

namespace
{

constexpr std::size_t N = 127;

void fill_castle(castle::container::hash_map<int, int, N>& map)
{
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        static_cast<void>(map.insert(static_cast<int>(i), static_cast<int>(i * 3U)));
    }
}

#if CASTLE_BENCHMARK_ENABLE_STL
void fill_stl(std::unordered_map<int, int>& map)
{
    map.reserve(N);
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        map.emplace(static_cast<int>(i), static_cast<int>(i * 3U));
    }
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// hash map insert
// ============================================================================

class hash_map_insert : public benchmark::Fixture {};

BENCHMARK_F(hash_map_insert, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::hash_map<int, int, N> map;
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
BENCHMARK_F(hash_map_insert, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::unordered_map<int, int> map;
            map.reserve(N);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                map.emplace(static_cast<int>(i), static_cast<int>(i * 3U));
            }

            benchmark::DoNotOptimize(map);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// hash map find hit
// ============================================================================

class hash_map_find_hit : public benchmark::Fixture {};

BENCHMARK_F(hash_map_find_hit, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle::container::hash_map<int, int, N> map;
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
BENCHMARK_F(hash_map_find_hit, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::unordered_map<int, int> map;
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
// hash map find miss
// ============================================================================

class hash_map_find_miss : public benchmark::Fixture {};

BENCHMARK_F(hash_map_find_miss, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle::container::hash_map<int, int, N> map;
    fill_castle(map);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = map.find(static_cast<int>(N + 7U));
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(hash_map_find_miss, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::unordered_map<int, int> map;
    fill_stl(map);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = map.find(static_cast<int>(N + 7U));
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// hash map erase
// ============================================================================

class hash_map_erase : public benchmark::Fixture {};

BENCHMARK_F(hash_map_erase, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::hash_map<int, int, N> map;
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
BENCHMARK_F(hash_map_erase, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::unordered_map<int, int> map;
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
