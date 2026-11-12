#include <gtest/gtest.h>
#include "castle/dsa/list.hpp"

#include <vector>

namespace
{
using L = castle::dsa::list<int, 4>;

struct Tracker
{
    explicit Tracker(int v = 0) : value(v) { ++live; }
    Tracker(const Tracker& other) : value(other.value) { ++live; }
    Tracker(Tracker&& other) : value(other.value) { other.value = -1; ++live; }
    ~Tracker() { --live; }
    bool operator==(const Tracker& other) const { return value == other.value; }
    int value;
    static int live;
};
int Tracker::live = 0;

struct Pair
{
    Pair(int k, int v) : key(k), value(v) {}
    bool operator==(const Pair& other) const { return key == other.key && value == other.value; }
    int key;
    int value;
};

template <typename List>
std::vector<int> forward_values(const List& l)
{
    std::vector<int> out;
    for (auto it = l.begin(); it != l.end(); ++it)
    {
        out.push_back(*it);
    }
    return out;
}

template <typename List>
std::vector<int> backward_values(List& l)
{
    std::vector<int> out;
    for (auto it = l.rbegin(); it != l.rend(); ++it)
    {
        out.push_back(*it);
    }
    return out;
}

using V = std::vector<int>;

TEST(ListTest, EmptyState)
{
    L l;
    static_assert(L::capacity() == 4U, "capacity");
    EXPECT_EQ(L::capacity(), 4U);
    EXPECT_EQ(l.max_size(), 4U);
    EXPECT_EQ(l.size(), 0U);
    EXPECT_EQ(l.available(), 4U);
    EXPECT_TRUE(l.empty());
    EXPECT_FALSE(l.full());
    EXPECT_TRUE(l.begin() == l.end());
    EXPECT_TRUE(l.cbegin() == l.cend());
    EXPECT_TRUE(l.rbegin() == l.rend());
    EXPECT_TRUE(l.crbegin() == l.crend());
    EXPECT_EQ(l.pop_front(), castle::status::empty);
    EXPECT_EQ(l.pop_back(), castle::status::empty);
    EXPECT_TRUE(l.erase(l.end()) == l.end());
    EXPECT_EQ(l.remove(1), 0U);
    EXPECT_TRUE(l.find(1) == l.end());
    l.reverse();
    l.clear();
    l.move_to_front(l.end());
    l.move_to_back(l.end());
    EXPECT_TRUE(l.empty());
    EXPECT_TRUE(l.begin() == l.end());
}

TEST(ListTest, SingularIteratorsCompareEqual)
{
    L::iterator a;
    L::iterator b;
    L::const_iterator ca;
    L::const_iterator cb;
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(ca == cb);
    EXPECT_FALSE(ca != cb);
}

TEST(ListTest, PushBothEndsAndLimits)
{
    L l;
    int x = 2;
    EXPECT_EQ(l.push_back(x), castle::status::ok);
    EXPECT_EQ(l.push_back(3), castle::status::ok);
    EXPECT_EQ(l.push_front(1), castle::status::ok);
    int y = 0;
    EXPECT_EQ(l.push_front(y), castle::status::ok);
    EXPECT_TRUE(l.full());
    EXPECT_EQ(l.available(), 0U);
    EXPECT_EQ(l.size(), 4U);
    EXPECT_EQ(forward_values(l), (V{0, 1, 2, 3}));
    EXPECT_EQ(backward_values(l), (V{3, 2, 1, 0}));
    EXPECT_EQ(l.front(), 0);
    EXPECT_EQ(l.back(), 3);

    EXPECT_EQ(l.push_back(9), castle::status::full);
    EXPECT_EQ(l.push_back(x), castle::status::full);
    EXPECT_EQ(l.push_front(9), castle::status::full);
    EXPECT_EQ(l.push_front(x), castle::status::full);
    EXPECT_EQ(l.emplace_back(9), castle::status::full);
    EXPECT_EQ(l.emplace_front(9), castle::status::full);
    EXPECT_EQ(l.size(), 4U);
    EXPECT_EQ(forward_values(l), (V{0, 1, 2, 3}));
}

TEST(ListTest, FrontAndBackAreMutableAndConstAccessible)
{
    L l{1, 2, 3};
    l.front() = 10;
    l.back() = 30;
    const L& cl = l;
    EXPECT_EQ(cl.front(), 10);
    EXPECT_EQ(cl.back(), 30);
    EXPECT_EQ(forward_values(cl), (V{10, 2, 30}));
}

TEST(ListTest, PopBothEnds)
{
    L l{1, 2, 3};
    EXPECT_EQ(l.pop_front(), castle::status::ok);
    EXPECT_EQ(forward_values(l), (V{2, 3}));
    EXPECT_EQ(l.pop_back(), castle::status::ok);
    EXPECT_EQ(forward_values(l), (V{2}));
    EXPECT_EQ(l.front(), l.back());
    EXPECT_EQ(l.pop_back(), castle::status::ok);
    EXPECT_TRUE(l.empty());
    EXPECT_TRUE(l.begin() == l.end());
    EXPECT_EQ(l.pop_front(), castle::status::empty);
}

TEST(ListTest, InitializerListStopsWhenFull)
{
    L l{1, 2, 3, 4, 5, 6};
    EXPECT_TRUE(l.full());
    EXPECT_EQ(forward_values(l), (V{1, 2, 3, 4}));
    EXPECT_EQ(backward_values(l), (V{4, 3, 2, 1}));

    L partial{7, 8};
    EXPECT_EQ(partial.size(), 2U);
    EXPECT_EQ(forward_values(partial), (V{7, 8}));
}

TEST(ListTest, IteratorOperators)
{
    castle::dsa::list<Pair, 3> l;
    l.emplace_back(1, 10);
    l.emplace_back(2, 20);
    l.emplace_back(3, 30);

    auto it = l.begin();
    EXPECT_EQ(it->key, 1);
    EXPECT_EQ((*it).value, 10);
    auto old = it++;
    EXPECT_EQ(old->key, 1);
    EXPECT_EQ(it->key, 2);
    ++it;
    EXPECT_EQ(it->key, 3);
    ++it;
    EXPECT_TRUE(it == l.end());
    --it;
    EXPECT_EQ(it->key, 3);
    auto older = it--;
    EXPECT_EQ(older->key, 3);
    EXPECT_EQ(it->key, 2);
    it->value = 21;
    EXPECT_EQ((*it).value, 21);

    const auto& cl = l;
    auto cit = cl.begin();
    EXPECT_EQ(cit->key, 1);
    EXPECT_EQ((*cit).value, 10);
    auto cold = cit++;
    EXPECT_EQ(cold->key, 1);
    EXPECT_EQ(cit->key, 2);
    ++cit;
    ++cit;
    EXPECT_TRUE(cit == cl.end());
    --cit;
    EXPECT_EQ(cit->key, 3);
    auto colder = cit--;
    EXPECT_EQ(colder->key, 3);
    EXPECT_EQ(cit->value, 21);
}

TEST(ListTest, MixedIteratorComparisonAndConversion)
{
    L l{1, 2};
    L::iterator it = l.begin();
    L::const_iterator cit = it;
    EXPECT_TRUE(it == cit);
    EXPECT_TRUE(cit == it);
    EXPECT_FALSE(it != cit);
    ++it;
    EXPECT_TRUE(it != cit);
    EXPECT_TRUE(it != l.cbegin());
    EXPECT_TRUE(l.cend() != l.begin());
    EXPECT_TRUE(l.cend() == l.end());
}

TEST(ListTest, ReverseIteratorBaseAndConst)
{
    L l{1, 2, 3};
    const L& cl = l;
    EXPECT_EQ(*cl.rbegin(), 3);
    EXPECT_EQ(*cl.crbegin(), 3);
    EXPECT_TRUE(cl.rbegin().base() == cl.end());
    EXPECT_TRUE(cl.rend().base() == cl.begin());
    EXPECT_TRUE(cl.crend() == cl.rend());

    auto rit = l.rbegin();
    *rit = 30;
    ++rit;
    EXPECT_EQ(*rit, 2);
    EXPECT_EQ(l.back(), 30);
}

TEST(ListTest, InsertEmplaceAtPositions)
{
    L l{2, 4};
    auto at_front = l.insert(l.begin(), 1);
    EXPECT_EQ(*at_front, 1);
    EXPECT_TRUE(at_front == l.begin());

    int three = 3;
    auto mid = l.begin();
    ++mid;
    ++mid; // 4
    auto at_mid = l.insert(mid, three);
    EXPECT_EQ(*at_mid, 3);
    EXPECT_EQ(forward_values(l), (V{1, 2, 3, 4}));
    EXPECT_EQ(backward_values(l), (V{4, 3, 2, 1}));
    EXPECT_TRUE(l.full());

    EXPECT_TRUE(l.insert(l.end(), 9) == l.end());
    EXPECT_TRUE(l.insert(l.begin(), three) == l.end());
    EXPECT_TRUE(l.emplace(l.begin(), 9) == l.end());
    EXPECT_EQ(forward_values(l), (V{1, 2, 3, 4}));

    L m;
    auto only = m.emplace(m.end(), 5);
    EXPECT_TRUE(only == m.begin());
    EXPECT_EQ(*only, 5);
    auto last = m.insert(m.end(), 6);
    EXPECT_EQ(m.back(), 6);
    EXPECT_EQ(*last, 6);
    EXPECT_EQ(forward_values(m), (V{5, 6}));
}

TEST(ListTest, EraseSingleAndRange)
{
    L l{1, 2, 3, 4};
    auto it = l.begin();
    ++it; // 2
    auto next = l.erase(it);
    EXPECT_EQ(*next, 3);
    EXPECT_EQ(forward_values(l), (V{1, 3, 4}));

    auto after_last = l.erase(--l.end());
    EXPECT_TRUE(after_last == l.end());
    EXPECT_EQ(forward_values(l), (V{1, 3}));
    EXPECT_TRUE(l.erase(l.end()) == l.end());
    EXPECT_EQ(l.size(), 2U);

    L r{1, 2, 3, 4};
    auto first = r.begin();
    ++first;
    auto last = first;
    ++last;
    ++last; // 4
    auto ret = r.erase(first, last);
    EXPECT_TRUE(ret == last);
    EXPECT_EQ(*ret, 4);
    EXPECT_EQ(forward_values(r), (V{1, 4}));
    EXPECT_TRUE(r.erase(r.begin(), r.begin()) == r.begin());
    EXPECT_EQ(r.size(), 2U);
    EXPECT_TRUE(r.erase(r.begin(), r.end()) == r.end());
    EXPECT_TRUE(r.empty());
    EXPECT_TRUE(r.begin() == r.end());
    EXPECT_EQ(r.available(), 4U);
}

TEST(ListTest, ErasedNodesAreReused)
{
    L l{1, 2, 3, 4};
    EXPECT_EQ(l.push_back(5), castle::status::full);
    l.pop_front();
    EXPECT_EQ(l.push_back(5), castle::status::ok);
    l.erase(l.begin());
    EXPECT_EQ(l.push_front(0), castle::status::ok);
    EXPECT_EQ(forward_values(l), (V{0, 3, 4, 5}));
    EXPECT_TRUE(l.full());
}

TEST(ListTest, SpliceMovesElementWithoutInvalidation)
{
    L l{1, 2, 3, 4};
    auto three = l.begin();
    ++three;
    ++three;
    auto two = l.begin();
    ++two;

    l.splice(l.begin(), three); // before 1
    EXPECT_EQ(forward_values(l), (V{3, 1, 2, 4}));
    EXPECT_EQ(backward_values(l), (V{4, 2, 1, 3}));
    EXPECT_EQ(*three, 3);

    l.splice(l.end(), two); // to the back
    EXPECT_EQ(forward_values(l), (V{3, 1, 4, 2}));
    EXPECT_EQ(backward_values(l), (V{2, 4, 1, 3}));
    EXPECT_EQ(*two, 2);

    auto one = l.find(1);
    auto four = l.find(4);
    l.splice(four, l.begin()); // middle destination, 3 before 4
    EXPECT_EQ(forward_values(l), (V{1, 3, 4, 2}));
    EXPECT_EQ(backward_values(l), (V{2, 4, 3, 1}));
    EXPECT_EQ(*one, 1);
    EXPECT_EQ(l.size(), 4U);
}

TEST(ListTest, SpliceNoOpCases)
{
    L l{1, 2, 3};
    auto two = l.find(2);
    auto three = l.find(3);

    l.splice(two, two);        // element == position
    EXPECT_EQ(forward_values(l), (V{1, 2, 3}));
    l.splice(three, two);      // already directly before position
    EXPECT_EQ(forward_values(l), (V{1, 2, 3}));
    l.splice(l.begin(), l.end()); // end() element
    EXPECT_EQ(forward_values(l), (V{1, 2, 3}));
    l.splice(l.end(), three);  // already last
    EXPECT_EQ(forward_values(l), (V{1, 2, 3}));
    EXPECT_EQ(backward_values(l), (V{3, 2, 1}));
}

TEST(ListTest, MoveToFrontAndBackSupportLruPattern)
{
    L l{1, 2, 3};
    l.move_to_front(l.find(3));
    EXPECT_EQ(forward_values(l), (V{3, 1, 2}));
    l.move_to_front(l.begin());
    EXPECT_EQ(forward_values(l), (V{3, 1, 2}));
    l.move_to_front(l.find(2));
    EXPECT_EQ(forward_values(l), (V{2, 3, 1}));
    EXPECT_EQ(backward_values(l), (V{1, 3, 2}));

    l.move_to_back(l.find(2));
    EXPECT_EQ(forward_values(l), (V{3, 1, 2}));
    l.move_to_back(--l.end());
    EXPECT_EQ(forward_values(l), (V{3, 1, 2}));
    EXPECT_EQ(backward_values(l), (V{2, 1, 3}));

    // Evict least-recently-used (back) and add a new most-recently-used (front).
    EXPECT_EQ(l.back(), 2);
    EXPECT_EQ(l.pop_back(), castle::status::ok);
    EXPECT_EQ(l.push_front(4), castle::status::ok);
    EXPECT_EQ(forward_values(l), (V{4, 3, 1}));
}

TEST(ListTest, RemoveEqualValues)
{
    L l{1, 2, 1, 1};
    EXPECT_EQ(l.remove(1), 3U);
    EXPECT_EQ(forward_values(l), (V{2}));
    EXPECT_EQ(l.remove(1), 0U);
    EXPECT_EQ(l.remove(2), 1U);
    EXPECT_TRUE(l.empty());

    L m{5, 6, 7};
    EXPECT_EQ(m.remove(7), 1U);
    EXPECT_EQ(m.remove(5), 1U);
    EXPECT_EQ(forward_values(m), (V{6}));
    EXPECT_EQ(backward_values(m), (V{6}));
}

TEST(ListTest, ReverseInPlace)
{
    L one{1};
    one.reverse();
    EXPECT_EQ(forward_values(one), (V{1}));

    L l{1, 2, 3, 4};
    auto first = l.begin();
    l.reverse();
    EXPECT_EQ(forward_values(l), (V{4, 3, 2, 1}));
    EXPECT_EQ(backward_values(l), (V{1, 2, 3, 4}));
    EXPECT_EQ(*first, 1);
    EXPECT_TRUE(first == --l.end());
    EXPECT_EQ(l.front(), 4);
    EXPECT_EQ(l.back(), 1);
    l.reverse();
    EXPECT_EQ(forward_values(l), (V{1, 2, 3, 4}));
}

TEST(ListTest, FindConstAndMutable)
{
    L l{1, 2, 3};
    auto it = l.find(2);
    ASSERT_TRUE(it != l.end());
    *it = 20;
    EXPECT_TRUE(l.find(2) == l.end());
    EXPECT_TRUE(l.find(20) == it);
    EXPECT_TRUE(l.find(99) == l.end());

    const L& cl = l;
    auto cit = cl.find(3);
    ASSERT_TRUE(cit != cl.end());
    EXPECT_EQ(*cit, 3);
    EXPECT_TRUE(cl.find(2) == cl.end());
    EXPECT_TRUE(cl.find(1) == cl.begin());
}

TEST(ListTest, ClearReleasesEverythingAndAllowsReuse)
{
    L l{1, 2, 3, 4};
    l.clear();
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.available(), 4U);
    EXPECT_TRUE(l.begin() == l.end());
    EXPECT_TRUE(l.rbegin() == l.rend());
    EXPECT_EQ(l.push_back(7), castle::status::ok);
    EXPECT_EQ(l.push_front(6), castle::status::ok);
    EXPECT_EQ(forward_values(l), (V{6, 7}));
    EXPECT_EQ(backward_values(l), (V{7, 6}));
}

