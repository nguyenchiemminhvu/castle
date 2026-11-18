#include "sample_support.hpp"

#include "castle/core/compiler_variants/clang.hpp"

namespace
{

CASTLE_INLINE int clang_backend_value()
{
    return 9;
}

} // namespace

int main()
{
    CASTLE_SAMPLE_CHECK(CASTLE_COMPILER_CLANG == 1);
    CASTLE_SAMPLE_CHECK(CASTLE_COMPILER_GCC == 1);
    CASTLE_SAMPLE_CHECK(clang_backend_value() == 9);
    return 0;
}
