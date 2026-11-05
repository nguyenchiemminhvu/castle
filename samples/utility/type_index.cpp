#include "sample_support.hpp"

#include "castle/utility/type_index.hpp"
#include "castle/utility/compare.hpp"
#include "castle/container/hash_map.hpp"

#include <stdint.h>
#include <string.h>

namespace
{

// Fixed-capacity registry mapping a payload type to a handler id.
struct route
{
    castle::type_index type;
    uint8_t handler;
};

constexpr uint8_t no_handler = 0xFFU;

// uint8_t find_handler(const route* routes, uint8_t count, castle::type_index type)
// {
//     for (uint8_t i = 0U; i < count; ++i)
//     {
//         if (routes[i].type == type)
//         {
//             return routes[i].handler;
//         }
//     }
//     return no_handler;
// }

} // namespace

int main()
{
    // const route routes[] = {
    //     { castle::type_id<uint32_t>(), 1U },
    //     { castle::type_id<float>(), 2U },
    // };
    // CASTLE_SAMPLE_CHECK(find_handler(routes, 2U, castle::type_id<const float&>()) == 2U);
    // CASTLE_SAMPLE_CHECK(find_handler(routes, 2U, castle::type_id<uint32_t>()) == 1U);
    // CASTLE_SAMPLE_CHECK(find_handler(routes, 2U, castle::type_id<int8_t>()) == no_handler);

    // // Copyable value with full comparison set.
    // castle::type_index a = castle::type_id<uint32_t>();
    // castle::type_index b = castle::type_id<float>();
    // castle::type_index copy = a;
    // CASTLE_SAMPLE_CHECK(copy == a);
    // CASTLE_SAMPLE_CHECK(a != b);
    // CASTLE_SAMPLE_CHECK((a < b) != (b < a));
    // CASTLE_SAMPLE_CHECK((a <= b) == !(a > b));
    // CASTLE_SAMPLE_CHECK((a >= b) == !(a < b));
    // CASTLE_SAMPLE_CHECK(a <= copy);
    // CASTLE_SAMPLE_CHECK(a >= copy);
    // CASTLE_SAMPLE_CHECK(castle::less<castle::type_index>()(a, b) == (a < b));

    // // Hashing for Castle hash containers.
    // CASTLE_SAMPLE_CHECK(castle::hash<castle::type_index>()(a) == a.hash_code());
    // CASTLE_SAMPLE_CHECK(a.hash_code() == castle::type_id<uint32_t>().hash_code());

    // // Names forward to type_info.
    // CASTLE_SAMPLE_CHECK(strcmp(a.name(), castle::type_id<uint32_t>().name()) == 0);

    // ---

    using handler_map = castle::container::hash_map<
        castle::type_index,
        uint8_t,
        8U>;

    handler_map routes;

    routes.insert(castle::type_id<uint32_t>(), 1U);
    routes.insert(castle::type_id<float>(), 2U);

    // Lookup by type.
    const uint8_t* float_handler = routes.get(castle::type_id<const float&>());
    const uint8_t* uint_handler = routes.get(castle::type_id<uint32_t>());
    const uint8_t* int8_handler = routes.get(castle::type_id<int8_t>());

    CASTLE_SAMPLE_CHECK(float_handler != nullptr);
    CASTLE_SAMPLE_CHECK(*float_handler == 2U);

    CASTLE_SAMPLE_CHECK(uint_handler != nullptr);
    CASTLE_SAMPLE_CHECK(*uint_handler == 1U);

    CASTLE_SAMPLE_CHECK(int8_handler == nullptr);

    // Copyable value with full comparison set.
    castle::type_index a = castle::type_id<uint32_t>();
    castle::type_index b = castle::type_id<float>();
    castle::type_index copy = a;

    CASTLE_SAMPLE_CHECK(copy == a);
    CASTLE_SAMPLE_CHECK(a != b);
    CASTLE_SAMPLE_CHECK((a < b) != (b < a));
    CASTLE_SAMPLE_CHECK((a <= b) == !(a > b));
    CASTLE_SAMPLE_CHECK((a >= b) == !(a < b));
    CASTLE_SAMPLE_CHECK(a <= copy);
    CASTLE_SAMPLE_CHECK(a >= copy);

    // Hashing used by hash_map.
    CASTLE_SAMPLE_CHECK(
        castle::hash<castle::type_index>()(a) ==
        a.hash_code());

    CASTLE_SAMPLE_CHECK(
        a.hash_code() ==
        castle::type_id<uint32_t>().hash_code());

    // Names forward to type_info.
    CASTLE_SAMPLE_CHECK(
        strcmp(
            a.name(),
            castle::type_id<uint32_t>().name()) == 0);

    return 0;
}
