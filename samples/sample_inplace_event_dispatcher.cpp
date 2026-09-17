#include "castle/events/event_config.h"
#include "castle/events/inplace_event_dispatcher.h"

#include <iostream>

using namespace castle::callbacks;
using namespace castle::events;

struct timer_expired_tag {};
struct button_pressed_tag {};

using dispatcher_t = inplace_event_dispatcher<
    event_config<timer_expired_tag, 2, void(std::uint32_t)>,
    event_config<button_pressed_tag, 2, void(std::uint32_t, std::uint32_t), 64>
>;

using timer_expired_signature = void(std::uint32_t);
using button_pressed_signature = void(std::uint32_t, std::uint32_t);

dispatcher_t g_dispatcher;

void on_timer(std::uint32_t timer_id)
{
    std::cout << "Timer expired: " << timer_id << std::endl;
}

void on_button(std::uint32_t button_id, std::uint32_t timestamp)
{
    std::cout << "Button pressed: " << button_id << " at " << timestamp << std::endl;
}

int main()
{
    inplace_function<timer_expired_signature> timer_cb(&on_timer);
    inplace_function<button_pressed_signature> button_cb(&on_button);

    auto timer_sub = g_dispatcher.register_callback<timer_expired_tag>(std::move(timer_cb));
    timer_sub
    ? std::cout << "Timer callback registered successfully." << std::endl
    : std::cout << "Failed to register timer callback." << std::endl;

    auto button_sub = g_dispatcher.register_callback<button_pressed_tag>(std::move(button_cb));
    button_sub
    ? std::cout << "Button callback registered successfully." << std::endl
    : std::cout << "Failed to register button callback." << std::endl;

    g_dispatcher.dispatch_event<timer_expired_tag>(42u);
    g_dispatcher.dispatch_event<button_pressed_tag>(1u, 1000u);

    timer_sub.unsubscribe();
    g_dispatcher.dispatch_event<timer_expired_tag>(43u); // No output, callback unsubscribed
    button_sub.unsubscribe();
    g_dispatcher.dispatch_event<button_pressed_tag>(2u, 2000u); // No output, callback unsubscribed

    return 0;
}
