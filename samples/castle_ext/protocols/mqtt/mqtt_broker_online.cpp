// Standalone localhost MQTT broker on 127.0.0.1:1883. Runs until SIGINT (Ctrl+C) or SIGTERM.
// Test from another terminal with the mosquitto clients, for example:
//   mosquitto_sub -h localhost -p 1883 -t "test/castle/#"
//   mosquitto_pub -h localhost -p 1883 -t test/castle/broker -m "this is castle broker data"
#include "castle/chrono/chrono.hpp"
#include "castle_ext/protocols/mqtt/broker.hpp"

#include <signal.h>
#include <stdio.h>

namespace
{

using broker_type = castle::protocols::mqtt::basic_broker<4U, 512U, 1024U, 16U, 8U, 8U, 128U, 512U>;

volatile sig_atomic_t stop_requested = 0;

void handle_stop_signal(int)
{
    stop_requested = 1;
}

}

int main()
{
    broker_type broker;
    if (broker.open(castle::net::endpoint::loopback_ipv4(1883U)) != castle::status::ok)
    {
        fprintf(stderr, "failed to listen on 127.0.0.1:1883\n");
        return 1;
    }
    signal(SIGINT, handle_stop_signal);
    signal(SIGTERM, handle_stop_signal);
    printf("castle MQTT broker listening on 127.0.0.1:1883 (Ctrl+C to stop)\n");

    const auto epoch = castle::chrono::steady_clock::now();
    int exit_code = 0;
    while (stop_requested == 0)
    {
        uint32_t now = static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
            castle::chrono::steady_clock::now() - epoch).count());
        // A signal interrupts the accept wait with EINTR, which poll() reports as an error;
        // that is expected once stop_requested is set, so only treat it as a failure otherwise.
        if (broker.poll(now, 100) != castle::status::ok && stop_requested == 0)
        {
            fprintf(stderr, "broker poll failed\n");
            exit_code = 2;
            break;
        }
    }

    printf("stopping castle MQTT broker\n");
    (void)broker.close();
    return exit_code;
}
