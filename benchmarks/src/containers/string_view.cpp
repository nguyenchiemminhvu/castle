#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string_view>

#include <castle/container/string_view.hpp>

namespace
{

constexpr char text[] =
    "This is a long long long long long sample string used by the benchmark";
constexpr std::size_t text_length = sizeof(text) - 1U;

// ============================================================================
// string_view construction
// ============================================================================

class string_view_construct : public benchmark::Fixture {};

BENCHMARK_F(string_view_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::container::string_view value(text);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_view_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::string_view value(text);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string_view find character
// ============================================================================

class string_view_find : public benchmark::Fixture {};

BENCHMARK_F(string_view_find, castle)(benchmark::State& state)
{
    const castle::container::string_view value(text);

    for (auto _ : state)
    {
        const auto result = value.find('s');
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(text_length) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_view_find, stl)(benchmark::State& state)
{
    const std::string_view value(text);

    for (auto _ : state)
    {
        const auto result = value.find('s');
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(text_length) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string_view substring find
// ============================================================================

class string_view_find_substring : public benchmark::Fixture {};

BENCHMARK_F(string_view_find_substring, castle)(benchmark::State& state)
{
    const castle::container::string_view value(text);
    const castle::container::string_view needle("sample");

    for (auto _ : state)
    {
        const auto result = value.find(needle);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(text_length) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_view_find_substring, stl)(benchmark::State& state)
{
    const std::string_view value(text);
    const std::string_view needle("sample");

    for (auto _ : state)
    {
        const auto result = value.find(needle);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(text_length) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string_view compare
// ============================================================================

class string_view_compare : public benchmark::Fixture {};

BENCHMARK_F(string_view_compare, castle)(benchmark::State& state)
{
    const castle::container::string_view lhs(text);
    const castle::container::string_view rhs(text);

    for (auto _ : state)
    {
        const int result = lhs.compare(rhs);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(text_length) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_view_compare, stl)(benchmark::State& state)
{
    const std::string_view lhs(text);
    const std::string_view rhs(text);

    for (auto _ : state)
    {
        const int result = lhs.compare(rhs);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(text_length) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
