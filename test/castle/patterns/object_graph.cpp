#include <gtest/gtest.h>

#include "castle/patterns/object_graph.hpp"
#include "castle/patterns/singleton.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"

#include <stdint.h>

namespace patterns = castle::patterns;

namespace
{

// Graph objects are constructed by the graph, so they report to one shared log.
// Positive ids are init/construction events, negative ids are deinit/destruction events.
struct event_log
{
    void push(int id) noexcept
    {
        if (count < 16)
        {
            ids[count] = id;
        }
        ++count;
    }

    void reset() noexcept
    {
        count = 0;
    }

    int ids[16] = {};
    int count = 0;
};

event_log hooks;
event_log lifetimes;

// Behavior of the lifecycle hooks of `node<Id>`, configurable per test.
template <int Id>
struct node_control
{
    static castle::status init_result;
    static castle::status deinit_result;

    static void reset() noexcept
    {
        init_result = castle::status::ok;
        deinit_result = castle::status::ok;
    }
};

template <int Id>
castle::status node_control<Id>::init_result = castle::status::ok;

template <int Id>
castle::status node_control<Id>::deinit_result = castle::status::ok;

// Object with status-returning hooks; logs construction and destruction separately.
template <int Id>
struct node
{
    node() noexcept
    {
        lifetimes.push(Id);
    }

    ~node()
    {
        lifetimes.push(-Id);
    }

    castle::status init() noexcept
    {
        hooks.push(Id);
        return node_control<Id>::init_result;
    }

    castle::status deinit() noexcept
    {
        hooks.push(-Id);
        return node_control<Id>::deinit_result;
    }
};

// Object with void hooks.
struct void_hooks
{
    void init() noexcept
    {
        hooks.push(100);
    }

    void deinit() noexcept
    {
        hooks.push(-100);
    }
};

// Objects without any hook.
struct plain_value
{
    int value;
};

struct config
{
    int baud = 9600;
};

struct shared_pool
{
    int writes = 0;
};

// Accepts only a const reference: compiles only when init_with<> passes const T&.
struct configured
{
    explicit configured(const config& source) noexcept
        : baud(source.baud)
        , source(&source)
    {
    }

    int baud;
    const config* source;
};

struct pool_user_a
{
    explicit pool_user_a(shared_pool& pool) noexcept
        : pool(pool)
    {
    }

    shared_pool& pool;
};

struct pool_user_b
{
    explicit pool_user_b(shared_pool& pool) noexcept
        : pool(pool)
    {
    }

    shared_pool& pool;
};

// Receives the arguments of three relations, flattened in written order.
struct assembly
{
    assembly(const config& cfg, pool_user_a& a, pool_user_b& b, shared_pool& pool) noexcept
        : cfg(cfg)
        , a(a)
        , b(b)
        , pool(pool)
    {
    }

    const config& cfg;
    pool_user_a& a;
    pool_user_b& b;
    shared_pool& pool;
};

using wiring_graph = patterns::object_graph<
    config,
    shared_pool,
    patterns::object<configured, patterns::init_with<config>>,
    patterns::object<pool_user_a, patterns::uses<shared_pool>>,
    patterns::object<pool_user_b, patterns::uses<shared_pool>>,
    patterns::object<assembly,
                     patterns::init_with<config>,
                     patterns::owns<pool_user_a, pool_user_b>,
                     patterns::uses<shared_pool>>>;

using lifecycle_graph = patterns::object_graph<node<1>, plain_value, void_hooks, node<2>, node<3>>;

void reset_logs() noexcept
{
    hooks.reset();
    lifetimes.reset();
    node_control<1>::reset();
    node_control<2>::reset();
    node_control<3>::reset();
}

class ObjectGraph : public ::testing::Test
{
protected:
    void SetUp() override
    {
        reset_logs();
    }

    void TearDown() override
    {
        reset_logs();
    }
};

} // namespace

TEST_F(ObjectGraph, ReportsSizeAndMembership)
{
    static_assert(wiring_graph::size() == 6U, "six objects");
    static_assert(wiring_graph::object_count == 6U, "six objects");
    static_assert(wiring_graph::contains<assembly>(), "assembly is an object");
    static_assert(!wiring_graph::contains<int>(), "int is not an object");

    EXPECT_EQ(lifecycle_graph::size(), 5U);
    EXPECT_TRUE(lifecycle_graph::contains<void_hooks>());
    EXPECT_FALSE(lifecycle_graph::contains<config>());
}

