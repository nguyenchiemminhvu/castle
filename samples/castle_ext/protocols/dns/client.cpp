#include "sample_support.hpp"
#include "castle_ext/net/client.hpp"
#include "castle_ext/protocols/dns/client.hpp"
#include "castle_ext/protocols/http/client.hpp"

#include <stdint.h>
#include <string.h>

int main()
{
    using namespace castle;
    using namespace castle::net;
    using namespace castle::protocols;

    // The sample is intentionally online: allow outbound UDP/TCP port 53 and TCP port 80.
    udp_client<512U> dns_udp;
    tcp_client<2048U> dns_tcp;
    dns::client resolver(dns_udp, dns_tcp);
    dns::response<8U> answer;
    CASTLE_SAMPLE_CHECK(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U),
                                         "httpforever.com", dns::record_type::a, answer) == status::ok);

    endpoint web_server;
    bool found_address = false;
    for (size_type i = 0U; i < answer.record_count; ++i)
    {
        if (answer.records[i].type == dns::record_type::a &&
            answer.records[i].address.family() == address_family::ipv4)
        {
            web_server = answer.records[i].address;
            found_address = true;
            break;
        }
    }
    CASTLE_SAMPLE_CHECK(found_address);
    web_server = endpoint::ipv4(web_server.address_bytes()[0U], web_server.address_bytes()[1U],
                                web_server.address_bytes()[2U], web_server.address_bytes()[3U], 80U);

    // 13,000 bytes are reserved for the received entity body. The page size changes over time, so the
    // body is validated by its closing tag rather than by an exact size.
    char body_storage[20000U]{};
    size_type body_size = 0U;
    bool body_overflow = false;
    bool response_ok = false;
    tcp_client<2048U> http_tcp;
    using http_client = http::http1_client<tcp_client<2048U>, 2048U, 20000U,
                                           32U, 64U, 512U, 640U, 2048U, true>;
    http_client http_request(http_tcp);
    CASTLE_SAMPLE_CHECK(http_request.set_body_callback([&](http::string_view chunk)
    {
        if (chunk.size() > sizeof(body_storage) - 1U - body_size)
        {
            body_overflow = true;
            return;
        }
        for (size_type i = 0U; i < chunk.size(); ++i)
        {
            body_storage[body_size + i] = chunk[i];
        }
        body_size += chunk.size();
        body_storage[body_size] = '\0';
    }) == status::ok);
    CASTLE_SAMPLE_CHECK(http_request.set_response_callback([&](status result, http::response_view const& response)
    {
        response_ok = result == status::ok && response.status_code == 200U;
    }) == status::ok);

    status connected = http_request.connect(web_server, "httpforever.com:80", 5000);
    CASTLE_SAMPLE_CHECK(connected == status::ok || connected == status::time_out);
    for (size_type i = 0U; i < 1000U && !http_request.connected(); ++i)
    {
        (void)http_request.poll(10);
    }
    CASTLE_SAMPLE_CHECK(http_request.connected());

    http_client::headers_type headers;
    CASTLE_SAMPLE_CHECK(http_request.request(http::method::get, "/", headers,
                                             http::string_view(), http::string_view(), false) == status::ok);
    for (size_type i = 0U; i < 2000U && http_request.busy(); ++i)
    {
        (void)http_request.poll(10);
    }
    CASTLE_SAMPLE_CHECK(response_ok);
    CASTLE_SAMPLE_CHECK(!body_overflow);
    CASTLE_SAMPLE_CHECK(body_size > 0U && strstr(body_storage, "</html>") != nullptr);
    CASTLE_SAMPLE_CHECK(body_storage[body_size] == '\0');
    (void)http_request.close();
    return 0;
}
