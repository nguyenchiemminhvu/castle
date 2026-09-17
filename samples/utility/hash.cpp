#include "sample_support.hpp"

#include "castle/utility/hash.hpp"
#include "castle/container/string_view.hpp"

#include <stdint.h>

enum class channel : uint8_t
{
    a = 1U,
    b = 2U
};

int main()
{
    const uint32_t integral = 123U;
    const uint32_t same_integral = 123U;
    const channel id = channel::b;
    uint32_t storage = 77U;
    uint32_t* pointer = &storage;
    const castle::container::string_view key("TEMP");

    CASTLE_SAMPLE_CHECK(castle::hash<uint32_t>()(integral) == castle::hash<uint32_t>()(same_integral));
    CASTLE_SAMPLE_CHECK(castle::hash<const uint32_t>()(integral) == castle::hash<uint32_t>()(integral));
    CASTLE_SAMPLE_CHECK(castle::hash<channel>()(id) == castle::hash<channel>()(id));
    CASTLE_SAMPLE_CHECK(castle::hash<uint32_t*>()(pointer) == castle::hash<uint32_t*>()(pointer));
    CASTLE_SAMPLE_CHECK(castle::hash<castle::container::string_view>()(key) == castle::hash<castle::container::string_view>()(castle::container::string_view("TEMP")));
    return 0;
}
