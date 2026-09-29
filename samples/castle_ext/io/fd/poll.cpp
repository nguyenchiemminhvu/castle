#include "sample_support.hpp"

#include "castle_ext/io/fd/poll.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <unistd.h>
#endif

int main()
{
#if CASTLE_USING_POSIX_APIS
    int descriptors[2] = {-1, -1};
    CASTLE_SAMPLE_CHECK(::pipe(descriptors) == 0);

    int callback_count = 0;
    castle::io::fd::fd_event_manager manager;
    CASTLE_SAMPLE_CHECK(manager.add_fd(
        descriptors[0],
        castle::io::fd::poll_read,
        [](int fd, short revents, void* user_data)
        {
            if ((revents & castle::io::fd::poll_read) != 0)
            {
                char byte = 0;
                const ssize_t read_count = ::read(fd, &byte, 1U);
                if (read_count == 1)
                {
                    ++*static_cast<int*>(user_data);
                }
            }
        },
        &callback_count,
        "sample-pipe") == castle::status::ok);

    CASTLE_SAMPLE_CHECK(manager.get_fd_count() == 1U);
    CASTLE_SAMPLE_CHECK(manager.get_enabled_fd_count() == 1U);
    CASTLE_SAMPLE_CHECK(manager.has_fd(descriptors[0]));

    castle::io::fd::fd_event_manager::registration_info registration{};
    CASTLE_SAMPLE_CHECK(manager.get_fd_info(descriptors[0], registration) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(registration.fd == descriptors[0]);
    CASTLE_SAMPLE_CHECK(registration.events == castle::io::fd::poll_read);
    CASTLE_SAMPLE_CHECK(registration.user_data == &callback_count);
    CASTLE_SAMPLE_CHECK(registration.name != nullptr);
    CASTLE_SAMPLE_CHECK(registration.enabled);

    CASTLE_SAMPLE_CHECK(manager.disable_fd(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(manager.get_enabled_fd_count() == 0U);
    const char byte = 'a';
    CASTLE_SAMPLE_CHECK(::write(descriptors[1], &byte, 1U) == 1);
    castle::size_type ready = 0U;
    CASTLE_SAMPLE_CHECK(manager.wait(0, ready) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ready == 0U);

    CASTLE_SAMPLE_CHECK(manager.enable_fd(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(manager.modify_events(descriptors[0], static_cast<short>(castle::io::fd::poll_read | castle::io::fd::poll_priority)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(manager.modify_events(descriptors[0], castle::io::fd::poll_read) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(manager.wait(1000, ready) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ready == 1U);
    CASTLE_SAMPLE_CHECK(manager.pending_event_count() == 1U);
    castle::size_type processed = 0U;
    CASTLE_SAMPLE_CHECK(manager.process_events(processed) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(processed == 1U);
    CASTLE_SAMPLE_CHECK(callback_count == 1);
    CASTLE_SAMPLE_CHECK(manager.process_events() == castle::status::ok);

    const char second_byte = 'b';
    CASTLE_SAMPLE_CHECK(::write(descriptors[1], &second_byte, 1U) == 1);
    // The no-argument overload waits indefinitely and discards the count.
    CASTLE_SAMPLE_CHECK(manager.wait_and_process() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(callback_count == 2);

    CASTLE_SAMPLE_CHECK(manager.remove_fd(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!manager.has_fd(descriptors[0]));
    CASTLE_SAMPLE_CHECK(manager.get_fd_count() == 0U);
    manager.clear();

    CASTLE_SAMPLE_CHECK(castle::io::fd::close_fd(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::close_fd(descriptors[1]) == castle::status::ok);
#else
    CASTLE_SAMPLE_CHECK(!castle::io::fd::poll_available);
#endif
    return 0;
}
