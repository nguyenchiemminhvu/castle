#include "gnss/components.hpp"

namespace gnss
{

castle::status LocationService::init() noexcept
{
    if (initialized_)
    {
        return castle::status::already_exists;
    }
    const castle::status result = graph_.init();
    initialized_ = (result == castle::status::ok);
    suspended_ = false;
    return result;
}

void LocationService::update(uint32_t now_ms, uint32_t elapsed_ticks) noexcept
{
    if (initialized_ && !suspended_)
    {
        graph_.get<TickEngine>().tick(now_ms, elapsed_ticks);
    }
}

castle::status LocationService::suspend() noexcept
{
    if (!initialized_)
    {
        return castle::status::not_configured;
    }
    if (suspended_)
    {
        return castle::status::ok;
    }
    const castle::status result = graph_.objects().for_each_child(
        castle::patterns::reverse_order,
        [](auto& component) noexcept
        {
            return component.suspend();
        });
    suspended_ = (result == castle::status::ok);
    return result;
}

castle::status LocationService::resume() noexcept
{
    if (!initialized_)
    {
        return castle::status::not_configured;
    }
    if (!suspended_)
    {
        return castle::status::invalid_state;
    }
    const castle::status result = graph_.objects().for_each_child(
        castle::patterns::forward_order,
        [](auto& component) noexcept
        {
            return component.resume();
        });
    suspended_ = (result != castle::status::ok);
    return result;
}

castle::status LocationService::shutdown() noexcept
{
    if (!initialized_)
    {
        return castle::status::ok;
    }
    const castle::status stopped = graph_.objects().for_each_child(
        castle::patterns::reverse_order,
        [](auto& component) noexcept
        {
            return component.shutdown();
        });
    const castle::status deinitialized = graph_.deinit();
    initialized_ = false;
    suspended_ = false;
    return stopped != castle::status::ok ? stopped : deinitialized;
}

GnssTransConfig& LocationService::config() noexcept
{
    return graph_.get<GnssTransConfig>();
}

UbxController& LocationService::controller() noexcept
{
    return graph_.get<UbxController>();
}

PosixTickSource& LocationService::tick_source() noexcept
{
    return graph_.get<PosixTickSource>();
}

} // namespace gnss
