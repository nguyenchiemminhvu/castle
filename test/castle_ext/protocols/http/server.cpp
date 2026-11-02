#include <gtest/gtest.h>

#include "castle_ext/net/client.hpp"
#include "castle_ext/protocols/http/server.hpp"

#include <atomic>
#include <chrono>
#include <stdint.h>
#include <string>
#include <thread>

namespace
{
using namespace castle;
using namespace castle::protocols::http;

namespace http_detail = castle::protocols::http::detail;

using test_server = basic_http_server<2U, 512U, 4U, 16U, 8U, 32U, 64U,
                                      128U, 64U, 512U, 256U, 64U, 4U, 64U>;

void append(char* destination, size_type capacity, size_type& used, string_view value)
{
    if (value.size() > capacity - used)
    {
        return;
    }
    for (size_type i = 0U; i < value.size(); ++i)
    {
        destination[used++] = value[i];
    }
}

size_t count_occurrences(std::string const& text, std::string const& value)
{
    size_t count = 0U;
    for (size_t pos = text.find(value); pos != std::string::npos; pos = text.find(value, pos + value.size()))
    {
        ++count;
    }
    return count;
}

std::string repeated_trailers(int count)
{
    std::string trailers;
    for (int i = 0; i < count; ++i)
    {
        trailers += "T" + std::to_string(i) + ": v\r\n";
    }
    return trailers;
}

class HttpServerLoopbackTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        server_.register_handler(HttpMethod::GET, "/health",
            [](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(response.set_body("ok"));
            });
        server_.register_handler(HttpMethod::GET, "/devices/{id}/state",
            [](test_server::request_type const& request, test_server::response_type& response)
            {
                char body[64U]{};
                size_type used = 0U;
                append(body, sizeof(body), used, request.path_param("id"));
                append(body, sizeof(body), used, string_view(":"));
                append(body, sizeof(body), used, request.query_param("refresh"));
                static_cast<void>(response.set_body(string_view(body, used)));
            });
        server_.register_handler(HttpMethod::POST, "/echo",
            [](test_server::request_type const& request, test_server::response_type& response)
            {
                static_cast<void>(response.set_body(request.body()));
            });
        server_.register_handler(HttpMethod::GET, "/trailing/",
            [](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(response.set_body("trailing"));
            });
        server_.register_handler(HttpMethod::GET, "/only-get",
            [](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(response.set_body("get"));
            });
        server_.register_prefix_handler(HttpMethod::GET, "/api",
            [](test_server::request_type const& request, test_server::response_type& response)
            {
                static_cast<void>(response.set_body(request.path()));
            });
        server_.register_handler("/any",
            [](test_server::request_type const& request, test_server::response_type& response)
            {
                static_cast<void>(response.set_body(request.method_text()));
            });
        server_.register_handler(HttpMethod::GET, "/pair/{a}/{b}",
            [](test_server::request_type const& request, test_server::response_type& response)
            {
                char body[64U]{};
                size_type used = 0U;
                append(body, sizeof(body), used, request.path_param("a"));
                append(body, sizeof(body), used, string_view("|"));
                append(body, sizeof(body), used, request.path_param("b"));
                append(body, sizeof(body), used, string_view("|"));
                append(body, sizeof(body), used, request.path_param("missing"));
                static_cast<void>(response.set_body(string_view(body, used)));
            });
        server_.register_handler(HttpMethod::GET, "/q",
            [](test_server::request_type const& request, test_server::response_type& response)
            {
                char body[64U]{};
                size_type used = 0U;
                append(body, sizeof(body), used, request.query_param("flag"));
                append(body, sizeof(body), used, string_view("|"));
                append(body, sizeof(body), used, request.query_param("x"));
                append(body, sizeof(body), used, string_view("|"));
                append(body, sizeof(body), used, request.query_param("zzz"));
                static_cast<void>(response.set_body(string_view(body, used)));
            });
        server_.register_handler(HttpMethod::GET, "/nocontent",
            [](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(response.set_status(status_code::no_content));
            });
        server_.register_handler(HttpMethod::GET, "/notmodified",
            [](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(response.set_status(status_code::not_modified));
            });
        server_.register_handler(HttpMethod::GET, "/reenter",
            [this](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(server_.poll(0));
                static_cast<void>(response.set_body("reentered"));
            });
        server_.set_not_found_handler(
            [](test_server::request_type const&, test_server::response_type& response)
            {
                static_cast<void>(response.set_status(status_code::not_found));
                static_cast<void>(response.set_body("fallback-404"));
            });

