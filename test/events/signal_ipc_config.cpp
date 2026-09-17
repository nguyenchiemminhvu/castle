#include <gtest/gtest.h>
#include "castle/events/signal_ipc_config.h"

namespace
{
TEST(SignalConfigTest, SignumMappingAndTraits)
{
    static_assert(castle::events::to_signum(castle::events::signal::sigusr1)==SIGUSR1,"usr1");
    using C=castle::events::signal_ipc_config<castle::events::signal::sigusr1,2,32,8>;
    EXPECT_EQ(castle::events::to_signum(castle::events::signal::sigusr1),SIGUSR1);
    EXPECT_EQ(castle::events::to_signum(castle::events::signal::sigterm),SIGTERM);
    EXPECT_EQ(C::max_callback,2U);
    EXPECT_EQ(C::storage_size,32U);
    EXPECT_EQ(C::storage_alignment,8U);
}
}
