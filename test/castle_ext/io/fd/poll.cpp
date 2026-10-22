#include <gtest/gtest.h>

#include "castle_ext/io/fd/poll.hpp"
#include "castle/chrono/clocks.hpp"
#include "castle/chrono/duration.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <signal.h>
    #include <sys/time.h>
    #include <unistd.h>
#endif

namespace
{

#if CASTLE_USING_POSIX_APIS
TEST(FdPoll, RegistrationCapacityAndMetadata)
{
    int first[2] = {-1, -1};
    int second[2] = {-1, -1};
    ASSERT_EQ(::pipe(first), 0);
    ASSERT_EQ(::pipe(second), 0);

    using manager_type = castle::io::fd::basic_fd_event_manager<1U>;
    manager_type manager;
    manager_type::registration_info info{};
    manager_type::callback_type empty_callback;

    EXPECT_EQ(manager.add_fd(-1, castle::io::fd::poll_read, empty_callback), castle::status::invalid_argument);
    EXPECT_EQ(manager.add_fd(first[0], castle::io::fd::poll_read, empty_callback), castle::status::invalid_callback);
    EXPECT_EQ(manager.add_fd(first[0], castle::io::fd::poll_read,
        [](int, short, void*) {}), castle::status::ok);
    EXPECT_EQ(manager.add_fd(first[0], castle::io::fd::poll_read,
        [](int, short, void*) {}), castle::status::already_exists);
    EXPECT_EQ(manager.add_fd(second[0], castle::io::fd::poll_read,
        [](int, short, void*) {}), castle::status::full);

    EXPECT_TRUE(manager.has_fd(first[0]));
    EXPECT_FALSE(manager.has_fd(second[0]));
    EXPECT_EQ(manager.get_fd_count(), 1U);
    EXPECT_EQ(manager.get_enabled_fd_count(), 1U);
    ASSERT_EQ(manager.get_fd_info(first[0], info), castle::status::ok);
    EXPECT_EQ(info.fd, first[0]);
    EXPECT_EQ(info.events, castle::io::fd::poll_read);
    EXPECT_TRUE(info.enabled);
    EXPECT_EQ(info.user_data, nullptr);
    EXPECT_EQ(info.name, nullptr);
    EXPECT_EQ(manager.get_fd_info(second[0], info), castle::status::not_found);

    EXPECT_EQ(manager.disable_fd(second[0]), castle::status::not_found);
    EXPECT_EQ(manager.enable_fd(second[0]), castle::status::not_found);
    EXPECT_EQ(manager.modify_events(second[0], castle::io::fd::poll_write), castle::status::not_found);
    EXPECT_EQ(manager.remove_fd(second[0]), castle::status::not_found);

    EXPECT_EQ(manager.disable_fd(first[0]), castle::status::ok);
    EXPECT_EQ(manager.disable_fd(first[0]), castle::status::ok); // idempotent
    EXPECT_EQ(manager.get_enabled_fd_count(), 0U);
    EXPECT_EQ(manager.enable_fd(first[0]), castle::status::ok);
    EXPECT_EQ(manager.enable_fd(first[0]), castle::status::ok); // idempotent
    EXPECT_EQ(manager.get_enabled_fd_count(), 1U);
    EXPECT_EQ(manager.modify_events(first[0], castle::io::fd::poll_write), castle::status::ok);
    EXPECT_EQ(manager.modify_events(first[0], castle::io::fd::poll_write), castle::status::ok); // unchanged mask
    ASSERT_EQ(manager.get_fd_info(first[0], info), castle::status::ok);
    EXPECT_EQ(info.events, castle::io::fd::poll_write);
    EXPECT_EQ(manager.modify_events(first[0], castle::io::fd::poll_read), castle::status::ok);

    manager.clear();
    EXPECT_EQ(manager.get_fd_count(), 0U);
    EXPECT_EQ(manager.get_enabled_fd_count(), 0U);
    EXPECT_EQ(manager.remove_fd(first[0]), castle::status::not_found);

    (void)::close(first[0]);
    (void)::close(first[1]);
    (void)::close(second[0]);
    (void)::close(second[1]);
}

TEST(FdPoll, WaitTimeoutAndNoEnabledDescriptors)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    castle::io::fd::basic_fd_event_manager<2U> manager;
    int calls = 0;
    ASSERT_EQ(manager.add_fd(descriptors[0], castle::io::fd::poll_read,
        [](int, short, void* context) { ++*static_cast<int*>(context); }, &calls), castle::status::ok);

