#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/backend.hpp"

int main()
{
    using castle::io::rpmsg::detail::has_backend_contract;
    static_assert(has_backend_contract<castle::io::rpmsg::mock::backend>::value, "mock contract");
    static_assert(!has_backend_contract<int>::value, "invalid contract");
    return 0;
}