TEST(ListTest, ElementLifetimeIsManaged)
{
    Tracker::live = 0;
    {
        castle::dsa::list<Tracker, 3> l;
        Tracker seed(1);
        EXPECT_EQ(Tracker::live, 1);
        EXPECT_EQ(l.push_back(seed), castle::status::ok);
        EXPECT_EQ(l.push_front(Tracker(2)), castle::status::ok);
        EXPECT_EQ(l.emplace_back(3), castle::status::ok);
        EXPECT_EQ(Tracker::live, 4);
        EXPECT_EQ(l.push_back(seed), castle::status::full);
        EXPECT_EQ(l.push_back(Tracker(9)), castle::status::full);
        EXPECT_EQ(Tracker::live, 4);
        EXPECT_EQ(l.front().value, 2);
        EXPECT_EQ(l.back().value, 3);
        EXPECT_EQ(l.pop_front(), castle::status::ok);
        EXPECT_EQ(Tracker::live, 3);
        l.erase(l.begin());
        EXPECT_EQ(Tracker::live, 2);
        l.clear();
        EXPECT_EQ(Tracker::live, 1);
        l.emplace_back(4);
        l.emplace_back(5);
        EXPECT_EQ(Tracker::live, 3);
        EXPECT_EQ(l.remove(Tracker(4)), 1U);
        EXPECT_EQ(Tracker::live, 2);
    }
    EXPECT_EQ(Tracker::live, 0);
}

TEST(ListTest, EmplaceForwardsConstructorArguments)
{
    castle::dsa::list<Pair, 2> l;
    EXPECT_EQ(l.emplace_back(1, 10), castle::status::ok);
    EXPECT_EQ(l.emplace_front(0, 5), castle::status::ok);
    EXPECT_EQ(l.front().key, 0);
    EXPECT_EQ(l.front().value, 5);
    EXPECT_EQ(l.back().key, 1);
    EXPECT_EQ(l.back().value, 10);
    EXPECT_TRUE(l.emplace(l.end(), 2, 20) == l.end());
}

TEST(ListTest, ListIsNotCopyOrMovable)
{
    static_assert(!__is_constructible(L, const L&), "copy");
    static_assert(!__is_constructible(L, L&&), "move");
    static_assert(!__is_assignable(L&, const L&), "copy assign");
    static_assert(!__is_assignable(L&, L&&), "move assign");
    SUCCEED();
}

} // namespace
