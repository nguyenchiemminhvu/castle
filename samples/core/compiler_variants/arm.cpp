#include "sample_support.hpp"

#include "castle/core/compiler_variants/arm.hpp"

namespace
{

CASTLE_INLINE int arm_backend_value()
{
    return 7;
}

} // namespace

int main()
{
    CASTLE_SAMPLE_CHECK(CASTLE_COMPILER_ARM == 1);
    CASTLE_SAMPLE_CHECK(CASTLE_COMPILER_GCC == 1);
    CASTLE_SAMPLE_CHECK(arm_backend_value() == 7);
    return 0;
}
