#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <deque>

#include <castle/container/ring_buffer.hpp>

namespace
{

constexpr std::size_t N = 255;

// ============================================================================
// queue push/pop
// ============================================================================

class queue_push_pop : public benchmark::Fixture {};

BENCHMARK_F(queue_push_pop, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::ring_buffer<int, N> queue;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(queue.push(static_cast<int>(i)));
            }

            int value = 0;
            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(queue.pop(value));
                benchmark::DoNotOptimize(value);
            }

            benchmark::DoNotOptimize(queue);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U * N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(queue_push_pop, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::deque<int> queue;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                queue.push_back(static_cast<int>(i));
            }

            int value = 0;
            for (std::size_t i = 0U; i < N; ++i)
            {
                value = queue.front();
                queue.pop_front();
                benchmark::DoNotOptimize(value);
            }

            benchmark::DoNotOptimize(queue);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U * N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// queue peek/front
// ============================================================================

class queue_front : public benchmark::Fixture {};

BENCHMARK_F(queue_front, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle::container::ring_buffer<int, N> queue;
    for (std::size_t i = 0U; i < N; ++i)
    {
        static_cast<void>(queue.push(static_cast<int>(i)));
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        const int result = queue.front();
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(queue_front, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::deque<int> queue;
    for (std::size_t i = 0U; i < N; ++i)
    {
        queue.push_back(static_cast<int>(i));
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        const int result = queue.front();
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
