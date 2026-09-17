#include <gtest/gtest.h>
#include "castle/events/inplace_signal_ipc_event.h"
#include <signal.h>

namespace
{
using E=castle::events::inplace_signal_event<
    castle::events::signal_ipc_config<castle::events::signal::sigusr2,2,32,8>>;

volatile sig_atomic_t hits=0;

TEST(InplaceSignalIpcEventTest, OwnedCallbackLifecycle)
{
    E::uninstall();
    E::clear();
    EXPECT_EQ(E::signal_capacity(),1U);
    EXPECT_EQ(E::callback_capacity<castle::events::signal::sigusr2>(),2U);
    EXPECT_EQ(E::callback_storage_size<castle::events::signal::sigusr2>(),32U);
    EXPECT_EQ(E::callback_storage_alignment<castle::events::signal::sigusr2>(),8U);
    castle::status e=castle::status::unknown_error;
    auto s=E::register_callback<castle::events::signal::sigusr2>([](){++hits;},&e);
    EXPECT_TRUE(s.valid());
    EXPECT_EQ(e,castle::status::ok);
    EXPECT_EQ(E::install(),castle::status::ok);
    hits=0;
    raise(SIGUSR2);
    EXPECT_EQ(hits,1);
    E::disable_signal<castle::events::signal::sigusr2>();
    raise(SIGUSR2);
    EXPECT_EQ(hits,1);
    E::enable_signal<castle::events::signal::sigusr2>();
    raise(SIGUSR2);
    EXPECT_EQ(hits,2);
    E::clear_signal<castle::events::signal::sigusr2>();
    EXPECT_EQ(E::subscriber_count<castle::events::signal::sigusr2>(),0U);
    E::uninstall();
}
}