TEST_F(ObjectGraph, InitWithPassesTheGraphObjectAsConstReference)
{
    wiring_graph graph;

    EXPECT_EQ(graph.get<configured>().baud, 9600);
    EXPECT_EQ(graph.get<configured>().source, &graph.get<config>());
}

TEST_F(ObjectGraph, UsesSharesOneObjectBetweenSeveralUsers)
{
    wiring_graph graph;

    EXPECT_EQ(&graph.get<pool_user_a>().pool, &graph.get<shared_pool>());
    EXPECT_EQ(&graph.get<pool_user_b>().pool, &graph.get<shared_pool>());

    graph.get<pool_user_a>().pool.writes = 3;
    EXPECT_EQ(graph.get<pool_user_b>().pool.writes, 3);
}

TEST_F(ObjectGraph, RelationsAreFlattenedInWrittenOrder)
{
    wiring_graph graph;
    assembly& whole = graph.get<assembly>();

    EXPECT_EQ(&whole.cfg, &graph.get<config>());
    EXPECT_EQ(&whole.a, &graph.get<pool_user_a>());
    EXPECT_EQ(&whole.b, &graph.get<pool_user_b>());
    EXPECT_EQ(&whole.pool, &graph.get<shared_pool>());
}

TEST_F(ObjectGraph, ObjectsWithoutRelationsAreValueInitialized)
{
    lifecycle_graph graph;

    EXPECT_EQ(graph.get<plain_value>().value, 0);
}

TEST_F(ObjectGraph, ConstGetReturnsConstReferenceToTheSameObject)
{
    wiring_graph graph;
    const wiring_graph& view = graph;

    static_assert(castle::meta::is_same<decltype(view.get<config>()), const config&>::value,
                  "const get returns a const reference");
    EXPECT_EQ(&view.get<config>(), &graph.get<config>());
}

TEST_F(ObjectGraph, ConstructsInDeclarationOrderAndDestroysInReverse)
{
    {
        lifecycle_graph graph;
        ASSERT_EQ(lifetimes.count, 3);
        EXPECT_EQ(lifetimes.ids[0], 1);
        EXPECT_EQ(lifetimes.ids[1], 2);
        EXPECT_EQ(lifetimes.ids[2], 3);
    }

    ASSERT_EQ(lifetimes.count, 6);
    EXPECT_EQ(lifetimes.ids[3], -3);
    EXPECT_EQ(lifetimes.ids[4], -2);
    EXPECT_EQ(lifetimes.ids[5], -1);
    EXPECT_EQ(hooks.count, 0); // never initialized, so the destructor runs no deinit hook
}

TEST_F(ObjectGraph, ObjectsViewVisitsEveryObjectInDeclarationOrder)
{
    wiring_graph graph;
    auto all = graph.objects();

    static_assert(decltype(all)::size() == 6U, "one child per object");
    EXPECT_EQ(&all.get<5U>(), &graph.get<assembly>());

    int order[6] = {};
    int index = 0;
    all.for_each_child([&order, &index](auto& item)
    {
        using item_type = castle::meta::decay_t<decltype(item)>;
        order[index++] = castle::meta::is_same<item_type, config>::value      ? 0
                         : castle::meta::is_same<item_type, shared_pool>::value ? 1
                         : castle::meta::is_same<item_type, configured>::value  ? 2
                         : castle::meta::is_same<item_type, pool_user_a>::value ? 3
                         : castle::meta::is_same<item_type, pool_user_b>::value ? 4
                                                                                : 5;
    });

    ASSERT_EQ(index, 6);
    for (int i = 0; i < 6; ++i)
    {
        EXPECT_EQ(order[i], i);
    }
}

TEST_F(ObjectGraph, ConstObjectsViewExposesConstObjects)
{
    wiring_graph graph;
    const wiring_graph& view = graph;
    auto all = view.objects();

    static_assert(castle::meta::is_same<decltype(all.get<0U>()), const config&>::value,
                  "const objects() refers to const objects");
    EXPECT_EQ(&all.get<0U>(), &graph.get<config>());
    EXPECT_EQ(&all.get<1U>(), &graph.get<shared_pool>());
}

