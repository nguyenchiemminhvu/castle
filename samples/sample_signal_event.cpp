#include "castle/events/signal_config.h"
#include "castle/events/signal_event.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace castle::callbacks;
using namespace castle::events;

using signals_t = signal_event<
    signal_config<signal::sigint,  1>,
    signal_config<signal::sigterm, 1>
>;

void on_sigint()
{
    std::cout << "SIGINT received" << std::endl;
}

void on_sigterm()
{
    std::cout << "SIGTERM received" << std::endl;
}

int main()
{
    function_ct<&on_sigint> sigint_cb;
    function_ct<&on_sigterm> sigterm_cb;

    auto sub_sigint = signals_t::register_callback<signal::sigint>(&sigint_cb);
    sub_sigint
    ? std::cout << "SIGINT callback registered successfully" << std::endl
    : std::cout << "Failed to register SIGINT callback" << std::endl;

    auto sub_sigterm = signals_t::register_callback<signal::sigterm>(&sigterm_cb);
    sub_sigterm
    ? std::cout << "SIGTERM callback registered successfully" << std::endl
    : std::cout << "Failed to register SIGTERM callback" << std::endl;

    signals_t::install();

    while (true)
    {
        // Wait for signals
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    signals_t::uninstall();
    signals_t::clear_signal<signal::sigint>();
    signals_t::clear_signal<signal::sigterm>();

    return 0;
}