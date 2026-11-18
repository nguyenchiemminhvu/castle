#include <gtest/gtest.h>

#include "castle/patterns/chain_of_responsibility.hpp"
#include "castle/callbacks/function.hpp"

#include <stdint.h>

namespace dp = castle::patterns;

namespace
{
struct request
{
    int code;
};

struct trace
{
    int calls[8] = {};
    int count = 0;

    void record(int id) noexcept
    {
        calls[count++] = id;
    }
};

bool accepts_seven(request const& req, trace&) noexcept
{
    return req.code == 7;
}

void handle_one(request const&, trace& log) noexcept
{
    log.record(1);
}

dp::handler_flow pass_two(request const&, trace& log) noexcept
{
    log.record(2);
    return dp::handler_flow::continue_chain;
}

dp::handler_flow stop_three(request const&, trace& log) noexcept
{
    log.record(3);
    return dp::handler_flow::stop_chain;
}

bool always(request const&, trace&) noexcept
{
    return true;
}

struct counting_predicate
{
    int* evaluations;
    bool result;

    bool operator()(request const&, trace&) const noexcept
    {
        ++(*evaluations);
        return result;
    }
};

TEST(ChainOfResponsibilityTest, VoidActionContinuesToNextHandler)
{
    auto chain = dp::make_chain(
        dp::make_handler(&accepts_seven, &handle_one),
        dp::make_default_handler([](request const&, trace& log) noexcept { log.record(9); }));

    request req{7};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.calls[0], 1);
    EXPECT_EQ(log.calls[1], 9);
}

TEST(ChainOfResponsibilityTest, FalsePredicateStopsChainByDefault)
{
    int evaluations = 0;
    auto chain = dp::make_chain(
        dp::make_handler(&accepts_seven, &handle_one),
        dp::make_handler(counting_predicate{&evaluations, true}, &handle_one),
        dp::make_default_handler([](request const&, trace& log) noexcept { log.record(9); }));

    request req{3};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::rejected);
    EXPECT_EQ(log.count, 0);
    EXPECT_EQ(evaluations, 0);
}

TEST(ChainOfResponsibilityTest, HandlersBeforeTheRejectingPredicateStillRun)
{
    auto chain = dp::make_chain(
        dp::make_default_handler([](request const&, trace& log) noexcept { log.record(9); }),
        dp::make_handler(&accepts_seven, &handle_one),
        dp::make_default_handler([](request const&, trace& log) noexcept { log.record(8); }));

    request req{3};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::rejected);
    ASSERT_EQ(log.count, 1);
    EXPECT_EQ(log.calls[0], 9);
}

TEST(ChainOfResponsibilityTest, SkipHandlerFallsThroughOnFalsePredicate)
{
    auto chain = dp::make_chain(
        dp::make_skippable_handler(&accepts_seven, &stop_three),
        dp::make_default_handler([](request const&, trace& log) noexcept { log.record(9); }));

    request req{3};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    ASSERT_EQ(log.count, 1);
    EXPECT_EQ(log.calls[0], 9);
}

TEST(ChainOfResponsibilityTest, StopFlowEndsChainAndSkipsRemainingHandlers)
{
    int evaluations = 0;
    auto chain = dp::make_chain(
        dp::make_skippable_handler(&accepts_seven, &stop_three),
        dp::make_skippable_handler(counting_predicate{&evaluations, true}, &handle_one));

    request req{7};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::stopped);
    ASSERT_EQ(log.count, 1);
    EXPECT_EQ(log.calls[0], 3);
    EXPECT_EQ(evaluations, 0);
}

TEST(ChainOfResponsibilityTest, FirstMatchingSkipHandlerDispatches)
{
    auto chain = dp::make_chain(
        dp::make_skippable_handler([](request const& r, trace&) noexcept { return r.code == 1; },
                              [](request const&, trace& log) noexcept
                              {
                                  log.record(1);
                                  return dp::handler_flow::stop_chain;
                              }),
        dp::make_skippable_handler([](request const& r, trace&) noexcept { return r.code == 2; },
                              [](request const&, trace& log) noexcept
                              {
                                  log.record(2);
                                  return dp::handler_flow::stop_chain;
                              }),
        dp::make_default_handler([](request const&, trace& log) noexcept
                                 {
                                     log.record(9);
                                     return dp::handler_flow::stop_chain;
                                 }));

    trace log;
    request second{2};
    EXPECT_EQ(chain.process(second, log), dp::chain_result::stopped);
    request other{5};
    EXPECT_EQ(chain.process(other, log), dp::chain_result::stopped);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.calls[0], 2);
    EXPECT_EQ(log.calls[1], 9);
}

