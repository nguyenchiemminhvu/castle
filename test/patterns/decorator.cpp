#include <gtest/gtest.h>

#include "castle/patterns/decorator.hpp"
#include "castle/callbacks/exec_policy.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

namespace dp = castle::patterns;
namespace cb = castle::callbacks;

namespace
{
struct trace
{
    int ids[16] = {};
    int count = 0;

    void record(int id) noexcept
    {
        ids[count++] = id;
    }
};

int triple(int value) noexcept
{
    return value * 3;
}

int g_plain_calls = 0;

void plain_target() noexcept
{
    ++g_plain_calls;
}

// Records `id` on entry and `-id` on exit, so nesting order is visible in a trace.
struct tracing_decorator
{
    trace* log;
    int id;

    template <typename Next, typename... Args>
    decltype(auto) operator()(Next& next, Args&&... args)
    {
        log->record(id);
        decltype(auto) result = next(castle::forward<Args>(args)...);
        log->record(-id);
        return result;
    }
};

struct counting_decorator
{
    int calls = 0;

    template <typename Next>
    int operator()(Next& next, int value)
    {
        ++calls;
        return next(value);
    }
};

struct move_only_target
{
    int base;

    explicit move_only_target(int b) noexcept : base(b) {}
    move_only_target(move_only_target&&) noexcept = default;
    move_only_target(move_only_target const&) = delete;

    int operator()(int value) const noexcept
    {
        return base + value;
    }
};

struct move_probe
{
    int copies = 0;
    int moves = 0;

    move_probe() noexcept = default;
    move_probe(move_probe const& other) noexcept : copies(other.copies + 1), moves(other.moves) {}
    move_probe(move_probe&& other) noexcept : copies(other.copies), moves(other.moves + 1) {}
};

constexpr int twice(int value) noexcept
{
    return value * 2;
}

struct add_one_after
{
    template <typename Next>
    constexpr int operator()(Next& next, int value) const noexcept
    {
        return next(value) + 1;
    }
};
} // namespace

TEST(Decorator, RunsCodeBeforeAndAfterWrappedFunction)
{
    trace log;
    cb::function<int(int)> target = [&log](int value) noexcept
    {
        log.record(100);
        return value + 1;
    };

    auto wrapped = dp::decorate(target, tracing_decorator{&log, 1});

    EXPECT_EQ(wrapped(4), 5);
    ASSERT_EQ(log.count, 3);
    EXPECT_EQ(log.ids[0], 1);
    EXPECT_EQ(log.ids[1], 100);
    EXPECT_EQ(log.ids[2], -1);
}

TEST(Decorator, CanRewriteArgumentsAndResult)
{
    auto wrapped = dp::decorate(&triple,
                                [](auto& next, int value) noexcept
                                {
                                    int const clamped = value < 0 ? 0 : value;
                                    return next(clamped) + 1000;
                                });

    EXPECT_EQ(wrapped(-5), 1000);
    EXPECT_EQ(wrapped(2), 1006);
}

TEST(Decorator, MayShortCircuitWithoutCallingTarget)
{
    int target_calls = 0;
    auto guarded = dp::decorate([&target_calls](int value) noexcept
                                {
                                    ++target_calls;
                                    return value;
                                },
                                [](auto& next, int value) noexcept
                                {
                                    return value == 0 ? -1 : next(value);
                                });

    EXPECT_EQ(guarded(0), -1);
    EXPECT_EQ(target_calls, 0);
    EXPECT_EQ(guarded(9), 9);
    EXPECT_EQ(target_calls, 1);
}

TEST(Decorator, MayCallTargetSeveralTimes)
{
    int attempts = 0;
    auto flaky = [&attempts](int value) noexcept
    {
        ++attempts;
        return attempts < 3 ? -1 : value;
    };

    auto retried = dp::decorate(flaky,
                                [](auto& next, int value) noexcept
                                {
                                    int result = -1;
                                    for (int i = 0; i < 5 && result < 0; ++i)
                                    {
                                        result = next(value);
                                    }
                                    return result;
                                });

    EXPECT_EQ(retried(7), 7);
    EXPECT_EQ(attempts, 3);
}

TEST(Decorator, FirstDecoratorIsOutermostLayer)
{
    trace log;
    auto target = [&log](int value) noexcept
    {
        log.record(100);
        return value;
    };

    auto stacked = dp::decorate(target,
                                tracing_decorator{&log, 1},
                                tracing_decorator{&log, 2},
                                tracing_decorator{&log, 3});

    using nested = decltype(dp::decorate(dp::decorate(dp::decorate(target, tracing_decorator{&log, 3}),
                                                      tracing_decorator{&log, 2}),
                                         tracing_decorator{&log, 1}));
    static_assert(castle::meta::is_same<decltype(stacked), nested>::value,
                  "decorate(t, a, b, c) must nest as decorate(decorate(decorate(t, c), b), a)");

    EXPECT_EQ(stacked(8), 8);
    int const expected[] = {1, 2, 3, 100, -3, -2, -1};
    ASSERT_EQ(log.count, 7);
    for (int i = 0; i < 7; ++i)
    {
        EXPECT_EQ(log.ids[i], expected[i]);
    }

    EXPECT_EQ(stacked.decorator().id, 1);
    EXPECT_EQ(stacked.target().decorator().id, 2);
    EXPECT_EQ(stacked.target().target().decorator().id, 3);
}

TEST(Decorator, FunctionReferenceTargetDecaysToPointer)
{
    auto wrapped = dp::decorate(triple, counting_decorator{});

    static_assert(castle::meta::is_same<decltype(wrapped)::target_type, int (*)(int) noexcept>::value,
                  "function reference target must decay to a function pointer");
    EXPECT_EQ(wrapped.target(), &triple);
    EXPECT_EQ(wrapped(5), 15);
    EXPECT_EQ(wrapped.decorator().calls, 1);
}

