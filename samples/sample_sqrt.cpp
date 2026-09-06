#include "sample_support.h"

#include "castle/math/sqrt.h"

// Scenario: compile-time integer square-root calculations used for
// deterministic buffer sizing and lookup-table dimensions.

int main()
{
    static_assert(castle::sqrt<0U>::value == 0U, "");
    static_assert(castle::sqrt<1U>::value == 1U, "");
    static_assert(castle::sqrt<144U>::value == 12U, "");
    static_assert(castle::sqrt<145U>::value == 12U, "");
    static_assert(castle::sqrt<255U>::value == 15U, "");
    static_assert(castle::sqrt<256U>::value == 16U, "");

    using root_type = castle::sqrt<144U>;

    CASTLE_SAMPLE_CHECK(root_type::value == 12U);
    CASTLE_SAMPLE_CHECK(root_type::type::value == 12U);

    return 0;
}