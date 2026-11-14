#include <gtest/gtest.h>

#include "castle/patterns/composite.hpp"
#include "castle/core/traits.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

namespace patterns = castle::patterns;

namespace
{
struct first_component
{
    void run() noexcept
    {
        ++calls;
    }

    int calls = 0;
};

struct second_component
{
    void run() noexcept
    {
        calls += 2;
    }

    int calls = 0;
};

struct third_component
{
    void run() noexcept
    {
        calls += 3;
    }

    int calls = 0;
};

struct move_only_component
{
    explicit move_only_component(int value) noexcept
        : value(value)
    {
    }

    move_only_component(move_only_component&& other) noexcept
        : value(other.value)
    {
        other.value = -1;
    }

    move_only_component(move_only_component const&) = delete;
    move_only_component& operator=(move_only_component const&) = delete;

    int value;
};

struct copy_probe
{
    explicit copy_probe(int value) noexcept
        : value(value)
    {
    }

    int value;
};

struct trace_log
{
    void push(int id) noexcept
    {
        ids[count++] = id;
    }

    int ids[8] = {};
    int count = 0;
};

template <int Id>
struct traced_component
{
    explicit traced_component(trace_log& log, castle::status result = castle::status::ok) noexcept
        : log(&log)
        , result(result)
    {
    }

    castle::status stop() noexcept
    {
        log->push(Id);
        return result;
    }

    trace_log* log;
    castle::status result;
};

} // namespace

TEST(Composite, EmptyGroupHasNoChildrenOrLeaves)
{
    patterns::composite<> group;

    EXPECT_TRUE(group.empty());
    EXPECT_EQ(group.size(), 0U);
    EXPECT_EQ(group.leaf_size(), 0U);

    int visits = 0;
    group.for_each([&visits](auto&)
    {
        ++visits;
    });
    group.for_each_child([&visits](auto&)
    {
        ++visits;
    });
    group.for_each_node([&visits](auto&)
    {
        ++visits;
    });

    EXPECT_EQ(visits, 0);
}

TEST(Composite, DefaultConstructsDefaultConstructibleChildren)
{
    patterns::composite<first_component, second_component> group;

    EXPECT_EQ(group.size(), 2U);
    EXPECT_EQ(group.get<0U>().calls, 0);
    EXPECT_EQ(group.get<1U>().calls, 0);
}

TEST(Composite, MakeCompositeOwnsDecayedValues)
{
    copy_probe source{42};
    auto group = patterns::make_composite(source);

    static_assert(castle::meta::is_same<decltype(group), patterns::composite<copy_probe> >::value,
                  "make_composite stores decayed values");
    EXPECT_EQ(group.get<0U>().value, 42);

    group.get<0U>().value = 7;
    EXPECT_EQ(source.value, 42);
    EXPECT_EQ(group.get<0U>().value, 7);
}

TEST(Composite, MakeCompositeMovesMoveOnlyChildren)
{
    move_only_component source{19};
    auto group = patterns::make_composite(castle::move(source));

    EXPECT_EQ(group.get<0U>().value, 19);
    EXPECT_EQ(source.value, -1);
}

TEST(Composite, TieCompositeStoresReferences)
{
    first_component first;
    second_component second;
    auto group = patterns::tie_composite(first, second);

    static_assert(castle::meta::is_same<decltype(group),
                                        patterns::composite<first_component&, second_component&> >::value,
                  "tie_composite stores lvalue references");

    group.for_each([](auto& component)
    {
        component.run();
    });

    EXPECT_EQ(first.calls, 1);
    EXPECT_EQ(second.calls, 2);
}

TEST(Composite, NestedGroupsVisitLeavesDepthFirstAndLeftToRight)
{
    first_component first;
    second_component second;
    third_component third;

    auto left = patterns::tie_composite(first, second);
    auto right = patterns::tie_composite(third);
    auto root = patterns::make_composite(left, right);

    static_assert(decltype(root)::size() == 2U, "root has two immediate children");
    static_assert(decltype(root)::leaf_size() == 3U, "root has three leaf children");

    int order[3] = {};
    int index = 0;
    root.for_each([&](auto& component)
    {
        using component_type = castle::meta::decay_t<decltype(component)>;
        if constexpr (castle::meta::is_same<component_type, first_component>::value)
        {
            order[index++] = 1;
        }
        else if constexpr (castle::meta::is_same<component_type, second_component>::value)
        {
            order[index++] = 2;
        }
        else
        {
            order[index++] = 3;
        }
        component.run();
    });

    ASSERT_EQ(index, 3);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
    EXPECT_EQ(order[2], 3);
    EXPECT_EQ(first.calls, 1);
    EXPECT_EQ(second.calls, 2);
    EXPECT_EQ(third.calls, 3);
}

