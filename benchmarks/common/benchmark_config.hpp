#pragma once

#ifndef CASTLE_BENCHMARK_ENABLE_STL
#define CASTLE_BENCHMARK_ENABLE_STL 0
#endif

namespace castle_benchmark
{

static constexpr bool stl_enabled =
    CASTLE_BENCHMARK_ENABLE_STL != 0;

} // namespace castle_benchmark
