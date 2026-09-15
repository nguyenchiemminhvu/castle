#include <benchmark/benchmark.h>

#include <castle/variant.hpp>

#if CASTLE_BENCHMARK_ENABLE_STL
#include <variant>
#endif

#include <cstdint>

namespace
{

using A = std::uint32_t;
using B = std::uint64_t;

using CastleVariant = castle::variant<A, B>;

#if CASTLE_BENCHMARK_ENABLE_STL
using STLVariant = std::variant<A, B>;
#endif

// ============================================================================
// Construction
// ============================================================================

static void BM_CastleVariant_Construct(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        CastleVariant value(
            static_cast<A>(42)
        );

        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK(BM_CastleVariant_Construct);


#if CASTLE_BENCHMARK_ENABLE_STL

static void BM_STLVariant_Construct(
    benchmark::State& state)
{
    for (auto _ : state)
    {
        STLVariant value(
            static_cast<A>(42)
        );

        benchmark::DoNotOptimize(value);
    }
}

BENCHMARK(BM_STLVariant_Construct);

#endif

} // namespace
