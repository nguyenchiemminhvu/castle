#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include <castle/events/sigslot.hpp>

namespace
{

using castle_signal = castle::sigslot::signal<4U, void(std::uint32_t)>;

// ============================================================================
// signal emit
// ============================================================================

class signal_emit : public benchmark::Fixture {};

BENCHMARK_F(signal_emit, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_signal signal;
    std::array<castle_signal::connection_type, 4U> connections{};

    for (std::size_t i = 0U; i < connections.size(); ++i)
    {
        connections[i] = signal.connect([](std::uint32_t) {});
    }
    state.ResumeTiming();

    for (auto _ : state)
    {
        signal.emit(17U);
    }

    benchmark::DoNotOptimize(signal);
    state.PauseTiming();
    state.SetItemsProcessed(static_cast<int64_t>(4U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(signal_emit, stl)(benchmark::State& state)
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
// signal connect/disconnect
// ============================================================================

class signal_connect_disconnect : public benchmark::Fixture {};

BENCHMARK_F(signal_connect_disconnect, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle_signal signal;
            state.ResumeTiming();

            auto connection = signal.connect([](std::uint32_t) {});
            benchmark::DoNotOptimize(connection);
            static_cast<void>(connection.disconnect());

            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(signal_connect_disconnect, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::vector<std::function<void(std::uint32_t)>> callbacks;
            callbacks.reserve(1U);
            state.ResumeTiming();

            callbacks.emplace_back([](std::uint32_t) {});
            benchmark::DoNotOptimize(callbacks);

            state.PauseTiming();
            callbacks.clear();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
