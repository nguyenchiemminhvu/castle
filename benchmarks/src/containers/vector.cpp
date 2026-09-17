#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <castle/container/vector.hpp>

namespace
{

constexpr std::size_t N = 255;

// ============================================================================
// vector construct
// ============================================================================

class vector_construct : public benchmark::Fixture {};

BENCHMARK_F(vector_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::container::vector<int, N> value;
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(vector_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::vector<int> value;
        value.reserve(N);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// vector push back
// ============================================================================

class vector_push_back : public benchmark::Fixture {};

BENCHMARK_F(vector_push_back, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::vector<int, N> value;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(value.push_back(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(vector_push_back, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<int> value;
            value.reserve(N);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                value.push_back(static_cast<int>(i));
            }

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// vector emplace back
// ============================================================================

class vector_emplace_back : public benchmark::Fixture {};

BENCHMARK_F(vector_emplace_back, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::vector<int, N> value;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(value.emplace_back(static_cast<int>(i)));
            }

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(vector_emplace_back, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<int> value;
            value.reserve(N);
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                value.emplace_back(static_cast<int>(i));
            }

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// vector pop back
// ============================================================================

class vector_pop_back : public benchmark::Fixture {};

BENCHMARK_F(vector_pop_back, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::vector<int, N> value;
            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(value.emplace_back(static_cast<int>(i)));
            }
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(value.pop_back());
            }

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(vector_pop_back, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<int> value;
            value.reserve(N);
            for (std::size_t i = 0U; i < N; ++i)
            {
                value.emplace_back(static_cast<int>(i));
            }
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                value.pop_back();
            }

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// vector clear of a populated vector
// ============================================================================

class vector_clear : public benchmark::Fixture {};

BENCHMARK_F(vector_clear, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::vector<int, N> value;
            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(value.emplace_back(static_cast<int>(i)));
            }
            state.ResumeTiming();

            value.clear();

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(vector_clear, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<int> value;
            value.reserve(N);
            for (std::size_t i = 0U; i < N; ++i)
            {
                value.emplace_back(static_cast<int>(i));
            }
            state.ResumeTiming();

            value.clear();

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
