#include <gtest/gtest.h>
#include "castle/events/signal_ipc_event.h"
#include <signal.h>

namespace
{
using E=castle::events::signal_ipc_event<castle::events::signal_ipc_config<
    castle::events::signal::sigusr1,2>>;

volatile sig_atomic_t hits=0;

void hit()
{
    ++hits;
}

TEST(SignalIpcEventTest, InstallEnableDisableDispatchAndClear)
{
    E::uninstall();
    E::clear();
    EXPECT_FALSE(E::is_installed());
    EXPECT_EQ(E::signal_capacity(),1U);
    static_assert(E::callback_capacity<castle::events::signal::sigusr1>()==2U,"cap"); castle::callbacks::function<void()> cb(&hit);
    castle::status e=castle::status::unknown_error;
    auto sub=E::register_callback<castle::events::signal::sigusr1>(&cb,&e);
    EXPECT_TRUE(sub.valid());
    EXPECT_EQ(e,castle::status::ok);
    EXPECT_EQ(E::subscriber_count<castle::events::signal::sigusr1>(),1U);
    EXPECT_EQ(E::install(),castle::status::ok);
    EXPECT_TRUE(E::is_installed()); hits=0; raise(SIGUSR1);
    EXPECT_EQ(hits,1);
    E::disable_signal<castle::events::signal::sigusr1>();
    EXPECT_FALSE(E::is_signal_enabled<castle::events::signal::sigusr1>());
    raise(SIGUSR1);
    EXPECT_EQ(hits,1);
    E::enable_signal<castle::events::signal::sigusr1>();
    raise(SIGUSR1);
    EXPECT_EQ(hits,2);
    E::clear_signal<castle::events::signal::sigusr1>();
    EXPECT_EQ(E::subscriber_count<castle::events::signal::sigusr1>(),0U);
    E::uninstall();
    EXPECT_FALSE(E::is_installed());
    E::uninstall();
}
}
