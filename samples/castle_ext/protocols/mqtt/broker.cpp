#include "sample_support.hpp"
#include "castle_ext/protocols/mqtt/broker.hpp"

using namespace castle::protocols::mqtt;

int main()
{
    basic_broker<2U, 128U, 256U, 8U, 4U, 2U, 48U, 64U> broker;
    CASTLE_SAMPLE_CHECK(!broker.transport().is_open());
    CASTLE_SAMPLE_CHECK(broker.client_count() == 0U && broker.subscription_count() == 0U);
    CASTLE_SAMPLE_CHECK(broker.set_authentication("device", "secret") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(broker.set_authentication("", "") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(broker.poll(0U) == castle::status::not_configured);
    return 0;
}
