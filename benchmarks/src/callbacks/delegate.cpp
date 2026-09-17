#include <benchmark/benchmark.h>

#include <cstdint>
#include <functional>

#include <castle/callbacks/delegate.hpp>

namespace
{

int increment_free(int value)
{
    return value + 1;
}

struct IncrementFunctor
{
    int operator()(int value) const
    {
        return value + 1;
    }
};

struct Receiver
{
    int increment(int value)
    {
        return value + 1;
    }
};

// ============================================================================
// Runtime-bound free function
// ============================================================================

class function_free_call : public benchmark::Fixture {};

BENCHMARK_F(function_free_call, castle)(benchmark::State& state)
{
    castle::callbacks::delegate_ptr<int(int)> callback(&increment_free);

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(function_free_call, stl)(benchmark::State& state)
{
    std::function<int(int)> callback(increment_free);

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// Compile-time-bound free function
// ============================================================================

class function_compile_time_call : public benchmark::Fixture {};

BENCHMARK_F(function_compile_time_call, castle)(benchmark::State& state)
{
    castle::callbacks::delegate_ptr_ct<&increment_free> callback;

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(function_compile_time_call, stl)(benchmark::State& state)
{
    auto callback = increment_free;

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// Stateful functor
// ============================================================================

class function_functor_call : public benchmark::Fixture {};

BENCHMARK_F(function_functor_call, castle)(benchmark::State& state)
{
    castle::callbacks::delegate_ft<IncrementFunctor, int(int)> callback{IncrementFunctor{}};

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(function_functor_call, stl)(benchmark::State& state)
{
    std::function<int(int)> callback = IncrementFunctor{};

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// Runtime-bound member function
// ============================================================================

class function_member_call : public benchmark::Fixture {};

BENCHMARK_F(function_member_call, castle)(benchmark::State& state)
{
    Receiver receiver;
    castle::callbacks::delegate_member<Receiver, int(int)> callback(receiver, &Receiver::increment);

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(function_member_call, stl)(benchmark::State& state)
{
    Receiver receiver;
    auto callback = [&receiver](int value) { return receiver.increment(value); };

    for (auto _ : state)
    {
        const int result = callback(17);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
