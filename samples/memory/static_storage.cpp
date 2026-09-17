#include "sample_support.hpp"

#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"

#include <stdint.h>

struct Counter
{
    explicit Counter(uint32_t initial) : value(initial) {}
    ~Counter() noexcept { value = 0U; }

    uint32_t value;
};

int main()
{
    castle::memory::static_storage<Counter, 2U> storage;
    Counter* first = castle::memory::construct_at<Counter>(storage.address(0U), 7U);
    Counter* second = castle::memory::construct_at<Counter>(storage.address(1U), 9U);

    CASTLE_SAMPLE_CHECK(decltype(storage)::capacity == 2U);
    CASTLE_SAMPLE_CHECK(storage.bytes() != nullptr);
    CASTLE_SAMPLE_CHECK(first != second);
    CASTLE_SAMPLE_CHECK(first->value == 7U);
    CASTLE_SAMPLE_CHECK(second->value == 9U);

    castle::memory::destroy_at(second);
    castle::memory::destroy_at(first);
    return 0;
}
