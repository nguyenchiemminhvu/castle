#include "sample_support.hpp"

#include "castle/design_patterns/observer.hpp"

#include <stdint.h>

struct sensor_observer : castle::design_patterns::observer<uint16_t, void>
{
    void notify(uint16_t const& value) override
    {
        last_value = value;
        ++value_notifications;
    }

    void notify() override
    {
        ++empty_notifications;
    }

    uint16_t last_value = 0U;
    uint32_t value_notifications = 0U;
    uint32_t empty_notifications = 0U;
};

int main()
{
    sensor_observer first;
    sensor_observer second;
    sensor_observer third;
    castle::design_patterns::observable<sensor_observer, 2U> bus;

    CASTLE_SAMPLE_CHECK(!bus.add_observer(nullptr));
    CASTLE_SAMPLE_CHECK(bus.add_observer(&first));
    CASTLE_SAMPLE_CHECK(!bus.add_observer(&first));
    CASTLE_SAMPLE_CHECK(bus.add_observer(&second));
    CASTLE_SAMPLE_CHECK(!bus.add_observer(&third));

    bus.notify_observers<uint16_t>(42U);
    bus.notify_observers();
    CASTLE_SAMPLE_CHECK(first.last_value == 42U && second.last_value == 42U);
    CASTLE_SAMPLE_CHECK(first.value_notifications == 1U && second.value_notifications == 1U);
    CASTLE_SAMPLE_CHECK(first.empty_notifications == 1U && second.empty_notifications == 1U);

    CASTLE_SAMPLE_CHECK(bus.remove_observer(&first));
    CASTLE_SAMPLE_CHECK(!bus.remove_observer(&first));
    bus.notify_observers<uint16_t>(7U);
    CASTLE_SAMPLE_CHECK(first.last_value == 42U);
    CASTLE_SAMPLE_CHECK(second.last_value == 7U);
    return 0;
}
