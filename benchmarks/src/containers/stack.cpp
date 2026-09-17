#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <castle/container/stack.hpp>

namespace
{

constexpr std::size_t N = 255;

// ============================================================================
// stack push/pop
// ============================================================================

class stack_push_pop : public benchmark::Fixture {};

BENCHMARK_F(stack_push_pop, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::stack_buffer<int, N> stack;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(stack.push(static_cast<int>(i)));
            }

            for (std::size_t i = 0U; i < N; ++i)
            {
                static_cast<void>(stack.pop());
            }

            benchmark::DoNotOptimize(stack);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U * N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(stack_push_pop, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<int> stack;
            state.ResumeTiming();

            for (std::size_t i = 0U; i < N; ++i)
            {
                stack.push_back(static_cast<int>(i));
            }

            for (std::size_t i = 0U; i < N; ++i)
            {
                stack.pop_back();
            }

            benchmark::DoNotOptimize(stack);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U * N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// stack top
// ============================================================================

class stack_top : public benchmark::Fixture {};

BENCHMARK_F(stack_top, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle::container::stack_buffer<int, N> stack;
    for (std::size_t i = 0U; i < N; ++i)
    {
        static_cast<void>(stack.push(static_cast<int>(i)));
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        const int result = stack.top();
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(stack_top, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::vector<int> stack;
    stack.reserve(N);
    for (std::size_t i = 0U; i < N; ++i)
    {
        stack.push_back(static_cast<int>(i));
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        const int result = stack.back();
        benchmark::DoNotOptimize(result);
    }

    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