        ASSERT_EQ(server_.last_registration_status(), status::ok);
        ASSERT_EQ(server_.start(net::endpoint::loopback_ipv4(0U)), status::ok);
        ASSERT_EQ(server_.local_endpoint(local_), status::ok);
        running_.store(true);
        worker_ = std::thread([this]()
        {
            while (running_.load())
            {
                static_cast<void>(server_.poll(1));
                std::this_thread::yield();
            }
        });
    }

    void TearDown() override
    {
        running_.store(false);
        if (worker_.joinable())
        {
            worker_.join();
        }
        if (server_.is_running())
        {
            EXPECT_EQ(server_.stop(), status::ok);
        }
    }

    std::string exchange(std::string const& request)
    {
        net::tcp_client<512U> client;
        status connected = client.connect(local_, 1000, false);
        EXPECT_EQ(connected, status::ok);
        if (connected != status::ok)
        {
            return std::string();
        }

        size_type sent = 0U;
        status sent_status = client.send_all(request.data(), request.size(), sent);
        EXPECT_EQ(sent_status, status::ok);
        EXPECT_EQ(sent, request.size());

        std::string response;
        char buffer[512U]{};
        bool peer_closed = false;
        for (size_type i = 0U; i < 16U && !peer_closed; ++i)
        {
            size_type received = 0U;
            status received_status = client.receive_some(buffer, sizeof(buffer), received, nullptr, &peer_closed);
            if (received > 0U)
            {
                response.append(buffer, received);
            }
            if (received_status != status::ok && received_status != status::not_configured)
            {
                break;
            }
        }
        static_cast<void>(client.close());
        return response;
    }

    static bool contains(std::string const& text, std::string const& value)
    {
        return text.find(value) != std::string::npos;
    }

    void stop_worker()
    {
        running_.store(false);
        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    void pump(int rounds = 20)
    {
        for (int i = 0; i < rounds; ++i)
        {
            static_cast<void>(server_.poll(0));
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void send_text(net::tcp_client<512U>& client, std::string const& text)
    {
        size_type sent = 0U;
        ASSERT_EQ(client.send_all(text.data(), text.size(), sent), status::ok);
        ASSERT_EQ(sent, text.size());
    }

    std::string receive_response(net::tcp_client<512U>& client, std::string const& marker = std::string())
    {
        std::string response;
        char buffer[512U]{};
        bool peer_closed = false;
        for (size_type i = 0U; i < 64U && !peer_closed && (marker.empty() || !contains(response, marker)); ++i)
        {
            size_type received = 0U;
            status received_status = client.receive_some(buffer, sizeof(buffer), received, nullptr, &peer_closed);
            if (received > 0U)
            {
                response.append(buffer, received);
            }
            if (received_status != status::ok && received_status != status::not_configured)
            {
                break;
            }
        }
        return response;
    }

    std::string pumped_exchange(std::string const& request)
    {
        net::tcp_client<512U> client;
        EXPECT_EQ(client.connect(local_, 1000, false), status::ok);
        size_type sent = 0U;
        EXPECT_EQ(client.send_all(request.data(), request.size(), sent), status::ok);
        pump();
        std::string response = receive_response(client);
        static_cast<void>(client.close());
        return response;
    }

    test_server server_;
    net::endpoint local_;
    std::atomic<bool> running_{false};
    std::thread worker_;
};

TEST(HttpServerModel, RequestResponseHelpersRespectFixedCapacity)
{
    basic_http_request<4U, 32U, 64U, 64U, 16U, 2U> request;
    request.clear();
    EXPECT_TRUE(request.path().empty());
    EXPECT_TRUE(request.query().empty());

    basic_http_response<2U, 32U, 64U, 8U> response;
    EXPECT_EQ(response.status(), 200U);
    EXPECT_EQ(response.set_status(status_code::created), status::ok);
    EXPECT_EQ(response.status(), 201U);
    EXPECT_EQ(response.set_header("Content-Type", "application/json"), status::ok);
    EXPECT_EQ(response.set_json("{}"), status::ok);
    EXPECT_EQ(response.set_body("123456789"), status::out_of_range);
    EXPECT_EQ(response.set_file("text/plain", "resource"), status::ok);
    EXPECT_EQ(response.body(), string_view("resource"));
    EXPECT_EQ(response.set_status(99U), status::invalid_argument);
}

TEST_F(HttpServerLoopbackTest, DispatchesPathAndQueryParameters)
{
    std::string response = exchange(
        "GET /devices/42/state?refresh=1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "Content-Length: 4\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\n42:1"));
}

TEST_F(HttpServerLoopbackTest, DecodesChunkedRequestBody)
{
    std::string response = exchange(
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n\r\n"
        "4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "Content-Length: 9\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\nWikipedia"));
}

TEST_F(HttpServerLoopbackTest, HandlesPrefixRoutesAndPersistentPipelining)
{
    std::string response = exchange(
        "GET /api/first HTTP/1.1\r\nHost: localhost\r\n\r\n"
        "GET /api/second HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\n/api/first"));
    EXPECT_TRUE(contains(response, "/api/second"));
    EXPECT_NE(response.find("HTTP/1.1 200 OK", response.find("HTTP/1.1 200 OK") + 1U), std::string::npos);
}

TEST_F(HttpServerLoopbackTest, ReturnsMethodNotAllowedAndNotFound)
{
    std::string wrong_method = exchange(
        "POST /only-get HTTP/1.1\r\nHost: localhost\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(wrong_method, "HTTP/1.1 405 Method Not Allowed\r\n"));

    std::string missing = exchange(
        "GET /missing HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(missing, "HTTP/1.1 404 Not Found\r\n"));
    EXPECT_TRUE(contains(missing, "fallback-404"));
}

TEST_F(HttpServerLoopbackTest, RejectsConflictingContentLengths)
{
    std::string response = exchange(
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 1\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n\r\n"
        "x");
    EXPECT_TRUE(contains(response, "HTTP/1.1 400 Bad Request\r\n"));
}

TEST_F(HttpServerLoopbackTest, RejectsBodyBeyondConfiguredCapacity)
{
    std::string response = exchange(
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 65\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 413 Payload Too Large\r\n"));
}

TEST_F(HttpServerLoopbackTest, NormalizesAbsoluteFormRequestTargets)
{
    std::string response = exchange(
        "GET http://localhost/devices/42/state?refresh=1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\n42:1"));
}

TEST_F(HttpServerLoopbackTest, RejectsUnsupportedTransferCoding)
{
    std::string response = exchange(
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: gzip\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 501 Not Implemented\r\n"));
}

TEST_F(HttpServerLoopbackTest, RejectsUnsupportedHttpVersion)
{
    std::string response = exchange(
        "GET /health HTTP/2.0\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 505 HTTP Version Not Supported\r\n"));
}

TEST_F(HttpServerLoopbackTest, ReturnsNotImplementedForUnknownMethod)
{
    std::string response = exchange(
        "BREW /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 501 Not Implemented\r\n"));
}

TEST_F(HttpServerLoopbackTest, RejectsHeaderCountBeyondCapacity)
{
    std::string response = exchange(
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-One: 1\r\n"
        "X-Two: 2\r\n"
        "X-Three: 3\r\n"
        "X-Four: 4\r\n"
        "X-Five: 5\r\n"
        "X-Six: 6\r\n"
        "X-Seven: 7\r\n"
        "X-Eight: 8\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 431 Request Header Fields Too Large\r\n"));
}

TEST_F(HttpServerLoopbackTest, MatchesExplicitTrailingSlashRoutes)
{
    std::string response = exchange(
        "GET /trailing/ HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\ntrailing"));
}

TEST_F(HttpServerLoopbackTest, HandlesServerWideOptionsAsterisk)
{
    std::string response = exchange(
        "OPTIONS * HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "Allow: GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH\r\n"));
}

TEST(HttpServerModel, RejectsUnroutablePatternsAndRouteOverflow)
{
    using tiny_server = basic_http_server<1U, 64U, 1U, 1U, 2U, 16U, 32U,
                                          32U, 32U, 128U, 64U, 32U, 1U, 64U>;
    tiny_server server;
    server.register_handler(HttpMethod::GET, "/items/{id}/{state}",
        [](tiny_server::request_type const&, tiny_server::response_type&) {});
    EXPECT_EQ(server.last_registration_status(), status::invalid_argument);

    server.register_handler(HttpMethod::GET, "/items",
        [](tiny_server::request_type const&, tiny_server::response_type&) {});
    EXPECT_EQ(server.last_registration_status(), status::ok);
    server.register_handler(HttpMethod::GET, "/other",
        [](tiny_server::request_type const&, tiny_server::response_type&) {});
    EXPECT_EQ(server.last_registration_status(), status::full);
}

TEST_F(HttpServerLoopbackTest, SendsContinueBeforeExpectedRequestBody)
{
    std::string response = exchange(
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Expect: 100-continue\r\n"
        "Content-Length: 4\r\n"
        "Connection: close\r\n\r\n"
        "data");
    EXPECT_TRUE(contains(response, "HTTP/1.1 100 Continue\r\n\r\n"));
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\ndata"));
}

TEST(HttpServerModel, ToMethodMapsEveryEnumerator)
{
    EXPECT_EQ(to_method(HttpMethod::GET), method::get);
    EXPECT_EQ(to_method(HttpMethod::POST), method::post);
    EXPECT_EQ(to_method(HttpMethod::PUT), method::put);
    EXPECT_EQ(to_method(HttpMethod::DELETE), method::delete_);
    EXPECT_EQ(to_method(HttpMethod::HEAD), method::head);
    EXPECT_EQ(to_method(HttpMethod::OPTIONS), method::options);
    EXPECT_EQ(to_method(HttpMethod::PATCH), method::patch);
    EXPECT_EQ(to_method(static_cast<HttpMethod>(99U)), method::custom);
}

TEST(HttpServerModel, ParseMethodRecognizesEveryToken)
{
    EXPECT_EQ(http_detail::parse_method(string_view("GET")), method::get);
    EXPECT_EQ(http_detail::parse_method(string_view("POST")), method::post);
    EXPECT_EQ(http_detail::parse_method(string_view("PUT")), method::put);
    EXPECT_EQ(http_detail::parse_method(string_view("DELETE")), method::delete_);
    EXPECT_EQ(http_detail::parse_method(string_view("HEAD")), method::head);
    EXPECT_EQ(http_detail::parse_method(string_view("OPTIONS")), method::options);
    EXPECT_EQ(http_detail::parse_method(string_view("PATCH")), method::patch);
    EXPECT_EQ(http_detail::parse_method(string_view("BREW")), method::custom);
}

TEST(HttpServerModel, HttpCopyRejectsNullZeroCapacityAndOverflow)
{
    char buffer[4]{};
    EXPECT_FALSE(http_detail::http_copy(nullptr, sizeof(buffer), string_view("a")));
    EXPECT_FALSE(http_detail::http_copy(buffer, 0U, string_view("a")));
    EXPECT_FALSE(http_detail::http_copy(buffer, 2U, string_view("abc")));
    EXPECT_TRUE(http_detail::http_copy(buffer, sizeof(buffer), string_view("abc")));
    EXPECT_EQ(string_view(buffer), string_view("abc"));
}

TEST(HttpServerModel, TrimOwsRemovesSurroundingWhitespace)
{
    EXPECT_EQ(http_detail::server_trim_ows(string_view("")), string_view(""));
    EXPECT_EQ(http_detail::server_trim_ows(string_view(" \t ")), string_view(""));
    EXPECT_EQ(http_detail::server_trim_ows(string_view("\t value \t")), string_view("value"));
    EXPECT_EQ(http_detail::server_trim_ows(string_view("value")), string_view("value"));
}

TEST(HttpServerModel, ContainsTokenMatchesCommaSeparatedValues)
{
    EXPECT_FALSE(http_detail::server_contains_token(string_view(""), string_view("close")));
    EXPECT_TRUE(http_detail::server_contains_token(string_view("close"), string_view("close")));
    EXPECT_TRUE(http_detail::server_contains_token(string_view("keep-alive , CLOSE"), string_view("close")));
    EXPECT_TRUE(http_detail::server_contains_token(string_view("a,close"), string_view("close")));
    EXPECT_FALSE(http_detail::server_contains_token(string_view("a,b"), string_view("close")));
    EXPECT_FALSE(http_detail::server_contains_token(string_view("closed"), string_view("close")));
}

TEST(HttpServerModel, ParseHexHandlesCaseExtensionsAndOverflow)
{
    uint64_t value = 0U;
    EXPECT_FALSE(http_detail::server_parse_hex(string_view(""), value));
    EXPECT_FALSE(http_detail::server_parse_hex(string_view(";ext"), value));
    EXPECT_TRUE(http_detail::server_parse_hex(string_view("1a"), value));
    EXPECT_EQ(value, 26U);
    EXPECT_TRUE(http_detail::server_parse_hex(string_view("FF"), value));
    EXPECT_EQ(value, 255U);
    EXPECT_TRUE(http_detail::server_parse_hex(string_view("3;name=v"), value));
    EXPECT_EQ(value, 3U);
    EXPECT_FALSE(http_detail::server_parse_hex(string_view("1g"), value));
    EXPECT_FALSE(http_detail::server_parse_hex(string_view("10000000000000000"), value));
    EXPECT_TRUE(http_detail::server_parse_hex(string_view("FFFFFFFFFFFFFFFF"), value));
    EXPECT_EQ(value, static_cast<uint64_t>(-1));
}

TEST(HttpServerModel, ReasonPhraseAndFormatUintCoverEveryBranch)
{
    EXPECT_EQ(http_detail::reason_phrase(100U), string_view("Continue"));
    EXPECT_EQ(http_detail::reason_phrase(101U), string_view("Switching Protocols"));
    EXPECT_EQ(http_detail::reason_phrase(200U), string_view("OK"));
    EXPECT_EQ(http_detail::reason_phrase(201U), string_view("Created"));
    EXPECT_EQ(http_detail::reason_phrase(202U), string_view("Accepted"));
    EXPECT_EQ(http_detail::reason_phrase(204U), string_view("No Content"));
    EXPECT_EQ(http_detail::reason_phrase(301U), string_view("Moved Permanently"));
    EXPECT_EQ(http_detail::reason_phrase(302U), string_view("Found"));
    EXPECT_EQ(http_detail::reason_phrase(304U), string_view("Not Modified"));
    EXPECT_EQ(http_detail::reason_phrase(400U), string_view("Bad Request"));
    EXPECT_EQ(http_detail::reason_phrase(401U), string_view("Unauthorized"));
    EXPECT_EQ(http_detail::reason_phrase(403U), string_view("Forbidden"));
    EXPECT_EQ(http_detail::reason_phrase(404U), string_view("Not Found"));
    EXPECT_EQ(http_detail::reason_phrase(405U), string_view("Method Not Allowed"));
    EXPECT_EQ(http_detail::reason_phrase(408U), string_view("Request Timeout"));
    EXPECT_EQ(http_detail::reason_phrase(413U), string_view("Payload Too Large"));
    EXPECT_EQ(http_detail::reason_phrase(414U), string_view("URI Too Long"));
    EXPECT_EQ(http_detail::reason_phrase(415U), string_view("Unsupported Media Type"));
    EXPECT_EQ(http_detail::reason_phrase(417U), string_view("Expectation Failed"));
    EXPECT_EQ(http_detail::reason_phrase(429U), string_view("Too Many Requests"));
    EXPECT_EQ(http_detail::reason_phrase(431U), string_view("Request Header Fields Too Large"));
    EXPECT_EQ(http_detail::reason_phrase(500U), string_view("Internal Server Error"));
    EXPECT_EQ(http_detail::reason_phrase(501U), string_view("Not Implemented"));
    EXPECT_EQ(http_detail::reason_phrase(502U), string_view("Bad Gateway"));
    EXPECT_EQ(http_detail::reason_phrase(503U), string_view("Service Unavailable"));
    EXPECT_EQ(http_detail::reason_phrase(504U), string_view("Gateway Timeout"));
    EXPECT_EQ(http_detail::reason_phrase(505U), string_view("HTTP Version Not Supported"));
    EXPECT_EQ(http_detail::reason_phrase(999U), string_view("Unknown"));

    char digits[12]{};
    EXPECT_EQ(http_detail::format_uint(0U, digits, sizeof(digits)), 1U);
    EXPECT_EQ(http_detail::format_uint(4294967295U, digits, sizeof(digits)), 10U);
    EXPECT_EQ(string_view(digits, 10U), string_view("4294967295"));
    EXPECT_EQ(http_detail::format_uint(123U, digits, 2U), 0U);
}

TEST(HttpServerModel, ResponseRejectsOutOfRangeStatusAndFullHeaders)
{
    basic_http_response<1U, 32U, 64U, 8U> response;
    EXPECT_EQ(response.set_status(1000U), status::invalid_argument);
    EXPECT_EQ(response.set_status(99U), status::invalid_argument);
    EXPECT_EQ(response.set_header("X-Full", "1"), status::ok);
    EXPECT_EQ(response.set_json("{}"), status::full);
    EXPECT_EQ(response.set_file("text/plain", "x"), status::full);
}

TEST(HttpServerModel, DefaultRequestAccessorsReturnEmptyViews)
{
    HttpRequest request;
    EXPECT_TRUE(request.path_param("id").empty());
    EXPECT_TRUE(request.query_param("x").empty());
    EXPECT_TRUE(request.query().empty());
}

TEST(HttpServerModel, RouteRegistrationRejectsInvalidRoutes)
{
    test_server server;
    auto handler = [](test_server::request_type const&, test_server::response_type&) {};

    server.register_handler(HttpMethod::GET, "/x", nullptr);
    EXPECT_EQ(server.last_registration_status(), status::invalid_callback);

    char const* invalid_paths[] = {"", "health", "/a/{", "/a/{}", "/a/{bad name}", "/a/b}"};
    for (char const* path : invalid_paths)
    {
        server.register_handler(HttpMethod::GET, path, handler);
        EXPECT_EQ(server.last_registration_status(), status::invalid_argument) << path;
    }

    std::string const too_long = "/" + std::string(64U, 'a');
    server.register_handler(HttpMethod::GET, string_view(too_long.data(), too_long.size()), handler);
    EXPECT_EQ(server.last_registration_status(), status::invalid_argument);
    EXPECT_EQ(server.route_count(), 0U);
}

TEST(HttpServerModel, RouteRegistrationRejectsPathsBeyondUriCapacity)
{
    using wide_server = basic_http_server<1U, 64U, 1U, 4U, 2U, 16U, 32U, 16U, 32U, 128U, 64U, 32U, 1U, 64U>;
    wide_server server;
    auto handler = [](wide_server::request_type const&, wide_server::response_type&) {};

    server.register_handler(HttpMethod::GET, "/this/path/is/long", handler);
    EXPECT_EQ(server.last_registration_status(), status::invalid_argument);
    server.register_handler(HttpMethod::GET, "/short/one", handler);
    EXPECT_EQ(server.last_registration_status(), status::ok);
}

TEST(HttpServerModel, SetNotFoundHandlerRejectsEmptyCallback)
{
    test_server server;
    server.set_not_found_handler(nullptr);
    EXPECT_EQ(server.last_registration_status(), status::invalid_callback);
}

TEST(HttpServerModel, LifecycleRejectsCallsOutsideRunningState)
{
    test_server server;
    castle::net::endpoint local;
    EXPECT_EQ(server.local_endpoint(local), status::not_configured);
    EXPECT_EQ(server.poll(0), status::not_configured);
    EXPECT_EQ(server.stop(), status::not_configured);
    ASSERT_EQ(server.start(net::endpoint::loopback_ipv4(0U)), status::ok);
    EXPECT_EQ(server.start(net::endpoint::loopback_ipv4(0U)), status::invalid_config);
    EXPECT_EQ(server.stop(), status::ok);
}

TEST(HttpServerModel, StartFailsWhenEndpointIsAlreadyBound)
{
    test_server first;
    ASSERT_EQ(first.start(net::endpoint::loopback_ipv4(0U)), status::ok);
    castle::net::endpoint bound;
    ASSERT_EQ(first.local_endpoint(bound), status::ok);

    test_server second;
    EXPECT_NE(second.start(bound), status::ok);
    EXPECT_FALSE(second.is_running());
    EXPECT_EQ(first.stop(), status::ok);
}

TEST(HttpServerModel, DestroyingRunningServerReleasesListener)
{
    castle::net::endpoint bound;
    {
        test_server server;
        ASSERT_EQ(server.start(net::endpoint::loopback_ipv4(0U)), status::ok);
        ASSERT_EQ(server.local_endpoint(bound), status::ok);
    }
    test_server again;
    EXPECT_EQ(again.start(bound), status::ok);
    EXPECT_EQ(again.stop(), status::ok);
}

TEST_F(HttpServerLoopbackTest, MatchesAnyRecognizedMethodAndOmitsHeadBody)
{
    EXPECT_TRUE(contains(exchange("PUT /any HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nPUT"));
    EXPECT_TRUE(contains(exchange("DELETE /any HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nDELETE"));
    EXPECT_TRUE(contains(exchange("OPTIONS /any HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nOPTIONS"));
    EXPECT_TRUE(contains(exchange("PATCH /any HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nPATCH"));

    std::string head = exchange("HEAD /any HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(head, "HTTP/1.1 200 OK\r\n"));
    EXPECT_FALSE(contains(head, "Content-Length"));

    EXPECT_TRUE(contains(exchange("PUT /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "HTTP/1.1 405 Method Not Allowed\r\n"));
    EXPECT_TRUE(contains(exchange("POST /api/items HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "HTTP/1.1 405 Method Not Allowed\r\n"));
}

TEST_F(HttpServerLoopbackTest, ExtractsPathParametersAndReportsMissingSegments)
{
    std::string response = exchange("GET /pair/1/2 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "\r\n\r\n1|2|"));

    std::string short_path = exchange("GET /pair/1 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(short_path, "HTTP/1.1 404 Not Found\r\n"));
    EXPECT_TRUE(contains(short_path, "fallback-404"));
}

TEST_F(HttpServerLoopbackTest, ParsesQueryFlagsDelimitersAndMissingKeys)
{
    EXPECT_TRUE(contains(exchange("GET /q?flag&x=1&y=2 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "\r\n\r\n|1|"));
    EXPECT_TRUE(contains(exchange("GET /q HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "\r\n\r\n||"));
    EXPECT_TRUE(contains(exchange("GET /q?flag HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "\r\n\r\n||"));
}

TEST_F(HttpServerLoopbackTest, OmitsBodyAndLengthForBodylessStatuses)
{
    std::string no_content = exchange("GET /nocontent HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(no_content, "HTTP/1.1 204 No Content\r\n"));
    EXPECT_FALSE(contains(no_content, "Content-Length"));

    std::string not_modified = exchange("GET /notmodified HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(not_modified, "HTTP/1.1 304 Not Modified\r\n"));
    EXPECT_FALSE(contains(not_modified, "Content-Length"));
}

TEST_F(HttpServerLoopbackTest, HandlerReentrantPollDropsPeerAndResponse)
{
    std::string response = exchange("GET /reenter HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_FALSE(contains(response, "reentered"));
}

TEST_F(HttpServerLoopbackTest, ClosesHttp10ConnectionsUnlessKeepAliveIsRequested)
{
    std::string plain = exchange("GET /health HTTP/1.0\r\n\r\n");
    EXPECT_TRUE(contains(plain, "HTTP/1.0 200 OK\r\n"));
    EXPECT_TRUE(contains(plain, "Connection: close\r\n"));

    std::string combined = exchange("GET /health HTTP/1.0\r\nConnection: keep-alive, close\r\n\r\n");
    EXPECT_TRUE(contains(combined, "Connection: close\r\n"));
}

TEST_F(HttpServerLoopbackTest, KeepsConnectionsOpenForKeepAliveRequests)
{
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);

    send_text(client, "GET /health HTTP/1.0\r\nConnection: keep-alive\r\n\r\n");
    std::string first = receive_response(client, "ok");
    EXPECT_TRUE(contains(first, "HTTP/1.0 200 OK\r\n"));
    EXPECT_TRUE(contains(first, "Connection: keep-alive\r\n"));

    send_text(client, "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n");
    std::string second = receive_response(client, "ok");
    EXPECT_TRUE(contains(second, "Connection: keep-alive\r\n"));

    send_text(client, "GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    std::string last = receive_response(client);
    EXPECT_TRUE(contains(last, "HTTP/1.1 200 OK\r\n"));
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, HonorsMultipleConnectionHeaders)
{
    std::string response = exchange(
        "GET /health HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: close\r\n"
        "Connection: keep-alive\r\n\r\n");
    EXPECT_TRUE(contains(response, "Connection: close\r\n"));
}

TEST_F(HttpServerLoopbackTest, ClosesConnectionAfterMaxRequestsPerConnection)
{
    std::string const request = "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n";
    std::string response = exchange(request + request + request + request);
    EXPECT_EQ(count_occurrences(response, "HTTP/1.1 200 OK\r\n"), 4U);
    EXPECT_TRUE(contains(response, "Connection: close\r\n"));
}

TEST_F(HttpServerLoopbackTest, RejectsMalformedRequestLines)
{
    std::string const bad_lines[] = {
        " GET /health HTTP/1.1",
        "GET  /health HTTP/1.1",
        "GET /health HTTP/1.1 extra",
        "GET /health",
        "GET/health HTTP/1.1",
        "G@T /health HTTP/1.1",
        std::string("GET /a") + '\x01' + "b HTTP/1.1",
        "GET /health#frag HTTP/1.1",
        "GET health HTTP/1.1",
        "GET http:///health HTTP/1.1",
        "ABCDEFGHIJKLMNOPQ /health HTTP/1.1",
    };
    for (std::string const& line : bad_lines)
    {
        std::string response = exchange(line + "\r\nHost: localhost\r\nConnection: close\r\n\r\n");
        EXPECT_TRUE(contains(response, "HTTP/1.1 400 Bad Request\r\n")) << line;
    }
}

TEST_F(HttpServerLoopbackTest, HandlesAbsoluteAndUnusualRequestTargets)
{
    EXPECT_TRUE(contains(exchange("GET https://localhost/health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "\r\n\r\nok"));
    EXPECT_TRUE(contains(exchange("GET http://localhost HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "fallback-404"));
    EXPECT_TRUE(contains(exchange("GET http://localhost?refresh=1 HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "fallback-404"));
    EXPECT_TRUE(contains(exchange("GET /" + std::string(130U, 'a') + " HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"),
                         "HTTP/1.1 414 URI Too Long\r\n"));
}

TEST_F(HttpServerLoopbackTest, RejectsOversizedRequestLinesAndHeaderSections)
{
    EXPECT_TRUE(contains(exchange("GET /" + std::string(170U, 'a')), "HTTP/1.1 414 URI Too Long\r\n"));
    EXPECT_TRUE(contains(exchange("GET /health HTTP/1.1\r\nHost: localhost\r\nX-Pad: " + std::string(240U, 'a')),
                         "HTTP/1.1 431 Request Header Fields Too Large\r\n"));
    EXPECT_TRUE(contains(exchange("GET /health HTTP/1.1\r\nHost: localhost\r\nX-Pad: " + std::string(210U, 'a') + "\r\n\r\n"),
                         "HTTP/1.1 431 Request Header Fields Too Large\r\n"));
}

TEST_F(HttpServerLoopbackTest, RejectsMalformedHeaderFields)
{
    std::string const host = "GET /health HTTP/1.1\r\nHost: localhost\r\n";
    struct malformed_case
    {
        std::string request;
        char const* status_line;
    };
    malformed_case const cases[] = {
        {host + "Bad Name: x\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + " folded: x\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "\tfolded: x\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "NoColon\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + ": missing-name\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "Host: other\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {"GET /health HTTP/1.1\r\nHost:\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {"GET /health HTTP/1.1\r\nConnection: close\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {"GET /health HTTP/1.1\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "Content-Length: abc\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "Transfer-Encoding: chunked\r\nTransfer-Encoding: chunked\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "Content-Length: 0\r\nTransfer-Encoding: chunked\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {host + "Expect: unsupported\r\n\r\n", "HTTP/1.1 417 Expectation Failed\r\n"},
        {host + "X-Long-Name-" + std::string(40U, 'a') + ": v\r\n\r\n", "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
        {host + "X-Big: " + std::string(70U, 'a') + "\r\n\r\n", "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
    };
    for (malformed_case const& item : cases)
    {
        EXPECT_TRUE(contains(exchange(item.request), item.status_line)) << item.request;
    }
}

TEST_F(HttpServerLoopbackTest, DecodesChunkedBodiesWithExtensionsAndTrailers)
{
    std::string response = exchange(
        "POST /echo HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Connection: close\r\n\r\n"
        "a\r\n0123456789\r\n"
        "B\r\nabcdefghijk\r\n"
        "3;name=v\r\nxyz\r\n"
        "0\r\nX-Trailer: value\r\n\r\n");
    EXPECT_TRUE(contains(response, "Content-Length: 24\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\n0123456789abcdefghijkxyz"));
}

TEST_F(HttpServerLoopbackTest, RejectsMalformedAndOversizedChunkedBodies)
{
    std::string const head =
        "POST /echo HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
    struct malformed_case
    {
        std::string request;
        char const* status_line;
    };
    malformed_case const cases[] = {
        {head + "zz\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "41\r\n", "HTTP/1.1 413 Payload Too Large\r\n"},
        {head + "4\r\nWikiab", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "4\r\nWiki\rX", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\n X: y\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\nNoColon\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\n: y\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\nA B: y\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\nContent-Length: 1\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\nTransfer-Encoding: chunked\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\nHost: x\r\n\r\n", "HTTP/1.1 400 Bad Request\r\n"},
        {head + "0\r\nX-Long-Name-" + std::string(40U, 'a') + ": v\r\n\r\n", "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
        {head + "0\r\nX-V: " + std::string(70U, 'a') + "\r\n\r\n", "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
        {head + "0\r\n" + repeated_trailers(9) + "\r\n", "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
        {head + "0\r\nX-Big: " + std::string(250U, 'a') + "\r\n\r\n", "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
        {head + "0\r\n" + std::string(300U, 'a'), "HTTP/1.1 431 Request Header Fields Too Large\r\n"},
    };
    for (malformed_case const& item : cases)
    {
        EXPECT_TRUE(contains(exchange(item.request), item.status_line)) << item.status_line;
    }
}

TEST_F(HttpServerLoopbackTest, ReassemblesSplitRequestLinesAndHeaders)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);

    send_text(client, "G");
    pump();
    send_text(client, "ET /health HTTP/1.1\r\nHost:");
    pump();
    send_text(client, " localhost\r\nConnection: close\r\n\r");
    pump();
    send_text(client, "\n");
    pump();

    std::string response = receive_response(client);
    EXPECT_TRUE(contains(response, "HTTP/1.1 200 OK\r\n"));
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, WaitsForIncompleteLineWithBareCarriageReturn)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    send_text(client, "GET /x\rY");
    pump();
    static_cast<void>(client.close());
    pump();
    EXPECT_EQ(server_.connection_count(), 0U);
}

TEST_F(HttpServerLoopbackTest, ReassemblesSplitContentLengthBody)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    send_text(client, "POST /echo HTTP/1.1\r\nHost: localhost\r\nContent-Length: 4\r\nConnection: close\r\n\r\nda");
    pump();
    send_text(client, "ta");
    pump();

    std::string response = receive_response(client);
    EXPECT_TRUE(contains(response, "\r\n\r\ndata"));
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, ReassemblesSplitChunkedBody)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    send_text(client, "POST /echo HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n4");
    pump();
    send_text(client, "\r\nWi");
    pump();
    send_text(client, "ki\r");
    pump();
    send_text(client, "\n0\r\n");
    pump();
    send_text(client, "\r\n");
    pump();

    std::string response = receive_response(client);
    EXPECT_TRUE(contains(response, "\r\n\r\nWiki"));
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, RejectsRequestsBeyondReceiveBufferAcrossReads)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    send_text(client, "GET /health HTTP/1.1\r\nHost: localhost\r\nX-Pad: ");
    pump();
    send_text(client, std::string(500U, 'a'));
    pump();

    std::string response = receive_response(client);
    EXPECT_TRUE(contains(response, "HTTP/1.1 413 Payload Too Large\r\n"));
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, ReleasesSlotsClosedThroughTransport)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    pump();
    ASSERT_EQ(server_.connection_count(), 1U);

    test_server::client_handle handle = server_.transport().client_at(0U);
    ASSERT_TRUE(handle.is_valid());
    ASSERT_EQ(server_.transport().close_client(handle), status::ok);
    pump(2);
    EXPECT_EQ(server_.connection_count(), 0U);
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, ServesClientsAcceptedDirectlyByTransport)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    test_server::client_handle handle;
    ASSERT_EQ(server_.transport().accept_client(handle, 1000), status::ok);

    send_text(client, "GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    pump();
    std::string response = receive_response(client);
    EXPECT_TRUE(contains(response, "\r\n\r\nok"));
    static_cast<void>(client.close());
}

TEST_F(HttpServerLoopbackTest, DropsDataForClientsWithoutHttpState)
{
    stop_worker();
    net::tcp_client<512U> client;
    ASSERT_EQ(client.connect(local_, 1000, false), status::ok);
    test_server::client_handle handle;
    ASSERT_EQ(server_.transport().accept_client(handle, 1000), status::ok);

    send_text(client, "GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_EQ(server_.transport().process_client(handle, 1000), status::ok);
    static_cast<void>(client.close());
    pump();
}

TEST_F(HttpServerLoopbackTest, ReplacesStaleConnectionWhenSlotIsReused)
{
    stop_worker();
    net::tcp_client<512U> first;
    ASSERT_EQ(first.connect(local_, 1000, false), status::ok);
    pump();
    ASSERT_EQ(server_.connection_count(), 1U);
    test_server::client_handle stale = server_.transport().client_at(0U);
    ASSERT_EQ(server_.transport().close_client(stale), status::ok);
    static_cast<void>(first.close());

    net::tcp_client<512U> second;
    ASSERT_EQ(second.connect(local_, 1000, false), status::ok);
    test_server::client_handle fresh;
    ASSERT_EQ(server_.transport().accept_client(fresh, 1000), status::ok);
    ASSERT_EQ(fresh.index, stale.index);
    ASSERT_NE(fresh.generation, stale.generation);

    send_text(second, "GET /health HTTP/1.1\r\nHost: localhost\r\n\r\n");
    EXPECT_EQ(server_.transport().process_client(fresh, 1000), status::ok);

    send_text(second, "GET /health HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    pump();
    std::string response = receive_response(second);
    EXPECT_TRUE(contains(response, "\r\n\r\nok"));
    static_cast<void>(second.close());
}

TEST_F(HttpServerLoopbackTest, PollAcceptsQueuedConnectionsAndSkipsWhenFull)
{
    stop_worker();
    net::tcp_client<512U> a;
    net::tcp_client<512U> b;
    net::tcp_client<512U> c;
    ASSERT_EQ(a.connect(local_, 1000, false), status::ok);
    ASSERT_EQ(b.connect(local_, 1000, false), status::ok);
    pump();
    EXPECT_EQ(server_.connection_count(), 2U);

    ASSERT_EQ(c.connect(local_, 1000, false), status::ok);
    EXPECT_EQ(server_.poll(0), status::empty);
    EXPECT_EQ(server_.connection_count(), 2U);

    static_cast<void>(a.close());
    static_cast<void>(b.close());
    static_cast<void>(c.close());
}

TEST_F(HttpServerLoopbackTest, ReportsTransportErrorsWhenListenerIsClosed)
{
    stop_worker();
    static_cast<void>(server_.transport().close(nullptr));
    EXPECT_EQ(server_.poll(0), status::not_configured);
    static_cast<void>(server_.stop());
}

TEST_F(HttpServerLoopbackTest, ReturnsDefaultNotFoundWithoutFallbackHandler)
{
    stop_worker();
    server_.clear_routes();
    server_.register_handler(HttpMethod::GET, "/only",
        [](test_server::request_type const&, test_server::response_type& response)
        {
            static_cast<void>(response.set_body("only"));
        });
    std::string response = pumped_exchange("GET /missing HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    EXPECT_TRUE(contains(response, "HTTP/1.1 404 Not Found\r\n"));
    EXPECT_TRUE(contains(response, "\r\n\r\nNot Found"));
}

TEST_F(HttpServerLoopbackTest, MatchesExactRootAndTrailingSlashPrefixes)
{
    stop_worker();
    server_.clear_routes();
    server_.register_handler(HttpMethod::GET, "/",
        [](test_server::request_type const&, test_server::response_type& response)
        {
            static_cast<void>(response.set_body("root"));
        });
    server_.register_prefix_handler(HttpMethod::GET, "/dir/",
        [](test_server::request_type const&, test_server::response_type& response)
        {
            static_cast<void>(response.set_body("dir"));
        });
    server_.register_handler(HttpMethod::GET, "/t/",
        [](test_server::request_type const&, test_server::response_type& response)
        {
            static_cast<void>(response.set_body("t"));
        });

    EXPECT_TRUE(contains(pumped_exchange("GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nroot"));
    EXPECT_TRUE(contains(pumped_exchange("GET /dir/x HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\ndir"));
    EXPECT_TRUE(contains(pumped_exchange("GET /dir HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "HTTP/1.1 404 Not Found\r\n"));
    EXPECT_TRUE(contains(pumped_exchange("GET /t HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "HTTP/1.1 404 Not Found\r\n"));
    EXPECT_TRUE(contains(pumped_exchange("GET /t/ HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nt"));
}

TEST_F(HttpServerLoopbackTest, MatchesEveryPathWithRootPrefix)
{
    stop_worker();
    server_.clear_routes();
    server_.register_prefix_handler(HttpMethod::GET, "/",
        [](test_server::request_type const&, test_server::response_type& response)
        {
            static_cast<void>(response.set_body("all"));
        });
    EXPECT_TRUE(contains(pumped_exchange("GET /anything HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"), "\r\n\r\nall"));
}

} // namespace