TEST_F(ObjectGraph, InitRunsHooksInDeclarationOrderAndSkipsObjectsWithoutHooks)
{
    lifecycle_graph graph;
    EXPECT_FALSE(graph.is_initialized());

    EXPECT_EQ(graph.init(), castle::status::ok);
    EXPECT_TRUE(graph.is_initialized());

    ASSERT_EQ(hooks.count, 4);
    EXPECT_EQ(hooks.ids[0], 1);
    EXPECT_EQ(hooks.ids[1], 100);
    EXPECT_EQ(hooks.ids[2], 2);
    EXPECT_EQ(hooks.ids[3], 3);

    EXPECT_EQ(graph.deinit(), castle::status::ok);
}

TEST_F(ObjectGraph, InitTwiceIsRejectedWithoutRunningHooks)
{
    lifecycle_graph graph;
    ASSERT_EQ(graph.init(), castle::status::ok);
    hooks.reset();

    EXPECT_EQ(graph.init(), castle::status::already_exists);
    EXPECT_EQ(hooks.count, 0);
    EXPECT_TRUE(graph.is_initialized());

    EXPECT_EQ(graph.deinit(), castle::status::ok);
}

TEST_F(ObjectGraph, FailedInitRollsBackInitializedObjectsInReverse)
{
    node_control<2>::init_result = castle::status::time_out;
    lifecycle_graph graph;

    EXPECT_EQ(graph.init(), castle::status::time_out);
    EXPECT_FALSE(graph.is_initialized());

    // node<1>, void_hooks, node<2> fails; node<3> never starts; rollback skips node<2>.
    ASSERT_EQ(hooks.count, 5);
    EXPECT_EQ(hooks.ids[0], 1);
    EXPECT_EQ(hooks.ids[1], 100);
    EXPECT_EQ(hooks.ids[2], 2);
    EXPECT_EQ(hooks.ids[3], -100);
    EXPECT_EQ(hooks.ids[4], -1);
}

TEST_F(ObjectGraph, FailedInitOfFirstObjectRollsBackNothing)
{
    node_control<1>::init_result = castle::status::not_found;
    lifecycle_graph graph;

    EXPECT_EQ(graph.init(), castle::status::not_found);
    ASSERT_EQ(hooks.count, 1);
    EXPECT_EQ(hooks.ids[0], 1);
}

TEST_F(ObjectGraph, RollbackIgnoresDeinitFailuresAndReportsTheInitFailure)
{
    node_control<1>::deinit_result = castle::status::data_loss;
    node_control<3>::init_result = castle::status::full;
    lifecycle_graph graph;

    EXPECT_EQ(graph.init(), castle::status::full);
    ASSERT_EQ(hooks.count, 7);
    EXPECT_EQ(hooks.ids[4], -2);
    EXPECT_EQ(hooks.ids[5], -100);
    EXPECT_EQ(hooks.ids[6], -1);
}

TEST_F(ObjectGraph, InitCanBeRetriedAfterAFailure)
{
    node_control<3>::init_result = castle::status::system_call_error;
    lifecycle_graph graph;
    ASSERT_EQ(graph.init(), castle::status::system_call_error);

    node_control<3>::init_result = castle::status::ok;
    EXPECT_EQ(graph.init(), castle::status::ok);
    EXPECT_TRUE(graph.is_initialized());

    EXPECT_EQ(graph.deinit(), castle::status::ok);
}

TEST_F(ObjectGraph, DeinitRunsHooksInReverseDeclarationOrder)
{
    lifecycle_graph graph;
    ASSERT_EQ(graph.init(), castle::status::ok);
    hooks.reset();

    EXPECT_EQ(graph.deinit(), castle::status::ok);
    EXPECT_FALSE(graph.is_initialized());

    ASSERT_EQ(hooks.count, 4);
    EXPECT_EQ(hooks.ids[0], -3);
    EXPECT_EQ(hooks.ids[1], -2);
    EXPECT_EQ(hooks.ids[2], -100);
    EXPECT_EQ(hooks.ids[3], -1);
}

