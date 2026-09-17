#include "sample_support.hpp"

#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/storage.hpp"

#include <stdint.h>

struct Counter
{
    explicit Counter(uint32_t initial) : value(initial) {}
    ~Counter() noexcept { value = 0U; }

    uint32_t value;
};

int main()
{
    castle::memory::raw_storage<Counter, 2U> storage;
    Counter* first = castle::memory::construct_at<Counter>(storage.address(0U), 11U);
    Counter* second = castle::memory::construct_at<Counter>(storage.address(1U), 13U);

    CASTLE_SAMPLE_CHECK(decltype(storage)::capacity == 2U);
    CASTLE_SAMPLE_CHECK(storage.bytes() != nullptr);
    CASTLE_SAMPLE_CHECK(first->value == 11U);
    CASTLE_SAMPLE_CHECK(second->value == 13U);

    castle::memory::destroy_at(second);
    castle::memory::destroy_at(first);

    castle::memory::raw_storage<Counter, 0U> empty_storage;
    CASTLE_SAMPLE_CHECK(decltype(empty_storage)::capacity == 0U);
    CASTLE_SAMPLE_CHECK(empty_storage.address(0U) == nullptr);
    CASTLE_SAMPLE_CHECK(empty_storage.bytes() == nullptr);
    return 0;
}
