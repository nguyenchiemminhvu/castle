#include <benchmark/benchmark.h>

#include <cstdint>
#include <variant>

#include <castle/utility/variant.hpp>

namespace
{

using castle_variant = castle::variant<int, float, std::uint32_t>;

// ============================================================================
// variant construction
// ============================================================================

class variant_construct : public benchmark::Fixture {};

BENCHMARK_F(variant_construct, castle)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const castle_variant value(17);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(variant_construct, stl)(benchmark::State& state)
{
    for (auto _ : state)
    {
        const std::variant<int, float, std::uint32_t> value(17);
        benchmark::DoNotOptimize(value);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// variant emplace
// ============================================================================

class variant_emplace : public benchmark::Fixture {};

BENCHMARK_F(variant_emplace, castle)(benchmark::State& state)
{
    state.PauseTiming();
    castle_variant value(17);
    state.ResumeTiming();

    for (auto _ : state)
    {
        value.emplace<1U>(3.5F);
        benchmark::DoNotOptimize(value);

        state.PauseTiming();
        value.emplace<0U>(17);
        state.ResumeTiming();
    }

    state.PauseTiming();

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(variant_emplace, stl)(benchmark::State& state)
{
    state.PauseTiming();
    std::variant<int, float, std::uint32_t> value(17);
    state.ResumeTiming();

    for (auto _ : state)
    {
        value.emplace<1U>(3.5F);
        benchmark::DoNotOptimize(value);

        state.PauseTiming();
        value.emplace<0U>(17);
        state.ResumeTiming();
    }

    state.PauseTiming();

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// variant get
// ============================================================================

class variant_get : public benchmark::Fixture {};

BENCHMARK_F(variant_get, castle)(benchmark::State& state)
{
    const castle_variant value(17);

    for (auto _ : state)
    {
        const int result = castle::get<0U>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(variant_get, stl)(benchmark::State& state)
{
    const std::variant<int, float, std::uint32_t> value(17);

    for (auto _ : state)
    {
        const int result = std::get<0U>(value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

// ============================================================================
// variant visit
// ============================================================================

class variant_visit : public benchmark::Fixture {};

BENCHMARK_F(variant_visit, castle)(benchmark::State& state)
{
    const castle_variant value(17);
    const auto visitor = [](const auto& item) { return static_cast<int>(item); };

    for (auto _ : state)
    {
        const int result = castle::visit(visitor, value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

#if CASTLE_BENCHMARK_ENABLE_STL
BENCHMARK_F(variant_visit, stl)(benchmark::State& state)
{
    const std::variant<int, float, std::uint32_t> value(17);
    const auto visitor = [](const auto& item) { return static_cast<int>(item); };

    for (auto _ : state)
    {
        const int result = std::visit(visitor, value);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
#endif // CASTLE_BENCHMARK_ENABLE_STL

} // namespace