TEST(ChainOfResponsibilityTest, ContinueFlowRunsLaterHandlersInDeclarationOrder)
{
    auto chain = dp::make_chain(
        dp::make_handler(&always, &pass_two),
        dp::make_handler(&accepts_seven, &handle_one));

    request req{7};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.calls[0], 2);
    EXPECT_EQ(log.calls[1], 1);
}

TEST(ChainOfResponsibilityTest, EachPredicateIsEvaluatedAtMostOnce)
{
    int first = 0;
    int second = 0;
    auto chain = dp::make_chain(
        dp::make_skippable_handler(counting_predicate{&first, false}, &handle_one),
        dp::make_handler(counting_predicate{&second, true}, &handle_one));

    request req{0};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 1);
}

TEST(ChainOfResponsibilityTest, HandlerPoliciesAreVisibleInTheType)
{
    using guard = decltype(dp::make_handler(&always, &handle_one));
    using filter = decltype(dp::make_skippable_handler(&always, &handle_one));
    using fallback = decltype(dp::make_default_handler(&handle_one));
    static_assert(guard::on_reject == dp::reject_policy::stop, "make_handler stops on false");
    static_assert(filter::on_reject == dp::reject_policy::skip, "make_skippable_handler skips on false");
    static_assert(fallback::on_reject == dp::reject_policy::stop, "default handler never rejects");
    SUCCEED();
}

TEST(ChainOfResponsibilityTest, SingleArgumentAndTemporaryArgumentsAreAccepted)
{
    int total = 0;
    auto chain = dp::make_chain(
        dp::make_handler([](int value) noexcept { return value > 5; },
                         [&total](int value) noexcept { total += value; }));

    EXPECT_EQ(chain.process(10), dp::chain_result::completed);
    EXPECT_EQ(chain.process(1), dp::chain_result::rejected);
    EXPECT_EQ(total, 10);
}

TEST(ChainOfResponsibilityTest, ZeroArgumentChainWorks)
{
    int calls = 0;
    auto chain = dp::make_chain(dp::make_default_handler([&calls]() noexcept { ++calls; }));
    EXPECT_EQ(chain.process(), dp::chain_result::completed);
    EXPECT_EQ(calls, 1);
}

TEST(ChainOfResponsibilityTest, CapacityIsCompileTimeHandlerCount)
{
    using handler_a = dp::conditional_handler<bool (*)(request const&, trace&), void (*)(request const&, trace&)>;
    using chain_type = dp::chain_of_responsibility<handler_a, handler_a, handler_a>;
    static_assert(chain_type::capacity() == 3U, "capacity");
    static_assert(chain_type::handler_count == 3U, "handler_count");
    EXPECT_EQ(chain_type::capacity(), 3U);
}

TEST(ChainOfResponsibilityTest, DefaultConstructedChainWithStatelessHandlersAndHandlerAccess)
{
    using handler_type = dp::conditional_handler<dp::always_true, void (*)(request const&, trace&)>;
    dp::chain_of_responsibility<handler_type> chain;

    chain.handler_at<0U>().action() = &handle_one;
    request req{0};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_EQ(log.count, 1);
    EXPECT_TRUE(chain.handler_at<0U>().predicate()(req, log));
}

TEST(ChainOfResponsibilityTest, CopiedChainKeepsHandlersIndependent)
{
    int calls = 0;
    auto original = dp::make_chain(dp::make_default_handler([&calls]() noexcept { ++calls; }));
    auto copy = original;

    original.process();
    copy.process();
    EXPECT_EQ(calls, 2);
}

TEST(ChainOfResponsibilityTest, CallOperatorMapsResultToFlow)
{
    auto stopping = dp::make_chain(dp::make_skippable_handler(
        [](int value) noexcept { return value == 1; },
        [](int) noexcept { return dp::handler_flow::stop_chain; }));
    EXPECT_EQ(stopping(1), dp::handler_flow::stop_chain);
    EXPECT_EQ(stopping(2), dp::handler_flow::continue_chain);

    auto guard = dp::make_chain(dp::make_handler([](int value) noexcept { return value == 1; },
                                                 [](int) noexcept {}));
    EXPECT_EQ(guard(1), dp::handler_flow::continue_chain);
    EXPECT_EQ(guard(2), dp::handler_flow::stop_chain);
}

