#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include <castle/container/hash_set.hpp>

namespace
{

constexpr std::size_t N = 127;

void fill_castle(castle::container::hash_set<int, N>& set)
{
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        static_cast<void>(set.insert(static_cast<int>(i)));
    }
}

#if CASTLE_BENCHMARK_ENABLE_STL
void fill_stl(std::unordered_set<int>& set)
{
    set.reserve(N);
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        set.insert(static_cast<int>(i));
    }
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// hash set insert
// ============================================================================

class hash_set_insert : public benchmark::Fixture {};

BENCHMARK_F(hash_set_insert, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::hash_set<int, N> set;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                static_cast<void>(set.insert(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(set);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(hash_set_insert, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::unordered_set<int> set;
            set.reserve(N);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                set.insert(static_cast<int>(i));
            }

            benchmark::DoNotOptimize(set);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// hash set contains
// ============================================================================

class hash_set_contains : public benchmark::Fixture {};

BENCHMARK_F(hash_set_contains, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle::container::hash_set<int, N> set;
    fill_castle(set);
    state.ResumeTiming();

    for (auto _ : state)
    {
        const bool result = set.find(31) != set.end();
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(hash_set_contains, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::unordered_set<int> set;
    fill_stl(set);
    state.ResumeTiming();

    for (auto _ : state)
    {
        const bool result = set.find(31) != set.end();
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// hash set erase
// ============================================================================

class hash_set_erase : public benchmark::Fixture {};

BENCHMARK_F(hash_set_erase, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::hash_set<int, N> set;
            fill_castle(set);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                static_cast<void>(set.erase(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(set);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(hash_set_erase, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::unordered_set<int> set;
            fill_stl(set);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N / 2U; ++i)
            {
                static_cast<void>(set.erase(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(set);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
