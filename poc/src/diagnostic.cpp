#include "gnss/components.hpp"

namespace gnss
{

DiagnosticComponent::DiagnosticComponent(DataStore& data, WorkerPool& workers) noexcept
    : data_(data)
    , workers_(workers)
{
}

castle::status DiagnosticComponent::init() noexcept
{
    return manager_.set_recovery<ReceiverFaultTag>(
        &DiagnosticComponent::recovery_callback, this);
}

castle::status DiagnosticComponent::deinit() noexcept
{
    return event_handler_.shutdown();
}

castle::status DiagnosticComponent::suspend() noexcept
{
    suspended_ = true;
    manager_.pause();
    pending_recovery_.exchange(0U, castle::memory_order_acq_rel);
    return castle::status::ok;
}

castle::status DiagnosticComponent::resume() noexcept
{
    if (!suspended_)
    {
        return castle::status::ok;
    }
    suspended_ = false;
    previous_sample_ms_ = 0U;
    manager_.resume();
    return castle::status::ok;
}

castle::status DiagnosticComponent::shutdown() noexcept
{
    return event_handler_.shutdown();
}

void DiagnosticComponent::recovery_callback(void* context) noexcept
{
    static_cast<DiagnosticComponent*>(context)->queue_recovery(1U);
}

void DiagnosticComponent::queue_recovery(uint8_t level) noexcept
{
    const uint32_t bit = level == 1U ? 1U : 2U;
    static_cast<void>(event_handler_.post_event([this, bit]() noexcept
    {
        static_cast<void>(workers_.submit([this, bit]() noexcept
        {
            pending_recovery_.fetch_or(bit, castle::memory_order_release);
        }));
    }));
}

void DiagnosticComponent::sample(uint32_t now_ms, uint32_t last_byte_ms,
                                 uint32_t last_pvt_ms) noexcept
{
    if (suspended_)
    {
        return;
    }
    no_bytes_ = static_cast<uint32_t>(now_ms - last_byte_ms) >= 5000U;
    no_pvt_ = static_cast<uint32_t>(now_ms - last_pvt_ms) >= 5000U;
    static_cast<void>(manager_.report<NoBytesTag>(no_bytes_));
    static_cast<void>(manager_.report<NoPvtTag>(no_pvt_));
    static_cast<void>(manager_.update());

    const bool fault_active = manager_.get_state<ReceiverFaultTag>().active;
    const uint32_t elapsed_ms = previous_sample_ms_ == 0U
        ? 0U : static_cast<uint32_t>(now_ms - previous_sample_ms_);
    previous_sample_ms_ = now_ms;
    if (fault_active && !active_)
    {
        fault_since_ms_ = now_ms;
        recovery_level_ = 0U;
        static_cast<void>(recovery_sequence_.clear());
        static_cast<void>(recovery_sequence_.add_delay(castle::chrono::milliseconds(5000)));
        static_cast<void>(recovery_sequence_.add_call([this]() noexcept
        {
            queue_recovery(2U);
        }));
        static_cast<void>(recovery_sequence_.start());
    }
    else if (fault_active)
    {
        static_cast<void>(recovery_sequence_.update(castle::chrono::milliseconds(elapsed_ms)));
    }
    if (!fault_active && active_)
    {
        recovery_level_ = 0U;
        fault_since_ms_ = 0U;
        static_cast<void>(recovery_sequence_.clear());
    }
    active_ = fault_active;

    HalStatus status{};
    status.no_bytes_fault = no_bytes_;
    status.no_valid_pvt_fault = no_pvt_;
    status.recovery_level = recovery_level_;
    auto previous = data_.pool.get_snapshot<HalStatus>();
    if (previous)
    {
        status.uart_open = previous->uart_open;
        status.rx_bytes = previous->rx_bytes;
        status.last_error = previous->last_error;
    }
    static_cast<void>(data_.pool.publish(status));
}

uint32_t DiagnosticComponent::consume_recovery_requests() noexcept
{
    return pending_recovery_.exchange(0U, castle::memory_order_acq_rel);
}

uint8_t DiagnosticComponent::recovery_level() const noexcept
{
    return recovery_level_;
}

void DiagnosticComponent::mark_recovery_level(uint8_t level) noexcept
{
    recovery_level_ = level;
}

bool DiagnosticComponent::no_bytes_fault() const noexcept
{
    return no_bytes_;
}

bool DiagnosticComponent::no_pvt_fault() const noexcept
{
    return no_pvt_;
}

} // namespace gnss