    castle::size_type ready = 99U;
    EXPECT_EQ(manager.wait(0, ready), castle::status::time_out);
    EXPECT_EQ(ready, 0U);
    EXPECT_EQ(manager.pending_event_count(), 0U);

    EXPECT_EQ(manager.disable_fd(descriptors[0]), castle::status::ok);
    EXPECT_EQ(manager.wait(10, ready), castle::status::ok);
    EXPECT_EQ(ready, 0U);

    EXPECT_EQ(manager.process_events(ready), castle::status::ok);
    EXPECT_EQ(ready, 0U);
    EXPECT_EQ(calls, 0);

    manager.clear();
    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdPoll, QueuesAndDispatchesReadyCallbacks)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    castle::io::fd::fd_event_manager manager;
    int calls = 0;
    short received_events = 0;
    ASSERT_EQ(manager.add_fd(descriptors[0], castle::io::fd::poll_read,
        [&calls, &received_events](int fd, short revents, void* context)
        {
            (void)context;
            received_events = revents;
            char value = 0;
            if (::read(fd, &value, 1U) == 1)
            {
                ++calls;
            }
        }, nullptr, "queued-pipe"), castle::status::ok);

    const char value = 'q';
    ASSERT_EQ(::write(descriptors[1], &value, 1U), 1);
    castle::size_type ready = 0U;
    ASSERT_EQ(manager.wait(1000, ready), castle::status::ok);
    EXPECT_EQ(ready, 1U);
    EXPECT_EQ(manager.pending_event_count(), 1U);

    castle::size_type ignored = 0U;
    EXPECT_EQ(manager.wait(0, ignored), castle::status::invalid_config);

    castle::size_type processed = 0U;
    ASSERT_EQ(manager.process_events(processed), castle::status::ok);
    EXPECT_EQ(processed, 1U);
    EXPECT_EQ(calls, 1);
    EXPECT_NE(received_events & castle::io::fd::poll_read, 0);
    EXPECT_EQ(manager.pending_event_count(), 0U);

    // An empty queue is valid to process repeatedly.
    EXPECT_EQ(manager.process_events(processed), castle::status::ok);
    EXPECT_EQ(processed, 0U);

    manager.clear();
    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdPoll, CombinedWaitProcessAndCallbackCanReenterManager)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    castle::io::fd::basic_fd_event_manager<2U> manager;
    int calls = 0;
    ASSERT_EQ(manager.add_fd(descriptors[0], castle::io::fd::poll_read,
        [&manager, &calls](int fd, short, void*)
        {
            char value = 0;
            (void)::read(fd, &value, 1U);
            ++calls;
            EXPECT_EQ(manager.remove_fd(fd), castle::status::ok);
        }), castle::status::ok);

    const char value = 'r';
    ASSERT_EQ(::write(descriptors[1], &value, 1U), 1);
    castle::size_type processed = 0U;
    EXPECT_EQ(manager.wait_and_process(1000, processed), castle::status::ok);
    EXPECT_EQ(processed, 1U);
    EXPECT_EQ(calls, 1);
    EXPECT_FALSE(manager.has_fd(descriptors[0]));
    EXPECT_EQ(manager.get_fd_count(), 0U);

    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdPoll, ClearDropsQueuedEvents)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    castle::io::fd::basic_fd_event_manager<1U> manager;
    int calls = 0;
    ASSERT_EQ(manager.add_fd(descriptors[0], castle::io::fd::poll_read,
        [](int, short, void* context) { ++*static_cast<int*>(context); }, &calls), castle::status::ok);
    const char value = 'c';
    ASSERT_EQ(::write(descriptors[1], &value, 1U), 1);

    castle::size_type ready = 0U;
    ASSERT_EQ(manager.wait(1000, ready), castle::status::ok);
    EXPECT_EQ(ready, 1U);
    manager.clear();
    EXPECT_EQ(manager.pending_event_count(), 0U);
    castle::size_type processed = 0U;
    EXPECT_EQ(manager.process_events(processed), castle::status::ok);
    EXPECT_EQ(processed, 0U);
    EXPECT_EQ(calls, 0);

    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdPoll, QueuedCallbackSurvivesRegistrationRemoval)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    castle::io::fd::basic_fd_event_manager<1U> manager;
    int calls = 0;
    ASSERT_EQ(manager.add_fd(descriptors[0], castle::io::fd::poll_read,
        [](int, short, void* context) { ++*static_cast<int*>(context); }, &calls), castle::status::ok);

    const char value = 's';
    ASSERT_EQ(::write(descriptors[1], &value, 1U), 1);
    castle::size_type ready = 0U;
    ASSERT_EQ(manager.wait(1000, ready), castle::status::ok);
    ASSERT_EQ(ready, 1U);
    EXPECT_EQ(manager.remove_fd(descriptors[0]), castle::status::ok);

    castle::size_type processed = 0U;
    ASSERT_EQ(manager.process_events(processed), castle::status::ok);
    EXPECT_EQ(processed, 1U);
    EXPECT_EQ(calls, 1);

    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdPoll, ReportsInvalidDescriptorFromPoll)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    castle::io::fd::basic_fd_event_manager<1U> manager;
    int calls = 0;
    short received_events = 0;
    ASSERT_EQ(manager.add_fd(descriptors[0], castle::io::fd::poll_read,
        [&calls, &received_events](int, short revents, void*)
        {
            ++calls;
            received_events = revents;
        }), castle::status::ok);

    ASSERT_EQ(::close(descriptors[0]), 0);
    castle::size_type ready = 0U;
    ASSERT_EQ(manager.wait(1000, ready), castle::status::ok);
    EXPECT_EQ(ready, 1U);
    castle::size_type processed = 0U;
    ASSERT_EQ(manager.process_events(processed), castle::status::ok);
    EXPECT_EQ(processed, 1U);
    EXPECT_EQ(calls, 1);
    EXPECT_NE(received_events & castle::io::fd::poll_invalid, 0);

    manager.clear();
    (void)::close(descriptors[1]);
}

