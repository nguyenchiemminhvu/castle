#include "sample_support.hpp"
#include "castle_ext/protocols/coap/coap.hpp"

int main()
{
    castle::protocols::coap::client client;
    castle::protocols::coap::server server;
    CASTLE_SAMPLE_CHECK(!client.is_open());
    CASTLE_SAMPLE_CHECK(!server.is_open());
    CASTLE_SAMPLE_CHECK(server.resource_count() == 0U);
    return 0;
}