TEST(Composite, ForEachChildDoesNotExpandNestedGroups)
{
    first_component first;
    second_component second;
    auto child_group = patterns::tie_composite(first, second);
    auto root = patterns::make_composite(child_group, third_component{});

    int visits = 0;
    bool saw_group = false;
    bool saw_leaf = false;
    root.for_each_child([&](auto& child)
    {
        ++visits;
        using child_type = castle::meta::decay_t<decltype(child)>;
        if constexpr (castle::meta::is_same<child_type, decltype(child_group)>::value)
        {
            saw_group = true;
        }
        else if constexpr (castle::meta::is_same<child_type, third_component>::value)
        {
            saw_leaf = true;
        }
    });

    EXPECT_EQ(visits, 2);
    EXPECT_TRUE(saw_group);
    EXPECT_TRUE(saw_leaf);
}

TEST(Composite, ForEachNodeVisitsGroupsAndLeavesInPreOrder)
{
    first_component first;
    second_component second;
    auto inner = patterns::tie_composite(first, second);
    auto root = patterns::make_composite(inner, third_component{});

    int visits = 0;
    int group_visits = 0;
    int leaf_visits = 0;
    root.for_each_node([&](auto& node)
    {
        ++visits;
        using node_type = castle::meta::decay_t<decltype(node)>;
        if constexpr (castle::meta::is_same<node_type, decltype(inner)>::value)
        {
            ++group_visits;
        }
        else
        {
            ++leaf_visits;
        }
    });

    EXPECT_EQ(visits, 4);
    EXPECT_EQ(group_visits, 1);
    EXPECT_EQ(leaf_visits, 3);
}

TEST(Composite, ForEachNodeVisitsAnEmptyNestedGroup)
{
    patterns::composite<> empty;
    auto root = patterns::make_composite(empty);

    int visits = 0;
    root.for_each([&visits](auto&)
    {
        ++visits;
    });
    EXPECT_EQ(visits, 0);

    root.for_each_node([&visits](auto&)
    {
        ++visits;
    });
    EXPECT_EQ(visits, 1);
}

TEST(Composite, ConstTraversalPassesConstOwnedChildren)
{
    auto mutable_group = patterns::make_composite(first_component{}, second_component{});
    auto const& group = mutable_group;

    int visits = 0;
    group.for_each([&visits](auto const& component)
    {
        using component_type = decltype(component);
        static_assert(castle::meta::is_const<typename castle::meta::remove_reference<component_type>::type>::value,
                      "const composite traversal exposes const owned children");
        ++visits;
    });

    EXPECT_EQ(visits, 2);
}

TEST(Composite, ConstTraversalOfReferenceGroupPreservesReferencedType)
{
    first_component first;
    auto mutable_group = patterns::tie_composite(first);
    auto const& group = mutable_group;

    group.for_each([](auto& component)
    {
        component.run();
    });

    EXPECT_EQ(first.calls, 1);
}

TEST(Composite, GetReturnsImmediateChildReferences)
{
    first_component first;
    second_component second;
    auto group = patterns::tie_composite(first, second);

    static_assert(castle::meta::is_same<decltype(group.get<0U>()), first_component&>::value,
                  "get returns an lvalue reference");
    group.get<0U>().run();
    group.get<1U>().run();

    EXPECT_EQ(first.calls, 1);
    EXPECT_EQ(second.calls, 2);
}

TEST(Composite, ConstGetReturnsConstOwnedChildReference)
{
    auto mutable_group = patterns::make_composite(first_component{});
    auto const& group = mutable_group;

    static_assert(castle::meta::is_same<decltype(group.get<0U>()), first_component const&>::value,
                  "const get returns a const lvalue reference for owned values");
    EXPECT_EQ(group.get<0U>().calls, 0);
}

TEST(Composite, StatefulVisitorIsReusedAcrossLeaves)
{
    // A C++17 generic lambda is stored once by the caller and reused for each leaf.
    auto group = patterns::make_composite(first_component{}, second_component{}, third_component{});
    int calls = 0;
    auto visitor = [&calls](auto&)
    {
        ++calls;
    };

    group.for_each(visitor);
    EXPECT_EQ(calls, 3);
}

