#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <set>

#include <castle/container/set.hpp>

namespace
{

constexpr std::size_t N = 127;

using castle_set = castle::container::set<int, N>;

void fill_castle(castle_set& set)
{
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        static_cast<void>(set.insert(static_cast<int>(i)));
    }
}

#if CASTLE_BENCHMARK_ENABLE_STL
void fill_stl(std::set<int>& set)
{
    for (std::size_t i = 0U; i < N / 2U; ++i)
    {
        set.insert(static_cast<int>(i));
    }
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree set insert
// ============================================================================

class tree_set_insert : public benchmark::Fixture {};

BENCHMARK_F(tree_set_insert, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle_set set;
            state.ResumeTiming();

            fill_castle(set);

            benchmark::DoNotOptimize(set);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_set_insert, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::set<int> set;
            state.ResumeTiming();

            fill_stl(set);

            benchmark::DoNotOptimize(set);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N / 2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree set find
// ============================================================================

class tree_set_find : public benchmark::Fixture {};

BENCHMARK_F(tree_set_find, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_set set;
    fill_castle(set);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = set.find(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_set_find, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::set<int> set;
    fill_stl(set);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = set.find(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree set lower_bound
// ============================================================================

class tree_set_lower_bound : public benchmark::Fixture {};

BENCHMARK_F(tree_set_lower_bound, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_set set;
    fill_castle(set);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = set.lower_bound(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(tree_set_lower_bound, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::set<int> set;
    fill_stl(set);
    state.ResumeTiming();

    for (auto _ : state)
    {
        auto result = set.lower_bound(31);
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// tree set erase
// ============================================================================

class tree_set_erase : public benchmark::Fixture {};

BENCHMARK_F(tree_set_erase, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle_set set;
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
BENCHMARK_F(tree_set_erase, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::set<int> set;
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
