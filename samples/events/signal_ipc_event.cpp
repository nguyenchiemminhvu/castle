#include "sample_support.hpp"

#include "castle/events/signal_ipc_event.hpp"

using namespace castle::events;

int main()
{
    using ipc_type = signal_ipc_event<
        signal_ipc_config<signal::sigusr1, 2U, 64U>,
        signal_ipc_config<signal::sigusr2, 1U>
    >;

    static_assert(ipc_type::signal_capacity() == 2U, "Unexpected signal capacity");
    static_assert(ipc_type::callback_capacity<signal::sigusr1>() == 2U, "Unexpected sigusr1 callback capacity");

    volatile sig_atomic_t sigusr1_count = 0;
    volatile sig_atomic_t sigusr2_count = 0;

    ipc_type::error error = ipc_type::error::unknown_error;

    auto sigusr1_first = ipc_type::register_callback<signal::sigusr1>(
        [&sigusr1_count]() { ++sigusr1_count; },
        &error
    );
    CASTLE_SAMPLE_CHECK(error == ipc_type::error::ok);
    CASTLE_SAMPLE_CHECK(static_cast<bool>(sigusr1_first));

    auto sigusr1_second = ipc_type::register_callback<signal::sigusr1>(
        [&sigusr1_count]() { sigusr1_count += 10; },
        &error
    );
    CASTLE_SAMPLE_CHECK(error == ipc_type::error::ok);
    CASTLE_SAMPLE_CHECK(static_cast<bool>(sigusr1_second));

    auto sigusr1_overflow = ipc_type::register_callback<signal::sigusr1>(
        []() {},
        &error
    );
    CASTLE_SAMPLE_CHECK(error == ipc_type::error::full);
    CASTLE_SAMPLE_CHECK(!sigusr1_overflow.valid());

    auto sigusr2_subscription = ipc_type::register_callback<signal::sigusr2>(
        [&sigusr2_count]() { ++sigusr2_count; },
        &error
    );
    CASTLE_SAMPLE_CHECK(error == ipc_type::error::ok);
    CASTLE_SAMPLE_CHECK(static_cast<bool>(sigusr2_subscription));

    CASTLE_SAMPLE_CHECK(ipc_type::subscriber_count<signal::sigusr1>() == 2U);
    CASTLE_SAMPLE_CHECK(ipc_type::subscriber_count<signal::sigusr2>() == 1U);
    CASTLE_SAMPLE_CHECK(ipc_type::callback_storage_size<signal::sigusr1>() == 64U);
    CASTLE_SAMPLE_CHECK(ipc_type::callback_storage_alignment<signal::sigusr2>() > 0U);
    CASTLE_SAMPLE_CHECK(!ipc_type::is_installed());

    CASTLE_SAMPLE_CHECK(ipc_type::install() == ipc_type::error::ok);
    CASTLE_SAMPLE_CHECK(ipc_type::is_installed());
    CASTLE_SAMPLE_CHECK(ipc_type::is_signal_enabled<signal::sigusr1>());
    CASTLE_SAMPLE_CHECK(ipc_type::is_signal_enabled<signal::sigusr2>());

    ipc_type::disable_signal<signal::sigusr1>();
    CASTLE_SAMPLE_CHECK(!ipc_type::is_signal_enabled<signal::sigusr1>());
    CASTLE_SAMPLE_CHECK(::raise(SIGUSR1) == 0);
    CASTLE_SAMPLE_CHECK(sigusr1_count == 0);

    ipc_type::enable_signal<signal::sigusr1>();
    CASTLE_SAMPLE_CHECK(ipc_type::is_signal_enabled<signal::sigusr1>());
    CASTLE_SAMPLE_CHECK(::raise(SIGUSR1) == 0);
    CASTLE_SAMPLE_CHECK(sigusr1_count == 11);

    CASTLE_SAMPLE_CHECK(::raise(SIGUSR2) == 0);
    CASTLE_SAMPLE_CHECK(sigusr2_count == 1);

    ipc_type::clear_signal<signal::sigusr2>();
    CASTLE_SAMPLE_CHECK(ipc_type::subscriber_count<signal::sigusr2>() == 0U);
    CASTLE_SAMPLE_CHECK(::raise(SIGUSR2) == 0);
    CASTLE_SAMPLE_CHECK(sigusr2_count == 1);

    ipc_type::clear();
    CASTLE_SAMPLE_CHECK(ipc_type::subscriber_count<signal::sigusr1>() == 0U);
    CASTLE_SAMPLE_CHECK(ipc_type::subscriber_count<signal::sigusr2>() == 0U);

    CASTLE_SAMPLE_CHECK(sigusr1_first.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(sigusr1_second.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(sigusr2_subscription.unsubscribe() == castle::status::invalid_subscription);

    ipc_type::uninstall();
    CASTLE_SAMPLE_CHECK(!ipc_type::is_installed());

    return 0;
}
