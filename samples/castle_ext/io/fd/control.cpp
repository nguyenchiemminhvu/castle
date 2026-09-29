#include "sample_support.hpp"

#include "castle_ext/io/fd/control.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <fcntl.h>
    #include <unistd.h>
#endif

int main()
{
#if CASTLE_USING_POSIX_APIS
    int descriptors[2] = {-1, -1};
    CASTLE_SAMPLE_CHECK(::pipe(descriptors) == 0);

    CASTLE_SAMPLE_CHECK(castle::io::fd::is_valid_fd(descriptors[0]));
    CASTLE_SAMPLE_CHECK(castle::io::fd::is_valid_fd(descriptors[1]));

    int status_flags = 0;
    CASTLE_SAMPLE_CHECK(castle::io::fd::get_status_flags(descriptors[0], status_flags) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_status_flags(descriptors[0], status_flags | O_NONBLOCK) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_nonblocking(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::get_status_flags(descriptors[0], status_flags) == castle::status::ok);
    CASTLE_SAMPLE_CHECK((status_flags & O_NONBLOCK) != 0);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_blocking(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::get_status_flags(descriptors[0], status_flags) == castle::status::ok);
    CASTLE_SAMPLE_CHECK((status_flags & O_NONBLOCK) == 0);

    int descriptor_flags = 0;
    CASTLE_SAMPLE_CHECK(castle::io::fd::get_descriptor_flags(descriptors[0], descriptor_flags) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_descriptor_flags(descriptors[0], descriptor_flags | FD_CLOEXEC) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_close_on_exec(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::get_descriptor_flags(descriptors[0], descriptor_flags) == castle::status::ok);
    CASTLE_SAMPLE_CHECK((descriptor_flags & FD_CLOEXEC) != 0);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_close_on_exec(descriptors[0], false) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::io::fd::get_descriptor_flags(descriptors[0], descriptor_flags) == castle::status::ok);
    CASTLE_SAMPLE_CHECK((descriptor_flags & FD_CLOEXEC) == 0);

    CASTLE_SAMPLE_CHECK(castle::io::fd::close_fd(descriptors[0]) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!castle::io::fd::is_valid_fd(descriptors[0]));
    CASTLE_SAMPLE_CHECK(castle::io::fd::close_fd(descriptors[1]) == castle::status::ok);
#else
    CASTLE_SAMPLE_CHECK(!castle::io::fd::posix_available);
    CASTLE_SAMPLE_CHECK(castle::io::fd::set_nonblocking(3) == castle::status::not_configured);
#endif
    return 0;
}
