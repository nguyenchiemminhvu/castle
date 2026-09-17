#include "sample_support.hpp"

#include "castle/events/sigslot.hpp"

#include <stdint.h>

namespace
{
struct receiver
{
    uint32_t member_total = 0U;
    mutable uint32_t const_total = 0U;

    void add_member(uint32_t value)
    {
        member_total += value;
    }

    void add_const(uint32_t value) const
    {
        const_total += value;
    }
};
} // namespace

int main()
{
    using signal_type = castle::sigslot::signal<4U, void(uint32_t), 64U>;
    using connection_type = signal_type::connection_type;

    static_assert(signal_type::capacity() == 4U, "Unexpected signal capacity");

    signal_type signal;
    receiver instance{};
    const receiver const_instance{};
    uint32_t callback_total = 0U;
    uint32_t lambda_total = 0U;
    castle::sigslot::signal_error error = castle::sigslot::signal_error::ok;

    CASTLE_SAMPLE_CHECK(signal.empty());
    CASTLE_SAMPLE_CHECK(signal.size() == 0U);

    signal_type::callback_type callback(
        [&callback_total](uint32_t value)
        {
            callback_total += value;
        }
    );

    connection_type callback_connection = signal.connect(CASTLE_MOVE(callback), &error);
    CASTLE_SAMPLE_CHECK(error == castle::sigslot::signal_error::ok);
    CASTLE_SAMPLE_CHECK(callback_connection.connected());

    connection_type lambda_connection = signal.connect(
        [&lambda_total](uint32_t value)
        {
            lambda_total += value * 2U;
        },
        &error
    );
    CASTLE_SAMPLE_CHECK(error == castle::sigslot::signal_error::ok);

    connection_type member_connection = signal.connect(instance, &receiver::add_member, &error);
    CASTLE_SAMPLE_CHECK(error == castle::sigslot::signal_error::ok);

    connection_type const_member_connection = signal.connect(const_instance, &receiver::add_const, &error);
    CASTLE_SAMPLE_CHECK(error == castle::sigslot::signal_error::ok);

    auto overflow_connection = signal.connect([](uint32_t) {}, &error);
    CASTLE_SAMPLE_CHECK(error == castle::sigslot::signal_error::full);
    CASTLE_SAMPLE_CHECK(!overflow_connection.connected());
    CASTLE_SAMPLE_CHECK(signal.size() == signal.capacity());

    connection_type moved_connection(CASTLE_MOVE(lambda_connection));
    CASTLE_SAMPLE_CHECK(!lambda_connection.connected());
    CASTLE_SAMPLE_CHECK(moved_connection.connected());

    signal.emit(3U);
    signal(2U);

    CASTLE_SAMPLE_CHECK(callback_total == 5U);
    CASTLE_SAMPLE_CHECK(lambda_total == 10U);
    CASTLE_SAMPLE_CHECK(instance.member_total == 5U);
    CASTLE_SAMPLE_CHECK(const_instance.const_total == 5U);

    CASTLE_SAMPLE_CHECK(moved_connection.disconnect() == castle::sigslot::signal_error::ok);
    CASTLE_SAMPLE_CHECK(moved_connection.disconnect() == castle::sigslot::signal_error::invalid_connection);
    CASTLE_SAMPLE_CHECK(signal.size() == 3U);

    signal.emit(1U);
    CASTLE_SAMPLE_CHECK(callback_total == 6U);
    CASTLE_SAMPLE_CHECK(lambda_total == 10U);
    CASTLE_SAMPLE_CHECK(instance.member_total == 6U);
    CASTLE_SAMPLE_CHECK(const_instance.const_total == 6U);

    signal.disconnect_all();
    CASTLE_SAMPLE_CHECK(signal.empty());
    CASTLE_SAMPLE_CHECK(!callback_connection.connected());
    CASTLE_SAMPLE_CHECK(!member_connection.connected());
    CASTLE_SAMPLE_CHECK(!const_member_connection.connected());

    signal_type::callback_type empty_callback;
    auto invalid_connection = signal.connect(CASTLE_MOVE(empty_callback), &error);
    CASTLE_SAMPLE_CHECK(error == castle::sigslot::signal_error::invalid_connection);
    CASTLE_SAMPLE_CHECK(!invalid_connection.connected());

    return 0;
}