TEST(FdWaitFd, RejectsNegativeDescriptorAndClearsOutputs)
{
    short revents = 0x55;
    int system_error = 77;
    EXPECT_EQ(castle::io::fd::wait_fd(-1, castle::io::fd::poll_read, 0, revents, &system_error),
              castle::status::invalid_argument);
    EXPECT_EQ(revents, 0);
    EXPECT_EQ(system_error, 0);

    revents = 0x55;
    EXPECT_EQ(castle::io::fd::wait_fd(-5, castle::io::fd::poll_write, -1, revents), castle::status::invalid_argument);
    EXPECT_EQ(revents, 0);
}

TEST(FdWaitFd, ReportsTimeoutWhenNothingIsReady)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);

    short revents = 0x55;
    int system_error = 77;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 0, revents, &system_error),
              castle::status::time_out);
    EXPECT_EQ(revents, 0);
    EXPECT_EQ(system_error, 0);

    revents = 0x55;
    system_error = 77;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 10, revents, &system_error),
              castle::status::time_out);
    const castle::chrono::milliseconds elapsed =
        castle::chrono::duration_cast<castle::chrono::milliseconds>(castle::chrono::steady_clock::now() - start);
    EXPECT_GE(elapsed.count(), 9);
    EXPECT_LT(elapsed.count(), 2000);
    EXPECT_EQ(revents, 0);
    EXPECT_EQ(system_error, 0);

    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdWaitFd, ReportsReadableAfterWrite)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    const char byte = 'x';
    ASSERT_EQ(::write(descriptors[1], &byte, 1U), 1);

    short revents = 0;
    int system_error = 77;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 0, revents, &system_error),
              castle::status::ok);
    EXPECT_NE(revents & castle::io::fd::poll_read, 0);
    EXPECT_EQ(revents & castle::io::fd::poll_invalid, 0);
    EXPECT_EQ(system_error, 0);

    // A negative timeout waits indefinitely, but returns at once because data is already present.
    revents = 0;
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, -1, revents), castle::status::ok);
    const castle::chrono::milliseconds elapsed =
        castle::chrono::duration_cast<castle::chrono::milliseconds>(castle::chrono::steady_clock::now() - start);
    EXPECT_LT(elapsed.count(), 1000);
    EXPECT_NE(revents & castle::io::fd::poll_read, 0);

    // The data is still there: wait_fd does not consume it.
    char received = 0;
    EXPECT_EQ(::read(descriptors[0], &received, 1U), 1);
    EXPECT_EQ(received, 'x');
    revents = 0x55;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 0, revents),
              castle::status::time_out);
    EXPECT_EQ(revents, 0);

    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdWaitFd, ReportsWritableAndRequestedEventsOnly)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);

    short revents = 0;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[1], castle::io::fd::poll_write, 0, revents), castle::status::ok);
    EXPECT_NE(revents & castle::io::fd::poll_write, 0);
    EXPECT_EQ(revents & castle::io::fd::poll_read, 0);

    // Only the requested direction is reported: the read end of a pipe is never writable.
    revents = 0x55;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_write, 0, revents),
              castle::status::time_out);
    EXPECT_EQ(revents, 0);

    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

