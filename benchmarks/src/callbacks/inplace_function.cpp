#include <benchmark/benchmark.h>

#include <cstdint>
#include <functional>

#include <castle/callbacks/inplace_function.hpp>

namespace
{

struct IncrementFunctor
{
    int offset;

    int operator()(int value) const
    {
        return value + offset;
    }
};

// ============================================================================
// inplace_function call
// ============================================================================

class inplace_function_call : public benchmark::Fixture {};

BENCHMARK_F(inplace_function_call, castle)(benchmark::State& state)
{
    castle::callbacks::inplace_function<int(int)> callback{IncrementFunctor{3}};

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(inplace_function_call, stl)(benchmark::State& state)
{
    std::function<int(int)> callback = IncrementFunctor{3};

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// inplace_function construction
// ============================================================================

class inplace_function_construct : public benchmark::Fixture {};

BENCHMARK_F(inplace_function_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::callbacks::inplace_function<int(int)> callback{IncrementFunctor{3}};
        benchmark::DoNotOptimize(callback);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(inplace_function_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::function<int(int)> callback = IncrementFunctor{3};
        benchmark::DoNotOptimize(callback);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
