#include "sample_support.hpp"

#include "castle/memory/alignment.hpp"
#include "castle/memory/lifetime.hpp"
#include "castle/memory/new.hpp"

#include <stdint.h>

struct Counter
{
    explicit Counter(uint32_t initial) : value(initial) {}
    ~Counter() noexcept { value = 0U; }

    uint32_t value;
};

int main()
{
    castle::memory::aligned_storage_as_t<sizeof(Counter), Counter> storage{};

    Counter* first = ::new (storage.get_address<Counter>()) Counter(7U);
    Counter* laundered = castle::memory::launder(reinterpret_cast<Counter*>(storage.get_address<Counter>()));
    CASTLE_SAMPLE_CHECK(laundered == first);
    CASTLE_SAMPLE_CHECK(laundered->value == 7U);

    first->~Counter();

    Counter* second = ::new (storage.get_address<Counter>()) Counter(11U);
    Counter const* const_view = castle::memory::launder(static_cast<Counter const*>(second));
    CASTLE_SAMPLE_CHECK(const_view == second);
    CASTLE_SAMPLE_CHECK(const_view->value == 11U);

    second->~Counter();
    return 0;
}
