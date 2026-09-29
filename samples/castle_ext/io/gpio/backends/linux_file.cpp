#include <assert.h>

#include "castle/core/config.hpp"

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    #define CASTLE_EXT_ENABLE_GPIO_LINUX_FILE
    #include <fcntl.h>
    #include <unistd.h>
#endif

#include "castle_ext/io/gpio/backends/linux_file.hpp"

int main()
{
#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    using namespace castle::io::gpio;
    const char* path = "/tmp/castle_gpio_value_sample";
    const int descriptor = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    assert(descriptor >= 0);
    assert(::write(descriptor, "0\n", 2U) == 2);
    assert(::close(descriptor) == 0);

    const linux_file::pin_handle handle(path, true);
    level value = level::low;
    assert(linux_file::backend::configure(handle, pin_config()) == castle::status::not_configured);
    assert(linux_file::backend::read(handle, value) == castle::status::ok);
    assert(value == level::high);
    assert(linux_file::backend::write(handle, level::low) == castle::status::ok);
    assert(linux_file::backend::read_raw(handle, value) == castle::status::ok);
    assert(value == level::high);
    assert(::unlink(path) == 0);
#else
    assert(true); // This backend is intentionally disabled outside POSIX Linux builds.
#endif
    return 0;
}
