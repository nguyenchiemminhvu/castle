#include "castle/events/signal_config.h"
#include "castle/events/inplace_signal_event.h"

#include <iostream>
#include <thread>

using namespace castle::callbacks;
using namespace castle::events;

using signals_t = inplace_signal_event<
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
    inplace_function<void()> sigint_cb(&on_sigint);
    inplace_function<void()> sigterm_cb(&on_sigterm);

    auto sub_sigint = signals_t::register_callback<signal::sigint>(std::move(sigint_cb));
    sub_sigint
    ? std::cout << "SIGINT callback registered successfully" << std::endl
    : std::cout << "Failed to register SIGINT callback" << std::endl;

    auto sub_sigterm = signals_t::register_callback<signal::sigterm>(std::move(sigterm_cb));
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