TEST_F(ObjectGraph, DeinitVisitsEveryObjectAndReportsTheFirstFailure)
{
    node_control<3>::deinit_result = castle::status::invalid_config;
    node_control<1>::deinit_result = castle::status::unknown_error;
    lifecycle_graph graph;
    ASSERT_EQ(graph.init(), castle::status::ok);
    hooks.reset();

    EXPECT_EQ(graph.deinit(), castle::status::invalid_config);
    EXPECT_FALSE(graph.is_initialized());
    EXPECT_EQ(hooks.count, 4);
}

TEST_F(ObjectGraph, DeinitWithoutInitIsRejectedWithoutRunningHooks)
{
    lifecycle_graph graph;

    EXPECT_EQ(graph.deinit(), castle::status::not_configured);
    EXPECT_EQ(hooks.count, 0);

    ASSERT_EQ(graph.init(), castle::status::ok);
    ASSERT_EQ(graph.deinit(), castle::status::ok);
    hooks.reset();

    EXPECT_EQ(graph.deinit(), castle::status::not_configured);
    EXPECT_EQ(hooks.count, 0);
}

TEST_F(ObjectGraph, DestructorDeinitializesAnInitializedGraphBeforeDestroyingObjects)
{
    {
        lifecycle_graph graph;
        ASSERT_EQ(graph.init(), castle::status::ok);
        hooks.reset();
        lifetimes.reset();
    }

    ASSERT_EQ(hooks.count, 4);
    EXPECT_EQ(hooks.ids[0], -3);
    EXPECT_EQ(hooks.ids[3], -1);
    ASSERT_EQ(lifetimes.count, 3);
    EXPECT_EQ(lifetimes.ids[0], -3);
    EXPECT_EQ(lifetimes.ids[2], -1);
}

TEST_F(ObjectGraph, CompositeObjectIsOneObjectForLifecycleAndObjectsView)
{
    using group = patterns::composite<node<1>, node<2>>;
    patterns::object_graph<group, node<3>> graph;

    int children = 0;
    graph.objects().for_each_child([&children](auto&)
    {
        ++children;
    });
    EXPECT_EQ(children, 2);

    // The composite has no hooks of its own; its leaves are not initialized by the graph.
    node_control<3>::init_result = castle::status::empty;
    EXPECT_EQ(graph.init(), castle::status::empty);
    ASSERT_EQ(hooks.count, 1);
    EXPECT_EQ(hooks.ids[0], 3);

    node_control<3>::init_result = castle::status::ok;
    EXPECT_EQ(graph.init(), castle::status::ok);
    EXPECT_EQ(graph.init(), castle::status::already_exists);
    EXPECT_EQ(graph.deinit(), castle::status::ok);
    EXPECT_EQ(graph.deinit(), castle::status::not_configured);
    ASSERT_EQ(hooks.count, 3);
    EXPECT_EQ(hooks.ids[2], -3);

    {
        patterns::object_graph<group, node<3>> idle;
    }
    EXPECT_EQ(hooks.count, 3); // an uninitialized graph runs no hook when destroyed

    EXPECT_EQ(graph.init(), castle::status::ok);
    // The destructor deinitializes the still-initialized graph.
}

TEST_F(ObjectGraph, GraphWithoutHooksInitializesAndDeinitializes)
{
    wiring_graph graph;

    EXPECT_EQ(graph.deinit(), castle::status::not_configured);
    EXPECT_EQ(graph.init(), castle::status::ok);
    EXPECT_EQ(graph.init(), castle::status::already_exists);
    EXPECT_EQ(graph.deinit(), castle::status::ok);
    EXPECT_EQ(graph.init(), castle::status::ok);
    EXPECT_TRUE(graph.is_initialized());
    // The destructor deinitializes the still-initialized graph.
}

TEST_F(ObjectGraph, SingletonDefersConstructionOfAGraphInStaticStorage)
{
    using deferred = patterns::singleton<lifecycle_graph>;
    EXPECT_EQ(lifetimes.count, 0);

    deferred::create();
    EXPECT_EQ(lifetimes.count, 3);
    ASSERT_EQ(deferred::instance().init(), castle::status::ok);
    hooks.reset();

    deferred::destroy();
    EXPECT_FALSE(deferred::is_valid());
    EXPECT_EQ(hooks.count, 4); // destroy() ran the deinit hooks through the graph destructor
    EXPECT_EQ(lifetimes.count, 6);
}
