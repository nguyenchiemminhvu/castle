#include <benchmark/benchmark.h>

#include <cstdint>
#include <functional>
#include <vector>

#include <castle/events/event_config.hpp>
#include <castle/events/inplace_event_dispatcher.hpp>

namespace
{

struct TickEvent {};

using castle_dispatcher = castle::events::inplace_event_dispatcher<
    castle::events::event_config<TickEvent, 4U, void(std::uint32_t), 16U>>;

// ============================================================================
// inplace event dispatch with four subscribers
// ============================================================================

class inplace_event_dispatch : public benchmark::Fixture {};

BENCHMARK_F(inplace_event_dispatch, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_dispatcher dispatcher;
    for (std::size_t i = 0U; i < 4U; ++i)
    {
        static_cast<void>(dispatcher.register_callback<TickEvent>(
            [](std::uint32_t) {}
        ));
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        dispatcher.dispatch_event<TickEvent>(17U);
    }

    benchmark::DoNotOptimize(dispatcher);
    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(4U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(inplace_event_dispatch, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::vector<std::function<void(std::uint32_t)>> callbacks;
    callbacks.reserve(4U);
    for (std::size_t i = 0U; i < 4U; ++i)
    {
        callbacks.emplace_back([](std::uint32_t) {});
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        for (auto& callback : callbacks)
        {
                callback(17U);
            }
        }

        benchmark::DoNotOptimize(callbacks);
        state.PauseTiming();
        state.SetItemsProcessed(static_cast<int64_t>(4U) * state.iterations());
    }
    #endif // CASTLE_BENCHMARK_ENABLE_STL

    // ============================================================================
    // inplace event callback registration
    // ============================================================================

    class inplace_event_register : public benchmark::Fixture {};

    BENCHMARK_F(inplace_event_register, castle)(benchmark::State& state)
    {
        for (auto _ : state)
        {
            state.PauseTiming();
            {
            castle_dispatcher dispatcher;
            state.ResumeTiming();

            auto subscription = dispatcher.register_callback<TickEvent>(
                [](std::uint32_t) {}
            );
            benchmark::DoNotOptimize(subscription);

            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(inplace_event_register, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::function<void(std::uint32_t)> callback = [](std::uint32_t) {};
            state.ResumeTiming();

            benchmark::DoNotOptimize(callback);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
