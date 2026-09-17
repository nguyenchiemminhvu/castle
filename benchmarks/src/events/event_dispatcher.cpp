#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include <castle/callbacks/function.hpp>
#include <castle/events/event_config.hpp>
#include <castle/events/event_dispatcher.hpp>

namespace
{

struct TickEvent {};

using castle_dispatcher = castle::events::event_dispatcher<
    castle::events::event_config<TickEvent, 4U, void(std::uint32_t)>>;

void on_tick(std::uint32_t)
{
}

// ============================================================================
// event dispatch with four subscribers
// ============================================================================

class event_dispatch : public benchmark::Fixture {};

BENCHMARK_F(event_dispatch, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_dispatcher dispatcher;
    castle::callbacks::function_ct<&on_tick> callback;
    std::array<castle::callbacks::callback_subscription, 4U> subscriptions{};

    for (std::size_t i = 0U; i < subscriptions.size(); ++i)
    {
        subscriptions[i] = dispatcher.register_callback<TickEvent>(&callback);
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
BENCHMARK_F(event_dispatch, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::vector<std::function<void(std::uint32_t)>> callbacks;
    callbacks.reserve(4U);
    for (std::size_t i = 0U; i < 4U; ++i)
    {
        callbacks.emplace_back(on_tick);
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
    // event registration
    // ============================================================================

    class event_register : public benchmark::Fixture {};

    BENCHMARK_F(event_register, castle)(benchmark::State& state)
    {
        castle::callbacks::function_ct<&on_tick> callback;

        for (auto _ : state)
        {
            state.PauseTiming();
            {
            castle_dispatcher dispatcher;
            state.ResumeTiming();

            auto subscription = dispatcher.register_callback<TickEvent>(&callback);
            benchmark::DoNotOptimize(subscription);

            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(event_register, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<std::function<void(std::uint32_t)>> callbacks;
            callbacks.reserve(4U);
            state.ResumeTiming();

            callbacks.emplace_back(on_tick);
            benchmark::DoNotOptimize(callbacks);

            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
