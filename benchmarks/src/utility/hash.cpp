#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

#include <castle/utility/hash.hpp>

namespace
{

volatile std::uint32_t integral_hash_input = 0x12345678U;

// ============================================================================
// integral hash
// ============================================================================

class hash_integral : public benchmark::Fixture {};

BENCHMARK_F(hash_integral, castle)(benchmark::State& state)
{
    const std::uint32_t value = integral_hash_input;
    castle::hash<std::uint32_t> hasher;

    for (auto _ : state)
    {
        const auto result = hasher(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(hash_integral, stl)(benchmark::State& state)
{
    const std::uint32_t value = integral_hash_input;
    std::hash<std::uint32_t> hasher;

    for (auto _ : state)
    {
        const auto result = hasher(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string view hash
// ============================================================================

class hash_string_view : public benchmark::Fixture {};

BENCHMARK_F(hash_string_view, castle)(benchmark::State& state)
{
    state.PauseTiming();
    const castle::container::string_view value("embedded systems benchmark string");
    castle::hash<castle::container::string_view> hasher;
    state.ResumeTiming();

    for (auto _ : state)
    {
        const auto result = hasher(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(value.size()) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(hash_string_view, stl)(benchmark::State& state)
{
    state.PauseTiming();
    const std::string_view value("embedded systems benchmark string");
    std::hash<std::string_view> hasher;
    state.ResumeTiming();

    for (auto _ : state)
    {
        const auto result = hasher(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(value.size()) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