TEST(ChainOfResponsibilityTest, ChainCanBeTheActionOfAnotherChain)
{
    int inner_calls = 0;
    int outer_calls = 0;
    auto inner = dp::make_chain(dp::make_handler([](int value) noexcept { return value == 2; },
                                                 [&inner_calls](int) noexcept { ++inner_calls; }));
    auto outer = dp::make_chain(
        dp::make_default_handler(inner),
        dp::make_default_handler([&outer_calls](int) noexcept { ++outer_calls; }));

    EXPECT_EQ(outer.process(2), dp::chain_result::completed);
    EXPECT_EQ(inner_calls, 1);
    EXPECT_EQ(outer_calls, 1);

    // The inner chain rejects 3, which stops the outer chain before its second handler.
    EXPECT_EQ(outer.process(3), dp::chain_result::stopped);
    EXPECT_EQ(inner_calls, 1);
    EXPECT_EQ(outer_calls, 1);
}

TEST(ChainOfResponsibilityTest, AlwaysTrueAcceptsAnyArguments)
{
    dp::always_true predicate;
    EXPECT_TRUE(predicate());
    EXPECT_TRUE(predicate(1, 2.0, request{3}));
}

using bounded_type = dp::bounded_chain_of_responsibility<3U, void(request const&, trace&)>;

TEST(BoundedChainTest, EmptyChainIsCompleted)
{
    bounded_type chain;
    request req{0};
    trace log;
    EXPECT_TRUE(chain.empty());
    EXPECT_EQ(chain.size(), 0U);
    EXPECT_EQ(bounded_type::capacity(), 3U);
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
}

TEST(BoundedChainTest, HandlersRunInRegistrationOrderUntilStop)
{
    bounded_type chain;
    ASSERT_EQ(chain.add_handler(&always, &pass_two), castle::status::ok);
    ASSERT_EQ(chain.add_skip_handler(&accepts_seven, &stop_three), castle::status::ok);
    ASSERT_EQ(chain.add_default_handler([](request const&, trace& log) noexcept { log.record(9); }),
              castle::status::ok);
    EXPECT_EQ(chain.size(), 3U);
    EXPECT_FALSE(chain.empty());

    request req{7};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::stopped);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.calls[0], 2);
    EXPECT_EQ(log.calls[1], 3);

    request other{1};
    trace other_log;
    EXPECT_EQ(chain.process(other, other_log), dp::chain_result::completed);
    ASSERT_EQ(other_log.count, 2);
    EXPECT_EQ(other_log.calls[0], 2);
    EXPECT_EQ(other_log.calls[1], 9);
}

TEST(BoundedChainTest, FalsePredicateStopsChainByDefault)
{
    bounded_type chain;
    ASSERT_EQ(chain.add_handler(&accepts_seven, &handle_one), castle::status::ok);
    ASSERT_EQ(chain.add_default_handler(&handle_one), castle::status::ok);

    request req{1};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::rejected);
    EXPECT_EQ(log.count, 0);

    request match{7};
    EXPECT_EQ(chain.process(match, log), dp::chain_result::completed);
    EXPECT_EQ(log.count, 2);
}

TEST(BoundedChainTest, CompletedWhenEveryHandlerSkipsOrContinues)
{
    bounded_type chain;
    ASSERT_EQ(chain.add_skip_handler(&accepts_seven, &handle_one), castle::status::ok);
    ASSERT_EQ(chain.add_handler(&always, &pass_two), castle::status::ok);

    request req{1};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    ASSERT_EQ(log.count, 1);
    EXPECT_EQ(log.calls[0], 2);
}

TEST(BoundedChainTest, FullChainRejectsAnotherHandler)
{
    bounded_type chain;
    for (int i = 0; i < 3; ++i)
    {
        ASSERT_EQ(chain.add_handler(&accepts_seven, &handle_one), castle::status::ok);
    }
    EXPECT_EQ(chain.add_handler(&accepts_seven, &handle_one), castle::status::full);
    EXPECT_EQ(chain.add_default_handler(&handle_one), castle::status::full);
    EXPECT_EQ(chain.size(), 3U);
}

TEST(BoundedChainTest, NullFunctionPointersAreRejected)
{
    bounded_type chain;
    bool (*null_predicate)(request const&, trace&) = nullptr;
    void (*null_action)(request const&, trace&) = nullptr;

    EXPECT_EQ(chain.add_handler(null_predicate, &handle_one), castle::status::invalid_callback);
    EXPECT_EQ(chain.add_handler(&accepts_seven, null_action), castle::status::invalid_callback);
    EXPECT_TRUE(chain.empty());
}

TEST(BoundedChainTest, ClearRemovesHandlersAndAllowsReuse)
{
    bounded_type chain;
    ASSERT_EQ(chain.add_handler(&accepts_seven, &handle_one), castle::status::ok);
    EXPECT_EQ(chain.clear(), castle::status::ok);
    EXPECT_TRUE(chain.empty());

    request req{7};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_EQ(chain.add_handler(&accepts_seven, &handle_one), castle::status::ok);
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_EQ(log.count, 1);
}

