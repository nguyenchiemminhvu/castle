#include <assert.h>

#include "castle_ext/io/fd/fd.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <fcntl.h>
    #include <unistd.h>
#endif

int main()
{
    assert(castle::io::fd::posix_available == castle::io::fd::poll_available);
#if CASTLE_USING_POSIX_APIS
    int descriptors[2] = {-1, -1};
    assert(::pipe(descriptors) == 0);

    castle::io::fd::basic_fd reader(descriptors[0]);
    castle::io::fd::basic_fd writer(descriptors[1]);
    assert(reader.set_nonblocking(true) == castle::status::ok);

    castle::io::fd::fd_event_manager manager;
    int callbacks = 0;
    assert(manager.add_fd(reader.native_handle(), castle::io::fd::poll_read,
        [](int fd, short revents, void* context)
        {
            if ((revents & castle::io::fd::poll_read) != 0)
            {
                char value = 0;
                const ssize_t count = ::read(fd, &value, 1U);
                if (count == 1 && value == 'z')
                {
                    ++*static_cast<int*>(context);
                }
            }
        }, &callbacks) == castle::status::ok);

    const char value = 'z';
    castle::size_type sent = 0U;
    assert(writer.write_some(&value, 1U, sent) == castle::status::ok);
    assert(sent == 1U);
    castle::size_type processed = 0U;
    assert(manager.wait_and_process(1000, processed) == castle::status::ok);
    assert(processed == 1U && callbacks == 1);
    assert(manager.remove_fd(reader.native_handle()) == castle::status::ok);
    assert(writer.close() == castle::status::ok);
    assert(reader.close() == castle::status::ok);
#else
    assert(castle::io::fd::set_nonblocking(3) == castle::status::not_configured);
#endif
    return 0;
}
