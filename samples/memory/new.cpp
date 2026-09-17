#include "sample_support.hpp"

#include "castle/memory/new.hpp"

#include <stdint.h>

struct Widget
{
    explicit Widget(uint32_t initial = 0U) : value(initial) {}
    ~Widget() noexcept { value = 0U; }

    uint32_t value;
};

int main()
{
    alignas(Widget) unsigned char object_bytes[sizeof(Widget)] = {};
    Widget* object = ::new (static_cast<void*>(object_bytes)) Widget(7U);
    CASTLE_SAMPLE_CHECK(object->value == 7U);

    alignas(Widget) unsigned char array_bytes[sizeof(Widget) * 2U] = {};
    Widget* array = ::new (static_cast<void*>(array_bytes)) Widget[2U]{Widget(1U), Widget(2U)};
    CASTLE_SAMPLE_CHECK(array[0].value == 1U);
    CASTLE_SAMPLE_CHECK(array[1].value == 2U);

    array[1].~Widget();
    array[0].~Widget();
    object->~Widget();
    return 0;
}
