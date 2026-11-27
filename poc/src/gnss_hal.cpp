#include "gnss/components.hpp"

#include <unistd.h>

namespace gnss
{

GnssHal::GnssHal(GnssTransport& transport, UbxController& controller,
                 GnssReceiver& receiver, DiagnosticComponent& diagnostic,
                 DataStore& data) noexcept
    : transport_(transport)
    , controller_(controller)
    , receiver_(receiver)
    , diagnostic_(diagnostic)
    , data_(data)
{
}

castle::status GnssHal::init() noexcept
{
    if (!data_.pool.add_observer(&inbox_))
    {
        return castle::status::resource_exhausted;
    }
    last_byte_ms_ = monotonic_milliseconds();
    last_pvt_ms_ = last_byte_ms_;
    const castle::status result = controller_.begin_boot_sync();
    running_ = (result == castle::status::ok);
    return result;
}

castle::status GnssHal::deinit() noexcept
{
    static_cast<void>(data_.pool.remove_observer(&inbox_));
    running_ = false;
    return castle::status::ok;
}

castle::status GnssHal::suspend() noexcept
{
    suspended_ = true;
    return castle::status::ok;
}

castle::status GnssHal::resume() noexcept
{
    suspended_ = false;
    last_byte_ms_ = monotonic_milliseconds();
    last_pvt_ms_ = last_byte_ms_;
    return castle::status::ok;
}

castle::status GnssHal::shutdown() noexcept
{
    return deinit();
}

void GnssHal::update(uint32_t now_ms) noexcept
{
    if (!running_ || suspended_)
    {
        return;
    }

    uint32_t received = 0U;
    const castle::status rx_status = receiver_.poll(received);
    if (rx_status == castle::status::ok && received > 0U)
    {
        last_byte_ms_ = now_ms;
        total_rx_bytes_ += received;
    }
    else if (rx_status != castle::status::empty && rx_status != castle::status::time_out &&
             rx_status != castle::status::ok)
    {
        auto existing = data_.pool.get_snapshot<HalStatus>();
        HalStatus status{};
        status.uart_open = transport_.is_open();
        status.rx_bytes = total_rx_bytes_;
        status.last_error = rx_status;
        if (existing)
        {
            status.recovery_level = existing->recovery_level;
        }
        static_cast<void>(data_.pool.publish(status));
    }

    const Pool::update_set_type changed = inbox_.take();
    if (changed.contains<PvtHeartbeat>())
    {
        const auto latest_pvt = data_.pool.get_snapshot<PvtHeartbeat>();
        if (latest_pvt)
        {
            last_pvt_ms_ = now_ms;
        }
    }

    diagnostic_.sample(now_ms, last_byte_ms_, last_pvt_ms_);
    if (!diagnostic_.no_bytes_fault() && !diagnostic_.no_pvt_fault() &&
        diagnostic_.recovery_level() == 0U)
    {
        recovery_level_ = 0U;
    }
    const uint32_t recovery_requests = diagnostic_.consume_recovery_requests();
    if ((recovery_requests & 1U) != 0U && recovery_level_ == 0U)
    {
        const castle::status reopened = transport_.reopen();
        castle::string_builder<96U> recovery_line;
        recovery_line.build("GNSS recovery L1 reopen status=", static_cast<uint8_t>(reopened), "\n");
        static_cast<void>(::write(STDOUT_FILENO, recovery_line.c_str(), recovery_line.size()));
        recovery_level_ = 1U;
        diagnostic_.mark_recovery_level(recovery_level_);
        if (reopened != castle::status::ok)
        {
            HalStatus failure{};
            failure.uart_open = false;
            failure.recovery_level = recovery_level_;
            failure.rx_bytes = total_rx_bytes_;
            failure.last_error = reopened;
            static_cast<void>(data_.pool.publish(failure));
        }
    }
    if ((recovery_requests & 2U) != 0U && recovery_level_ < 2U)
    {
        const castle::status reset = controller_.cold_start();
        castle::string_builder<96U> recovery_line;
        recovery_line.build("GNSS recovery L2 cold_start status=", static_cast<uint8_t>(reset), "\n");
        static_cast<void>(::write(STDOUT_FILENO, recovery_line.c_str(), recovery_line.size()));
        recovery_level_ = 2U;
        diagnostic_.mark_recovery_level(recovery_level_);
        HalStatus status{};
        status.uart_open = transport_.is_open();
        status.recovery_level = recovery_level_;
        status.rx_bytes = total_rx_bytes_;
        status.last_error = reset;
        static_cast<void>(data_.pool.publish(status));
    }

    if ((now_ms % 100U) < 10U)
    {
        HalStatus status{};
        status.uart_open = transport_.is_open();
        status.no_bytes_fault = diagnostic_.no_bytes_fault();
        status.no_valid_pvt_fault = diagnostic_.no_pvt_fault();
        status.recovery_level = recovery_level_;
        status.rx_bytes = total_rx_bytes_;
        status.last_error = castle::status::ok;
        static_cast<void>(data_.pool.publish(status));
    }
}

bool GnssHal::running() const noexcept
{
    return running_;
}

} // namespace gnss
