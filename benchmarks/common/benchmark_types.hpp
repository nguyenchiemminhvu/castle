#pragma once

#include <cstddef>
#include <cstdint>

namespace castle_benchmark
{

template <std::size_t N>
struct TestData
{
    std::uint32_t values[N]{};

    TestData()
    {
        for (std::size_t i = 0; i < N; ++i)
        {
            values[i] =
                static_cast<std::uint32_t>(
                    i * 2654435761u
                );
        }
    }
};

} // namespace castle_benchmark