TEST(BoundedChainTest, ConfigurationIsRejectedWhileProcessing)
{
    bounded_type chain;
    castle::status add_result = castle::status::ok;
    castle::status clear_result = castle::status::ok;
    bool processing = false;

    ASSERT_EQ(chain.add_default_handler(
                  [&](request const&, trace&) noexcept
                  {
                      processing = chain.is_processing();
                      add_result = chain.add_default_handler(&handle_one);
                      clear_result = chain.clear();
                  }),
              castle::status::ok);

    request req{0};
    trace log;
    EXPECT_FALSE(chain.is_processing());
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_TRUE(processing);
    EXPECT_EQ(add_result, castle::status::invalid_config);
    EXPECT_EQ(clear_result, castle::status::invalid_config);
    EXPECT_FALSE(chain.is_processing());
    EXPECT_EQ(chain.size(), 1U);
}

TEST(BoundedChainTest, CallOperatorMapsResultToFlowAndChainsNest)
{
    dp::bounded_chain_of_responsibility<2U, void(int)> inner;
    int inner_calls = 0;
    ASSERT_EQ(inner.add_handler([](int value) noexcept { return value == 5; },
                                [&inner_calls](int) noexcept { ++inner_calls; }),
              castle::status::ok);
    EXPECT_EQ(inner(5), dp::handler_flow::continue_chain);
    EXPECT_EQ(inner(6), dp::handler_flow::stop_chain);
    EXPECT_EQ(inner_calls, 1);

    dp::bounded_chain_of_responsibility<2U, void(int)> outer;
    int outer_calls = 0;
    // A bounded chain is too large to copy into inline storage; reference it instead.
    ASSERT_EQ(outer.add_default_handler([&inner](int value) noexcept { return inner(value); }),
              castle::status::ok);
    ASSERT_EQ(outer.add_default_handler([&outer_calls](int) noexcept { ++outer_calls; }),
              castle::status::ok);
    EXPECT_EQ(outer.process(5), dp::chain_result::completed);
    EXPECT_EQ(outer_calls, 1);
    EXPECT_EQ(outer.process(6), dp::chain_result::stopped);
    EXPECT_EQ(outer_calls, 1);
    EXPECT_EQ(inner_calls, 2);
}

TEST(BoundedChainTest, HandlersOfDifferentCallableTypesShareOneChain)
{
    int captured = 0;
    bounded_type chain;
    ASSERT_EQ(chain.add_handler(&accepts_seven, [&captured](request const& req, trace&) noexcept
                                {
                                    captured = req.code;
                                    return dp::handler_flow::continue_chain;
                                }),
              castle::status::ok);
    ASSERT_EQ(chain.add_handler(counting_predicate{&captured, true}, &handle_one), castle::status::ok);

    request req{7};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_EQ(captured, 8); // action stored 7, second predicate incremented it
    EXPECT_EQ(log.count, 1);
}

TEST(BoundedChainTest, EmptyFunctionWrappersAreRejected)
{
    bounded_type chain;
    castle::callbacks::function<bool(request const&, trace&), 16U> empty_predicate;
    castle::callbacks::function<void(request const&, trace&), 16U> empty_action;
    castle::callbacks::function<void(request const&, trace&), 16U> action{&handle_one};

    EXPECT_EQ(chain.add_handler(empty_predicate, &handle_one), castle::status::invalid_callback);
    EXPECT_EQ(chain.add_handler(&accepts_seven, empty_action), castle::status::invalid_callback);
    EXPECT_EQ(chain.add_default_handler(action), castle::status::ok);

    request req{0};
    trace log;
    EXPECT_EQ(chain.process(req, log), dp::chain_result::completed);
    EXPECT_EQ(log.count, 1);
}

TEST(BoundedChainTest, ValueParametersAreSharedByEveryHandler)
{
    dp::bounded_chain_of_responsibility<2U, void(int)> chain;
    int seen = 0;
    ASSERT_EQ(chain.add_handler([&seen](int value) noexcept { seen += value; return true; },
                                [](int) noexcept { return dp::handler_flow::continue_chain; }),
              castle::status::ok);
    ASSERT_EQ(chain.add_handler([&seen](int value) noexcept { seen += value; return false; },
                                [](int) noexcept {}),
              castle::status::ok);
    EXPECT_EQ(chain.process(4), dp::chain_result::rejected);
    EXPECT_EQ(seen, 8);
}
} // namespace
