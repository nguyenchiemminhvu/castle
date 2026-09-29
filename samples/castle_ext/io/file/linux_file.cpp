#include <assert.h>

#include "castle_ext/io/file/linux_file.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <unistd.h>
#endif

int main()
{
#if CASTLE_USING_POSIX_APIS
    const char* path = "/tmp/castle_linux_file_sample.bin";
    (void)::unlink(path);

    castle::io::file::basic_file file;
    const castle::io::file::file_config cfg(path, true, true, true, true);
    assert(castle::io::file::validate_config(cfg) == castle::status::ok);
    assert(file.open(cfg) == castle::status::ok);
    assert(file.is_open() && file.native_handle() >= 0);

    const unsigned char tx[] = {0x43U, 0x41U, 0x53U, 0x54U};
    castle::size_type sent = 0U;
    assert(file.write_all(tx, sizeof(tx), sent) == castle::status::ok);
    assert(sent == sizeof(tx));

    int64_t file_size = 0;
    assert(file.size(file_size) == castle::status::ok);
    assert(file_size == static_cast<int64_t>(sizeof(tx)));
    int64_t position = -1;
    assert(file.seek(0, castle::io::file::seek_origin::begin, position) == castle::status::ok);
    assert(position == 0);

    unsigned char rx[sizeof(tx)] = {};
    castle::size_type received = 0U;
    assert(file.read_exact(rx, sizeof(rx), received) == castle::status::ok);
    assert(received == sizeof(rx) && rx[0] == 0x43U && rx[3] == 0x54U);
    assert(file.truncate(2) == castle::status::ok);
    assert(file.sync() == castle::status::ok);
    assert(file.close() == castle::status::ok);
    (void)::unlink(path);
#else
    assert(castle::io::file::validate_config(castle::io::file::file_config())
           == castle::status::invalid_argument);
#endif
    return 0;
}
