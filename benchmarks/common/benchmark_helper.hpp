#pragma once

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace castle_benchmark
{

template <typename T>
inline void DoNotOptimize(const T& value)
{
    benchmark::DoNotOptimize(value);
}

template <typename T>
inline void ClobberMemory()
{
    benchmark::ClobberMemory();
}

inline void SetItemsProcessed(
    benchmark::State& state,
    std::size_t count)
{
    state.SetItemsProcessed(
        static_cast<int64_t>(count) * state.iterations()
    );
}

} // namespace castle_benchmark
