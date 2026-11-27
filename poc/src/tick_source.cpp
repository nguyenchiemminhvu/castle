#include "gnss/components.hpp"

#include <signal.h>
#include <time.h>

namespace gnss
{

castle::status TickEngine::init() noexcept
{
    if (tick_timer_.set_period(1U) != castle::status::ok)
    {
        return castle::status::invalid_config;
    }
    tick_subscription_ = tick_timer_.register_callback([this]() noexcept
    {
        hal_.update(now_ms_);
        logger_.update();
        pearlchain_.update(now_ms_);
    });
    return tick_timer_.start(castle::events::tick_timer_mode::periodic) == castle::status::ok
        ? castle::status::ok : castle::status::invalid_state;
}

TickEngine::TickEngine(GnssHal& hal, LocationLogger& logger, Pearlchain& pearlchain) noexcept
    : hal_(hal)
    , logger_(logger)
    , pearlchain_(pearlchain)
{
}

castle::status TickEngine::deinit() noexcept
{
    tick_timer_.stop();
    tick_subscription_.unsubscribe();
    return castle::status::ok;
}

castle::status TickEngine::suspend() noexcept
{
    tick_timer_.pause();
    return castle::status::ok;
}

castle::status TickEngine::resume() noexcept
{
    return tick_timer_.resume() == castle::status::ok
        ? castle::status::ok : castle::status::invalid_state;
}

castle::status TickEngine::shutdown() noexcept
{
    return deinit();
}

void TickEngine::tick(uint32_t now_ms, uint32_t elapsed_ticks) noexcept
{
    now_ms_ = now_ms;
    tick_timer_.on_tick(elapsed_ticks == 0U ? 1U : elapsed_ticks);
}

castle::status PosixTickSource::init() noexcept
{
#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    struct sigevent event{};
    event.sigev_notify = SIGEV_SIGNAL;
    event.sigev_signo = SIGUSR1;
    event.sigev_value.sival_ptr = nullptr;
    if (::timer_create(CLOCK_MONOTONIC, &event, &timer_) != 0)
    {
        return castle::status::system_call_error;
    }
    created_ = true;
    arm(true);
    return castle::status::ok;
#else
    return castle::status::unsupported;
#endif
}

castle::status PosixTickSource::deinit() noexcept
{
    return shutdown();
}

castle::status PosixTickSource::suspend() noexcept
{
    if (!created_ || suspended_)
    {
        return castle::status::ok;
    }
    arm(false);
    suspended_ = true;
    return castle::status::ok;
}

castle::status PosixTickSource::resume() noexcept
{
    if (!created_ || !suspended_)
    {
        return castle::status::ok;
    }
    suspended_ = false;
    arm(true);
    return castle::status::ok;
}

castle::status PosixTickSource::shutdown() noexcept
{
#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    if (created_)
    {
        arm(false);
        const int result = ::timer_delete(timer_);
        created_ = false;
        suspended_ = false;
        return result == 0 ? castle::status::ok : castle::status::system_call_error;
    }
#endif
    return castle::status::ok;
}

bool PosixTickSource::is_open() const noexcept
{
    return created_;
}

void PosixTickSource::arm(bool enabled) noexcept
{
#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    struct itimerspec spec{};
    if (enabled)
    {
        spec.it_value.tv_sec = 0;
        spec.it_value.tv_nsec = 10L * 1000L * 1000L;
        spec.it_interval.tv_sec = 0;
        spec.it_interval.tv_nsec = 10L * 1000L * 1000L;
    }
    static_cast<void>(::timer_settime(timer_, 0, &spec, nullptr));
#else
    static_cast<void>(enabled);
#endif
}

uint32_t monotonic_milliseconds() noexcept
{
    struct timespec now{};
    if (::clock_gettime(CLOCK_MONOTONIC, &now) != 0)
    {
        return 0U;
    }
    const uint64_t millis = (static_cast<uint64_t>(now.tv_sec) * 1000ULL) +
                            (static_cast<uint64_t>(now.tv_nsec) / 1000000ULL);
    return static_cast<uint32_t>(millis & 0xFFFFFFFFULL);
}

} // namespace gnss