TEST(Decorator, SupportsVoidTargets)
{
    g_plain_calls = 0;
    int before = 0;
    auto wrapped = dp::decorate(plain_target,
                                [&before](auto& next) noexcept
                                {
                                    ++before;
                                    next();
                                });

    wrapped();
    wrapped();

    EXPECT_EQ(before, 2);
    EXPECT_EQ(g_plain_calls, 2);
}

TEST(Decorator, PreservesReferenceResults)
{
    int slots[2] = {10, 20};
    auto select = [&slots](int index) noexcept -> int&
    {
        return slots[index];
    };

    auto wrapped = dp::decorate(select,
                                [](auto& next, int index) noexcept -> decltype(auto)
                                {
                                    return next(index > 1 ? 1 : index);
                                });

    static_assert(castle::meta::is_same<decltype(wrapped(0)), int&>::value,
                  "decorated must return the decorator's result type unchanged");
    wrapped(5) = 99;
    EXPECT_EQ(slots[0], 10);
    EXPECT_EQ(slots[1], 99);
}

TEST(Decorator, ForwardsRvalueArgumentsToDecorator)
{
    auto wrapped = dp::decorate([](move_probe probe) noexcept
                                {
                                    return probe;
                                },
                                [](auto& next, move_probe probe) noexcept
                                {
                                    return next(castle::move(probe));
                                });

    move_probe source;
    move_probe const result = wrapped(castle::move(source));

    EXPECT_EQ(result.copies, 0);
    EXPECT_GE(result.moves, 1);

    move_probe const copied = wrapped(source);
    EXPECT_EQ(copied.copies, 1);
}

TEST(Decorator, MovesMoveOnlyTargetIntoWrapper)
{
    auto wrapped = dp::decorate(move_only_target{40}, counting_decorator{});
    auto moved = castle::move(wrapped);

    EXPECT_EQ(moved(2), 42);
    EXPECT_EQ(moved.target().base, 40);
    EXPECT_EQ(moved.decorator().calls, 1);
}

TEST(Decorator, CopiesOwnIndependentDecoratorState)
{
    auto original = dp::decorate(&triple, counting_decorator{});
    original(1);

    auto copy = original;
    copy(1);
    copy(1);

    EXPECT_EQ(original.decorator().calls, 1);
    EXPECT_EQ(copy.decorator().calls, 3);
}

TEST(Decorator, TargetAccessorRebindsWrappedFunction)
{
    cb::function<int(int)> target = [](int value) noexcept { return value; };
    auto wrapped = dp::decorate(castle::move(target), counting_decorator{});

    EXPECT_EQ(wrapped(3), 3);
    wrapped.target() = &triple;
    EXPECT_EQ(wrapped(3), 9);
    EXPECT_EQ(wrapped.decorator().calls, 2);
}

TEST(Decorator, ErasesIntoCastleFunctionAndDecoratesAgain)
{
    trace log;
    cb::function<int(int)> target = [](int value) noexcept { return value * 10; };
    auto layered = dp::decorate(target, tracing_decorator{&log, 1});

    cb::function<int(int), sizeof(layered)> erased = layered;
    EXPECT_EQ(erased(2), 20);

    auto outer = dp::decorate(erased, tracing_decorator{&log, 2});
    EXPECT_EQ(outer(3), 30);

    int const expected[] = {1, -1, 2, 1, -1, -2};
    ASSERT_EQ(log.count, 6);
    for (int i = 0; i < 6; ++i)
    {
        EXPECT_EQ(log.ids[i], expected[i]);
    }
}

TEST(Decorator, ComposesWithExecutionPolicies)
{
    int fired = 0;
    int seen = 0;
    auto count_seen = [&seen](auto& next, int value) noexcept
    {
        ++seen;
        next(value);
    };

    // Policy as the target: the decorator sees every call, the policy gates the callback.
    auto gated = dp::decorate(cb::policy::make_every_n(2U, [&fired](int value) noexcept { fired += value; }),
                              count_seen);
    gated(1);
    gated(2);
    gated(3);
    gated(4);
    EXPECT_EQ(seen, 4);
    EXPECT_EQ(fired, 6);

    // Decorated callable inside a policy: the whole stack runs at most once.
    seen = 0;
    fired = 0;
    auto once = cb::policy::make_once(dp::decorate([&fired](int value) noexcept { fired += value; }, count_seen));
    once(5);
    once(6);
    EXPECT_EQ(seen, 1);
    EXPECT_EQ(fired, 5);
}

TEST(Decorator, GenericDecoratorIsReusableAcrossSignatures)
{
    int calls = 0;
    auto count_calls = [&calls](auto& next, auto&&... args) -> decltype(auto)
    {
        ++calls;
        return next(castle::forward<decltype(args)>(args)...);
    };

    auto unary = dp::decorate(&triple, count_calls);
    auto binary = dp::decorate([](int a, int b) noexcept { return a - b; }, count_calls);

    EXPECT_EQ(unary(2), 6);
    EXPECT_EQ(binary(9, 4), 5);
    EXPECT_EQ(calls, 2);
}

TEST(Decorator, IsUsableInConstantExpressions)
{
    static_assert(dp::decorate(twice, add_one_after{})(3) == 7, "single layer must be constexpr");
    static_assert(dp::decorate(twice, add_one_after{}, add_one_after{})(3) == 8, "stack must be constexpr");

    auto runtime = dp::decorate(twice, add_one_after{});
    EXPECT_EQ(runtime(5), 11);
}