TEST(Composite, RepeatedTraversalRunsEveryTime)
{
    first_component first;
    second_component second;
    auto group = patterns::tie_composite(first, second);

    for (int iteration = 0; iteration < 3; ++iteration)
    {
        group.for_each([](auto& component)
        {
            component.run();
        });
    }

    EXPECT_EQ(first.calls, 3);
    EXPECT_EQ(second.calls, 6);
}

TEST(Composite, SupportsOwningAnInnerCompositeByValue)
{
    first_component first;
    second_component second;
    auto inner = patterns::tie_composite(first, second);
    auto root = patterns::make_composite(castle::move(inner));

    root.for_each([](auto& component)
    {
        component.run();
    });

    EXPECT_EQ(first.calls, 1);
    EXPECT_EQ(second.calls, 2);
}


TEST(Composite, CopyingSingleChildGroupUsesTheCopyConstructor)
{
    auto original = patterns::make_composite(first_component{});
    original.get<0U>().calls = 9;

    auto copied = original;
    EXPECT_EQ(copied.get<0U>().calls, 9);

    copied.get<0U>().run();
    EXPECT_EQ(copied.get<0U>().calls, 10);
    EXPECT_EQ(original.get<0U>().calls, 9);
}

TEST(Composite, ConstDirectAndNodeTraversalWork)
{
    first_component first;
    second_component second;
    auto inner = patterns::tie_composite(first, second);
    auto mutable_root = patterns::make_composite(inner, third_component{});
    auto const& root = mutable_root;

    int direct_visits = 0;
    root.for_each_child([&direct_visits](auto const&)
    {
        ++direct_visits;
    });

    int node_visits = 0;
    root.for_each_node([&node_visits](auto const&)
    {
        ++node_visits;
    });

    EXPECT_EQ(direct_visits, 2);
    EXPECT_EQ(node_visits, 4);
}

TEST(Composite, SingleComponentGroupWorks)
{
    first_component first;
    auto group = patterns::tie_composite(first);

    EXPECT_FALSE(group.empty());
    EXPECT_EQ(group.size(), 1U);
    EXPECT_EQ(group.leaf_size(), 1U);

    group.for_each([](auto& component)
    {
        component.run();
    });
    EXPECT_EQ(first.calls, 1);
}

TEST(Composite, FactorySupportsZeroComponents)
{
    auto group = patterns::make_composite();

    EXPECT_TRUE(group.empty());
    EXPECT_EQ(group.size(), 0U);
    EXPECT_EQ(group.leaf_size(), 0U);
}

TEST(Composite, NodeTraversalOrderIsPreOrder)
{
    first_component first;
    second_component second;
    auto inner = patterns::tie_composite(first, second);
    auto root = patterns::make_composite(inner, third_component{});

    int order[4] = {};
    int index = 0;
    root.for_each_node([&](auto& node)
    {
        using node_type = castle::meta::decay_t<decltype(node)>;
        if constexpr (castle::meta::is_same<node_type, decltype(inner)>::value)
        {
            order[index++] = 0;
        }
        else if constexpr (castle::meta::is_same<node_type, first_component>::value)
        {
            order[index++] = 1;
        }
        else if constexpr (castle::meta::is_same<node_type, second_component>::value)
        {
            order[index++] = 2;
        }
        else
        {
            order[index++] = 3;
        }
    });

    ASSERT_EQ(index, 4);
    EXPECT_EQ(order[0], 0);
    EXPECT_EQ(order[1], 1);
    EXPECT_EQ(order[2], 2);
    EXPECT_EQ(order[3], 3);
}

TEST(Composite, ReverseOrderMirrorsLeafTraversal)
{
    trace_log log;
    traced_component<1> first{log};
    traced_component<2> second{log};
    traced_component<3> third{log};
    auto root = patterns::make_composite(patterns::tie_composite(first, second), patterns::tie_composite(third));

    EXPECT_EQ(root.for_each(patterns::reverse_order, [](auto& component)
    {
        return component.stop();
    }), castle::status::ok);

    ASSERT_EQ(log.count, 3);
    EXPECT_EQ(log.ids[0], 3);
    EXPECT_EQ(log.ids[1], 2);
    EXPECT_EQ(log.ids[2], 1);
}

