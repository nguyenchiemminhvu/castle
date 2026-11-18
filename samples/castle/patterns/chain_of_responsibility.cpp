#include "sample_support.hpp"

#include "castle/patterns/chain_of_responsibility.hpp"
#include "castle/events/sigslot.hpp"

#include <stdint.h>

namespace dp = castle::patterns;

namespace
{
enum class command : uint8_t
{
    status,
    open,
    close,
    unknown
};

struct request
{
    command value;
};

struct counters
{
    uint32_t status_requests = 0U;
    uint32_t door_operations = 0U;
    uint32_t audited = 0U;
    uint32_t fallback = 0U;
    uint32_t executed = 0U;
};

bool is_command(command expected, request const& req)
{
    return req.value == expected;
}
} // namespace

int main()
{
    // 1. Dispatch chain: skip handlers pass on a false predicate and end the chain with stop_chain.
    //    A void action never ends the chain, so the audit step runs before dispatching.
    auto chain = dp::make_chain(
        dp::make_default_handler([](request const&, counters& state) noexcept { ++state.audited; }),
        dp::make_skippable_handler(
            [](request const& req, counters&) noexcept { return is_command(command::status, req); },
            [](request const&, counters& state) noexcept
            {
                ++state.status_requests;
                return dp::handler_flow::stop_chain;
            }),
        dp::make_skippable_handler(
            [](request const& req, counters&) noexcept
            {
                return is_command(command::open, req) || is_command(command::close, req);
            },
            [](request const&, counters& state) noexcept
            {
                ++state.door_operations;
                return dp::handler_flow::stop_chain;
            }),
        dp::make_default_handler(
            [](request const&, counters& state) noexcept
            {
                ++state.fallback;
                return dp::handler_flow::stop_chain;
            }));

    static_assert(decltype(chain)::capacity() == 4U, "chain capacity is a compile-time constant");

    counters state;
    request req{command::status};
    CASTLE_SAMPLE_CHECK(chain.process(req, state) == dp::chain_result::stopped);
    req.value = command::open;
    CASTLE_SAMPLE_CHECK(chain.process(req, state) == dp::chain_result::stopped);
    req.value = command::unknown;
    CASTLE_SAMPLE_CHECK(chain.process(req, state) == dp::chain_result::stopped);
    CASTLE_SAMPLE_CHECK(state.audited == 3U);
    CASTLE_SAMPLE_CHECK(state.status_requests == 1U);
    CASTLE_SAMPLE_CHECK(state.door_operations == 1U);
    CASTLE_SAMPLE_CHECK(state.fallback == 1U);

    // 2. Guard chain: make_handler() ends the chain with `rejected` when its predicate is false, so
    //    the predicate gates every later handler. Void actions let the chain run to the end.
    counters guard_state;
    auto guarded = dp::make_chain(
        dp::make_default_handler([](request const&, counters& s) noexcept { ++s.audited; }),
        dp::make_handler(
            [](request const& req, counters&) noexcept { return !is_command(command::unknown, req); },
            [](request const&, counters& s) noexcept { ++s.executed; }),
        dp::make_default_handler([](request const&, counters& s) noexcept { ++s.status_requests; }));

    req.value = command::open;
    CASTLE_SAMPLE_CHECK(guarded.process(req, guard_state) == dp::chain_result::completed);
    req.value = command::unknown;
    CASTLE_SAMPLE_CHECK(guarded.process(req, guard_state) == dp::chain_result::rejected);
    CASTLE_SAMPLE_CHECK(guard_state.audited == 2U);
    CASTLE_SAMPLE_CHECK(guard_state.executed == 1U);
    CASTLE_SAMPLE_CHECK(guard_state.status_requests == 1U);

    // 3. A chain is a callable: a copy plugs into a signal slot (the signal broadcasts, the chain short-circuits).
    counters signal_state;
    castle::sigslot::signal<2U, void(request&, counters&)> incoming;
    auto connection = incoming.connect(chain);
    CASTLE_SAMPLE_CHECK(connection.connected());
    req.value = command::close;
    incoming.emit(req, signal_state);
    CASTLE_SAMPLE_CHECK(signal_state.audited == 1U);
    CASTLE_SAMPLE_CHECK(signal_state.door_operations == 1U);

    // 4. Bounded chain: up to 3 handlers registered at runtime, storage inline, no allocation.
    counters bounded_state;
    dp::bounded_chain_of_responsibility<3U, void(request const&, counters&)> bounded;
    CASTLE_SAMPLE_CHECK(bounded.add_skip_handler(
        [](request const& r, counters&) noexcept { return is_command(command::status, r); },
        [](request const&, counters& s) noexcept
        {
            ++s.status_requests;
            return dp::handler_flow::stop_chain;
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(bounded.add_default_handler(
        [](request const&, counters& s) noexcept { ++s.fallback; }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(bounded.size() == 2U);

    req.value = command::status;
    CASTLE_SAMPLE_CHECK(bounded.process(req, bounded_state) == dp::chain_result::stopped);
    req.value = command::open;
    CASTLE_SAMPLE_CHECK(bounded.process(req, bounded_state) == dp::chain_result::completed);
    CASTLE_SAMPLE_CHECK(bounded_state.status_requests == 1U);
    CASTLE_SAMPLE_CHECK(bounded_state.fallback == 1U);

    return 0;
}
