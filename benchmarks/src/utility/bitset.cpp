#include <benchmark/benchmark.h>

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <string>

#include <castle/utility/bitset.hpp>

namespace
{

constexpr std::size_t N = 128;

// ============================================================================
// bitset set/reset a bit
// ============================================================================

class bitset_set_reset : public benchmark::Fixture {};

BENCHMARK_F(bitset_set_reset, castle)(benchmark::State& state)
{
    castle::bitset<N> value;

    for (auto _ : state)
    {
        value.set(37U);
        value.reset(37U);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(bitset_set_reset, stl)(benchmark::State& state)
{
    std::bitset<N> value;

    for (auto _ : state)
    {
        value.set(37U);
        value.reset(37U);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(2U) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// bitset count
// ============================================================================

class bitset_count : public benchmark::Fixture {};

BENCHMARK_F(bitset_count, castle)(benchmark::State& state)
{
    castle::bitset<N> value;
    value.set();

    for (auto _ : state)
    {
        const auto result = value.count();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(bitset_count, stl)(benchmark::State& state)
{
    std::bitset<N> value;
    value.set();

    for (auto _ : state)
    {
        const auto result = value.count();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// bitset test
// ============================================================================

class bitset_test : public benchmark::Fixture {};

BENCHMARK_F(bitset_test, castle)(benchmark::State& state)
{
    castle::bitset<N> value(0xA5A5A5A5ULL);

    for (auto _ : state)
    {
        const bool result = value.test(91U);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(bitset_test, stl)(benchmark::State& state)
{
    std::bitset<N> value(0xA5A5A5A5ULL);

    for (auto _ : state)
    {
        const bool result = value.test(91U);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// bitset conversion
// ============================================================================

class bitset_to_ullong : public benchmark::Fixture {};

BENCHMARK_F(bitset_to_ullong, castle)(benchmark::State& state)
{
    castle::bitset<64U> value(0x123456789ABCDEF0ULL);

    for (auto _ : state)
    {
        const auto result = value.to_ullong();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(bitset_to_ullong, stl)(benchmark::State& state)
{
    std::bitset<64U> value(0x123456789ABCDEF0ULL);

    for (auto _ : state)
    {
        const auto result = value.to_ullong();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// bitset string conversion
// ============================================================================

class bitset_to_string : public benchmark::Fixture {};

BENCHMARK_F(bitset_to_string, castle)(benchmark::State& state)
{
    castle::bitset<N> value(0xA5A5A5A5ULL);
    char buffer[N + 1U]{};

    for (auto _ : state)
    {
        const bool result = value.to_string(buffer, sizeof(buffer));
        benchmark::DoNotOptimize(result);
        benchmark::DoNotOptimize(buffer);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(bitset_to_string, stl)(benchmark::State& state)
{
    std::bitset<N> value(0xA5A5A5A5ULL);

    for (auto _ : state)
    {
        const std::string result = value.to_string();
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(N) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
