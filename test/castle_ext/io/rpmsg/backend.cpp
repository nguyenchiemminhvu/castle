#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/backend.hpp"

TEST(RpmsgBackendContract, DetectsStructuralBackend)
{
    EXPECT_TRUE((castle::io::rpmsg::detail::has_backend_contract<castle::io::rpmsg::mock::backend>::value));
    EXPECT_FALSE((castle::io::rpmsg::detail::has_backend_contract<int>::value));
    EXPECT_FALSE((castle::io::rpmsg::detail::has_backend_contract<void*>::value));
}

