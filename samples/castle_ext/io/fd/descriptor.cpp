#include <assert.h>

#include "castle_ext/io/fd/descriptor.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <fcntl.h>
    #include <unistd.h>
#endif

int main()
{
#if CASTLE_USING_POSIX_APIS
    int pipe_descriptors[2] = {-1, -1};
    assert(::pipe(pipe_descriptors) == 0);

    castle::io::fd::basic_fd reader(pipe_descriptors[0]);
    castle::io::fd::basic_fd writer(pipe_descriptors[1]);
    assert(reader.is_open() && writer.is_open());

    const char tx[] = {'C', 'A', 'S'};
    castle::size_type written = 0U;
    assert(writer.write_all(tx, sizeof(tx), written) == castle::status::ok);
    assert(written == sizeof(tx));

    char rx[sizeof(tx)] = {};
    castle::size_type received = 0U;
    assert(reader.read_exact(rx, sizeof(rx), received) == castle::status::ok);
    assert(received == sizeof(rx));
    assert(rx[0] == 'C' && rx[1] == 'A' && rx[2] == 'S');

    assert(writer.close() == castle::status::ok);
    assert(reader.close() == castle::status::ok);

    castle::io::fd::basic_fd null_device;
    assert(null_device.open("/dev/null", O_RDWR) == castle::status::ok);
    assert(null_device.is_open());
    const int released_descriptor = null_device.release();
    assert(released_descriptor >= 0);
    assert(castle::io::fd::close_fd(released_descriptor) == castle::status::ok);

    int raw_descriptor = -1;
    assert(castle::io::fd::open_fd("/dev/zero", O_RDONLY, raw_descriptor) == castle::status::ok);
    unsigned char raw_bytes[2] = {};
    castle::size_type raw_count = 0U;
    assert(castle::io::fd::read_exact_fd(raw_descriptor, raw_bytes, sizeof(raw_bytes), raw_count)
           == castle::status::ok);
    assert(raw_count == sizeof(raw_bytes));
    assert(raw_bytes[0] == 0U && raw_bytes[1] == 0U);
    assert(castle::io::fd::close_fd(raw_descriptor) == castle::status::ok);
#else
    assert(!castle::io::fd::descriptor_available);
#endif
    return 0;
}
