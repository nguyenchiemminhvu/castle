#include "sample_support.hpp"
#include "castle_ext/protocols/coap/client.hpp"

int main()
{
    using namespace castle::protocols::coap;
    basic_client<256U, 2U, 8U, 32U, 128U, 4U, 64U> client;
    CASTLE_SAMPLE_CHECK(client.set_retransmission(1000U, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.set_retransmission(0U, 2U) == castle::status::invalid_argument);
    bool callback_seen = false;
    CASTLE_SAMPLE_CHECK(client.set_response_callback([&callback_seen](basic_client<256U, 2U, 8U, 32U, 128U, 4U, 64U>::exchange_handle,
        castle::status, message<8U, 32U, 128U> const*, castle::net::endpoint const&) { callback_seen = true; }) == castle::status::ok);
    basic_client<256U, 2U, 8U, 32U, 128U, 4U, 64U>::exchange_handle invalid;
    CASTLE_SAMPLE_CHECK(client.cancel(invalid) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(client.allocate_message_id() == 1U);
    CASTLE_SAMPLE_CHECK(client.outstanding() == 0U);
    (void)callback_seen;
    return 0;
}
