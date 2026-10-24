#include <gtest/gtest.h>
#include "castle_ext/comm/http/http.hpp"

TEST(HttpUmbrella, ExposesCoreTypesAndHeaderCollection)
{
    castle::comm::http::basic_headers<4U, 32U, 64U> headers;
    EXPECT_EQ(headers.add("accept", "*/*"), castle::status::ok);
    EXPECT_TRUE(headers.contains("Accept"));
    EXPECT_EQ(castle::comm::http::method_token(castle::comm::http::method::options).size(), 7U);
    castle::comm::http::http2_session<1U, 128U, 16384U, 32U> session;
    EXPECT_EQ(session.start(), castle::status::ok);
}
