#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/backends/openamp.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

int main()
{
    static_assert(castle::io::rpmsg::default_buffer_size == 512U, "OpenAMP-compatible default");
#if defined(CASTLE_RPMSG_ENABLE_OPENAMP)
    // Platform-specific resources are supplied by the owning application.
#endif
    return 0;
}