TEST(Composite, ForEachVisitsEveryLeafAndReportsFirstFailure)
{
    trace_log log;
    traced_component<1> first{log, castle::status::time_out};
    traced_component<2> second{log};
    traced_component<3> third{log, castle::status::data_loss};
    auto root = patterns::make_composite(patterns::tie_composite(first, second), patterns::tie_composite(third));

    castle::status result = root.for_each([](auto& component)
    {
        return component.stop();
    });

    EXPECT_EQ(result, castle::status::time_out);
    EXPECT_EQ(log.count, 3);
}

TEST(Composite, TryForEachStopsAtFirstFailureAcrossNestedGroups)
{
    trace_log log;
    traced_component<1> first{log};
    traced_component<2> second{log, castle::status::not_configured};
    traced_component<3> third{log};
    auto root = patterns::make_composite(patterns::tie_composite(first, second), patterns::tie_composite(third));

    castle::status result = root.try_for_each([](auto& component)
    {
        return component.stop();
    });

    EXPECT_EQ(result, castle::status::not_configured);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.ids[0], 1);
    EXPECT_EQ(log.ids[1], 2);
}

TEST(Composite, TryForEachInReverseOrderStopsAtFirstFailure)
{
    trace_log log;
    traced_component<1> first{log};
    traced_component<2> second{log, castle::status::time_out};
    traced_component<3> third{log};
    auto root = patterns::make_composite(patterns::tie_composite(first, second), patterns::tie_composite(third));

    castle::status result = root.try_for_each(patterns::reverse_order, [](auto& component)
    {
        return component.stop();
    });

    EXPECT_EQ(result, castle::status::time_out);
    ASSERT_EQ(log.count, 2);
    EXPECT_EQ(log.ids[0], 3);
    EXPECT_EQ(log.ids[1], 2);
}

TEST(Composite, VisitorMayMixVoidAndStatusResults)
{
    trace_log log;
    first_component plain;
    traced_component<2> traced{log, castle::status::full};
    auto group = patterns::tie_composite(plain, traced);

    auto visitor = [](auto& component)
    {
        using component_type = castle::meta::decay_t<decltype(component)>;
        if constexpr (castle::meta::is_same<component_type, first_component>::value)
        {
            component.run();
        }
        else
        {
            return component.stop();
        }
    };

    EXPECT_EQ(group.try_for_each(visitor), castle::status::full);
    EXPECT_EQ(plain.calls, 1);
    EXPECT_EQ(log.count, 1);
}

TEST(Composite, ForEachChildInReverseOrder)
{
    first_component first;
    auto inner = patterns::tie_composite(first);
    auto root = patterns::make_composite(inner, third_component{});

    int order[2] = {};
    int index = 0;
    root.for_each_child(patterns::reverse_order, [&](auto& child)
    {
        order[index++] = patterns::is_composite<decltype(child)>::value ? 0 : 3;
    });

    ASSERT_EQ(index, 2);
    EXPECT_EQ(order[0], 3);
    EXPECT_EQ(order[1], 0);
}

TEST(Composite, ForEachNodeInReverseOrderVisitsDescendantsBeforeTheirGroup)
{
    trace_log log;
    traced_component<1> first{log};
    traced_component<2> second{log, castle::status::invalid_config};
    auto inner = patterns::tie_composite(first, second);
    auto root = patterns::make_composite(inner, traced_component<3>{log});

    castle::status result = root.for_each_node(patterns::reverse_order, [&log](auto& node)
    {
        if constexpr (patterns::is_composite<decltype(node)>::value)
        {
            log.push(0);
            return castle::status::ok;
        }
        else
        {
            return node.stop();
        }
    });

    EXPECT_EQ(result, castle::status::invalid_config);
    ASSERT_EQ(log.count, 4);
    EXPECT_EQ(log.ids[0], 3);
    EXPECT_EQ(log.ids[1], 2);
    EXPECT_EQ(log.ids[2], 1);
    EXPECT_EQ(log.ids[3], 0);
}

TEST(Composite, IsCompositeIgnoresCvAndReferenceQualifiers)
{
    using group_type = patterns::composite<first_component>;

    static_assert(patterns::is_composite<group_type>::value, "plain composite");
    static_assert(patterns::is_composite<group_type const&>::value, "const reference composite");
    static_assert(!patterns::is_composite<first_component&>::value, "leaf is not a composite");
    SUCCEED();
}
