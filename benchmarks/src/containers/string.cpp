#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>

#include <castle/container/string.hpp>

namespace
{

constexpr std::size_t N = 255;
constexpr char source_text[] = "This is a long long long long long sample string to be benchmarked";

// ============================================================================
// string construct
// ============================================================================

class string_construct : public benchmark::Fixture {};

BENCHMARK_F(string_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        castle::container::string<N> value(source_text);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::string value(source_text);
        value.reserve(N);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string append with pre-reserved STL storage
// ============================================================================

class string_append : public benchmark::Fixture {};

BENCHMARK_F(string_append, castle)(benchmark::State& state)
{
    const castle::container::string<N> source(source_text);

    for (auto _ : state)
    {
        state.PauseTiming();
        {
            castle::container::string<N> value;
            state.ResumeTiming();

            static_cast<void>(value.append(source));

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(source.size()) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_append, stl)(benchmark::State& state)
{
    const std::string source(source_text);

    for (auto _ : state)
    {
        state.PauseTiming();
        {
            std::string value;
            value.reserve(N);
            state.ResumeTiming();

            value.append(source);

            benchmark::DoNotOptimize(value);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }

    state.SetItemsProcessed(static_cast<int64_t>(source.size()) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string compare
// ============================================================================

class string_compare_equal : public benchmark::Fixture {};

BENCHMARK_F(string_compare_equal, castle)(benchmark::State& state)
{
    const castle::container::string<N> lhs(source_text);
    const castle::container::string<N> rhs(source_text);

    for (auto _ : state)
    {
        const bool result = (lhs == rhs);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(lhs.size()) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_compare_equal, stl)(benchmark::State& state)
{
    const std::string lhs(source_text);
    const std::string rhs(source_text);

    for (auto _ : state)
    {
        const bool result = (lhs == rhs);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(lhs.size()) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// string find
// ============================================================================

class string_find : public benchmark::Fixture {};

BENCHMARK_F(string_find, castle)(benchmark::State& state)
{
    const castle::container::string<N> value(source_text);

    for (auto _ : state)
    {
        const auto result = value.find(castle::container::string_view("sample"));
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(value.size()) * state.iterations());
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(string_find, stl)(benchmark::State& state)
{
    const std::string value(source_text);

    for (auto _ : state)
    {
        const auto result = value.find("sample");
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(value.size()) * state.iterations());
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
