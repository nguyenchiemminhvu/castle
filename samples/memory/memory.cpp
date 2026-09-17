#include "sample_support.hpp"

#include "castle/memory/alignment.hpp"
#undef CASTLE_MEMORY_MEMORY_HPP
#include "castle/memory/memory.hpp"

#include <stdint.h>

struct RegisterValue
{
    explicit RegisterValue(uint32_t initial) : value(initial) {}
    ~RegisterValue() noexcept { value = 0U; }

    uint32_t value;
};

int main()
{
    castle::aligned_storage_as_t<sizeof(RegisterValue), RegisterValue> storage{};
    RegisterValue* created = castle::memory::construct_at<RegisterValue>(storage.get_address<RegisterValue>(), 0x55U);
    RegisterValue* viewed = castle::memory::object_from_address<RegisterValue>(storage.get_address<RegisterValue>());

    CASTLE_SAMPLE_CHECK(castle::memory::is_aligned<RegisterValue>(created));
    CASTLE_SAMPLE_CHECK(castle::memory::addressof(*viewed) == created);
    CASTLE_SAMPLE_CHECK(viewed->value == 0x55U);

    castle::memory::destroy_at(created);
    return 0;
}
