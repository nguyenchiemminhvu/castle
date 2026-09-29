#include <gtest/gtest.h>

#include "castle_ext/io/gpio/backends/zephyr.hpp"

#if !defined(CASTLE_EXT_ENABLE_GPIO_ZEPHYR)
TEST(GpioZephyrBackend, HeaderRemainsAvailableWhenZephyrIsDisabled)
{
    SUCCEED();
}
#endif
