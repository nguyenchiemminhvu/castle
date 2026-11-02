#include "sample_support.hpp"
#include "castle_ext/net/client.hpp"
#include "castle_ext/protocols/http/client.hpp"

#include <stdint.h>
#include <stdlib.h>

namespace
{
struct observation
{
    castle::size_type completions = 0U;
    uint16_t status_code = 0U;
    castle::status result = castle::status::empty;
};

template <typename Client, typename Headers>
bool request_and_check(Client& client, observation& state,
                       castle::protocols::http::method verb,
                       castle::protocols::http::string_view path,
                       Headers const& headers,
                       castle::protocols::http::string_view body,
                       uint16_t expected_status)
{
    castle::size_type previous = state.completions;
    if (client.request(verb, path, headers, body, castle::protocols::http::string_view(), true) != castle::status::ok)
    {
        return false;
    }
    for (castle::size_type i = 0U; i < 1000U && state.completions == previous; ++i)
    {
        static_cast<void>(client.poll(10));
    }
    return state.completions == previous + 1U && state.result == castle::status::ok &&
           state.status_code == expected_status;
}
}

int main(int argc, char* argv[])
{
    using namespace castle;
    using namespace castle::protocols;

    net::tcp_client<512U> tcp;
    http::http1_client<net::tcp_client<512U>> client(tcp);
    observation state;
    CASTLE_SAMPLE_CHECK(client.set_response_callback([&state](castle::status result, http::response_view const& response)
    {
        state.result = result;
        state.status_code = response.status_code;
        ++state.completions;
        printf("response body: %s\n", response.body.data());
    }) == status::ok);

    uint16_t port = 8080U;
    if (argc > 1)
    {
        char* end = nullptr;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        CASTLE_SAMPLE_CHECK(end != argv[1] && *end == '\0' && parsed > 0U && parsed <= 65535U);
        port = static_cast<uint16_t>(parsed);
    }
    const net::endpoint remote = net::endpoint::loopback_ipv4(port);
    status connected = client.connect(remote, 1000);
    CASTLE_SAMPLE_CHECK(connected == status::ok || connected == status::time_out);
    for (size_type i = 0U; i < 100U && !client.connected(); ++i)
    {
        static_cast<void>(client.poll(10));
    }
    CASTLE_SAMPLE_CHECK(client.connected());

    http::http1_client<net::tcp_client<512U>>::headers_type headers;
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::get, "/health", headers,
                                          http::string_view(), 200U));

    headers.clear();
    CASTLE_SAMPLE_CHECK(headers.add("Content-Type", "application/json") == status::ok);
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::post, "/api/devices", headers,
                                          "{\"name\":\"sensor-1\"}", 201U));

    headers.clear();
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::get,
                                          "/devices/17/state?refresh=1", headers,
                                          http::string_view(), 200U));

    headers.clear();
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::put,
                                          "/devices/17/state", headers, "active", 200U));

    headers.clear();
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::head,
                                          "/health", headers, http::string_view(), 200U));

    headers.clear();
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::options,
                                          "/api/status", headers, http::string_view(), 204U));

    headers.clear();
    CASTLE_SAMPLE_CHECK(request_and_check(client, state, http::method::delete_,
                                          "/devices/17", headers, http::string_view(), 204U));

    (void)client.close();
    return 0;
}
