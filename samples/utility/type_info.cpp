#include "sample_support.hpp"

#include "castle/utility/type_info.hpp"

#include <stdint.h>
#include <string.h>

namespace telemetry
{
struct Sample
{
    uint16_t channel;
    int32_t value;
};
} // namespace telemetry

int main()
{
    // One descriptor per type; references and top-level cv-qualifiers are ignored like typeid.
    const castle::type_info& sample_info = castle::type_id<telemetry::Sample>();
    CASTLE_SAMPLE_CHECK(sample_info == castle::type_id<const telemetry::Sample&>());
    CASTLE_SAMPLE_CHECK(&sample_info == &castle::type_id<volatile telemetry::Sample>());
    CASTLE_SAMPLE_CHECK(sample_info != castle::type_id<telemetry::Sample*>());

    // Implementation-defined strict total order.
    const castle::type_info& a = castle::type_id<int32_t>();
    const castle::type_info& b = castle::type_id<float>();
    CASTLE_SAMPLE_CHECK(a.before(b) != b.before(a));
    CASTLE_SAMPLE_CHECK(!a.before(a));

    // Hash codes are equal for equal types.
    CASTLE_SAMPLE_CHECK(a.hash_code() == castle::type_id<const int32_t>().hash_code());

    // Names are extracted at compile time; spelling follows the compiler.
#if CASTLE_TYPE_INFO_NAMES
    CASTLE_SAMPLE_CHECK(strcmp(castle::type_id<int>().name(), "int") == 0);
    CASTLE_SAMPLE_CHECK(strcmp(castle::type_id<const int&>().name(), "int") == 0);
    CASTLE_SAMPLE_CHECK(strstr(sample_info.name(), "telemetry::Sample") != nullptr);
#else
    CASTLE_SAMPLE_CHECK(sample_info.name()[0] == '\0');
#endif

    // type_id<T>() is constexpr: the reference binds at compile time, no start-up code.
    static constexpr const castle::type_info& void_info = castle::type_id<void>();
    CASTLE_SAMPLE_CHECK(void_info == castle::type_id<const void>());
    return 0;
}
