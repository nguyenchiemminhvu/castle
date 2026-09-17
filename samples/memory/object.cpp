#include "sample_support.hpp"

#include "castle/memory/alignment.hpp"
#include "castle/memory/new.hpp"
#include "castle/memory/object.hpp"

#include <stdint.h>

struct RegisterValue
{
    uint32_t value;
};

int main()
{
    castle::memory::aligned_storage_as_t<sizeof(RegisterValue), RegisterValue> storage{};
    RegisterValue* created = ::new (storage.get_address<RegisterValue>()) RegisterValue{5U};
    RegisterValue* view = castle::memory::object_from_address<RegisterValue>(storage.get_address<RegisterValue>());
    RegisterValue const* const_view = castle::memory::object_from_address<RegisterValue>(static_cast<void const*>(created));

    CASTLE_SAMPLE_CHECK(view == created);
    CASTLE_SAMPLE_CHECK(const_view == created);
    CASTLE_SAMPLE_CHECK(view->value == 5U);
    CASTLE_SAMPLE_CHECK(const_view->value == 5U);
    return 0;
}