TEST(FdWaitFd, ReportsHangUpWhenWriteEndIsClosed)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    ASSERT_EQ(::close(descriptors[1]), 0);

    short revents = 0;
    int system_error = 77;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 100, revents, &system_error),
              castle::status::ok);
    EXPECT_NE(revents & castle::io::fd::poll_hangup, 0);
    EXPECT_EQ(system_error, 0);

    (void)::close(descriptors[0]);
}

TEST(FdWaitFd, ReportsClosedDescriptorNumberAsInvalid)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    ASSERT_EQ(::close(descriptors[0]), 0);

    short revents = 0;
    int system_error = 77;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 0, revents, &system_error),
              castle::status::ok);
    EXPECT_NE(revents & castle::io::fd::poll_invalid, 0);
    EXPECT_EQ(system_error, 0);

    (void)::close(descriptors[1]);
}

void ignore_signal(int)
{
}

TEST(FdWaitFd, ReportsInterruptedWaitAsSystemCallError)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);

    // A handler without SA_RESTART makes poll() fail with EINTR when the timer signal arrives.
    struct sigaction action;
    struct sigaction previous;
    ::sigemptyset(&action.sa_mask);
    action.sa_handler = ignore_signal;
    action.sa_flags = 0;
    ASSERT_EQ(::sigaction(SIGALRM, &action, &previous), 0);
    struct itimerval timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 10000;
    ASSERT_EQ(::setitimer(ITIMER_REAL, &timer, nullptr), 0);

    short revents = 0x55;
    int system_error = 0;
    EXPECT_EQ(castle::io::fd::wait_fd(descriptors[0], castle::io::fd::poll_read, 5000, revents, &system_error),
              castle::status::system_call_error);
    EXPECT_EQ(system_error, EINTR);
    EXPECT_EQ(revents, 0);

    ASSERT_EQ(::sigaction(SIGALRM, &previous, nullptr), 0);
    (void)::close(descriptors[0]);
    (void)::close(descriptors[1]);
}

#else
TEST(FdPoll, PollUnavailable)
{
    EXPECT_FALSE(castle::io::fd::poll_available);
    EXPECT_TRUE(castle::io::fd::poll_requires_posix);
}
#endif

} // namespace
