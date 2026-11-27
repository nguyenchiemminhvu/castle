#include "gnss/components.hpp"

#include <errno.h>

namespace gnss
{

GnssTransport::GnssTransport(const GnssTransConfig& config) noexcept
    : config_(config)
{
}

castle::status GnssTransport::init() noexcept
{
    const castle::io::uart::uart_config uart_config(
        config_.uart_path.c_str(),
        config_.baud_rate,
        castle::io::uart::data_bits::eight,
        castle::io::uart::parity::none,
        castle::io::uart::stop_bits::one,
        castle::io::uart::flow_control::none,
        0U,
        0U,
        true);
    return uart_.open(uart_config);
}

castle::status GnssTransport::deinit() noexcept
{
    return shutdown();
}

castle::status GnssTransport::suspend() noexcept
{
    if (suspended_)
    {
        return castle::status::ok;
    }
    suspended_ = true;
    if (uart_.is_open())
    {
        static_cast<void>(uart_.flush_input());
        static_cast<void>(uart_.flush_output());
        return uart_.close();
    }
    return castle::status::ok;
}

castle::status GnssTransport::resume() noexcept
{
    if (!suspended_)
    {
        return castle::status::ok;
    }
    suspended_ = false;
    return init();
}

castle::status GnssTransport::shutdown() noexcept
{
    suspended_ = false;
    return uart_.is_open() ? uart_.close() : castle::status::ok;
}

castle::status GnssTransport::reopen() noexcept
{
    if (uart_.is_open())
    {
        static_cast<void>(uart_.close());
    }
    return init();
}

castle::status GnssTransport::read(uint8_t* bytes, castle::size_type capacity,
                                   castle::size_type& count) noexcept
{
    count = 0U;
    if (!uart_.is_open())
    {
        return castle::status::not_configured;
    }

    short events = 0;
    const castle::status ready = castle::io::fd::wait_fd(
        uart_.native_handle(), castle::io::fd::poll_read, 0, events);
    if (ready == castle::status::time_out)
    {
        return castle::status::empty;
    }
    if (ready != castle::status::ok)
    {
        return ready;
    }
    if ((events & (castle::io::fd::poll_error | castle::io::fd::poll_hangup)) != 0)
    {
        return castle::status::system_call_error;
    }
    return uart_.read_some(bytes, capacity, count);
}

castle::status GnssTransport::write(const uint8_t* bytes, castle::size_type size,
                                    castle::size_type& count) noexcept
{
    count = 0U;
    if (!uart_.is_open())
    {
        return castle::status::not_configured;
    }
    // Non-blocking UART writes can be short or return `full` (EAGAIN).
    // Bound both the number of retries and each readiness wait so a control
    // transaction can never stall the service loop indefinitely.
    constexpr uint8_t max_attempts = 8U;
    uint8_t attempts = 0U;
    while (count < size && attempts < max_attempts)
    {
        short events = 0;
        const castle::status ready = castle::io::fd::wait_fd(
            uart_.native_handle(), castle::io::fd::poll_write, 10, events);
        ++attempts;
        if (ready == castle::status::time_out)
        {
            continue;
        }
        if (ready != castle::status::ok)
        {
            return ready;
        }
        if ((events & (castle::io::fd::poll_error | castle::io::fd::poll_hangup)) != 0)
        {
            return castle::status::system_call_error;
        }

        castle::size_type written = 0U;
        const castle::status result = uart_.write_some(bytes + count, size - count, written);
        count += written;
        if (result == castle::status::full)
        {
            continue;
        }
        if (result != castle::status::ok)
        {
            return result;
        }
    }
    return (count == size) ? castle::status::ok : castle::status::full;
}

bool GnssTransport::is_open() const noexcept
{
    return uart_.is_open();
}

int GnssTransport::native_handle() const noexcept
{
    return uart_.native_handle();
}

} // namespace gnss
