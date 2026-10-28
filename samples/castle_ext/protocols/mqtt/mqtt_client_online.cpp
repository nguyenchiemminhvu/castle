// Online sample: start an external broker first, for example:
//   mosquitto -p 1883
// Then publish a test message from another terminal:
//   mosquitto_pub -h 127.0.0.1 -p 1883 -t castle/test/online -m hello
// Or with Python paho-mqtt: publish.single("castle/test/online", "hello", hostname="127.0.0.1", port=1883)
// The MQTT library remains polling-based; this sample has no worker thread.
#include "sample_support.hpp"
#include "castle/chrono/chrono.hpp"
#include "castle_ext/protocols/mqtt/client.hpp"

#include <stdint.h>

int main()
{
    using namespace castle::protocols::mqtt;
    basic_client<512U, 1024U, 4U, 128U, 512U> client;
    bool received = false;
    if (client.set_message_callback([&received](message_view const& message)
    {
        bool topic_equal = message.topic.size() == sizeof("castle/test/online") - 1U;
        for (castle::size_type i = 0U; topic_equal && i < message.topic.size(); ++i)
        { topic_equal = message.topic[i] == "castle/test/online"[i]; }
        if (topic_equal && message.payload_size != 0U) { received = true; }
    }) != castle::status::ok) { return 1; }

    connect_options<> options;
    options.protocol_version = version::v3_1_1;
    options.client_id = "castle-online-subscriber";
    options.keep_alive_seconds = 20U;
    if (client.connect(castle::net::endpoint::loopback_ipv4(1883U), options, 3000) != castle::status::ok)
    { return 2; }
    uint16_t subscription_id = 0U;
    if (client.subscribe("castle/test/online", qos::at_least_once, subscription_id) != castle::status::ok)
    { (void)client.disconnect(); return 3; }

    const auto start = castle::chrono::steady_clock::now();
    while (!received && castle::chrono::steady_clock::now() - start < castle::chrono::seconds(30))
    {
        uint32_t now = static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
            castle::chrono::steady_clock::now().time_since_epoch()).count());
        castle::status status = client.poll(now, 100);
        if (status != castle::status::ok && status != castle::status::empty) { (void)client.force_close(); return 4; }
    }
    (void)client.disconnect();
    return received ? 0 : 5;
}
