#include "sample_support.hpp"

#include "castle/container/avl_tree.hpp"

#include <stdint.h>

int main()
{
    castle::container::avl_tree<uint16_t, uint32_t, 5U> tree;
    CASTLE_SAMPLE_CHECK(tree.capacity() == 5U);
    CASTLE_SAMPLE_CHECK(tree.available() == 5U);
    CASTLE_SAMPLE_CHECK(tree.empty());

    castle::less<uint16_t> compare;
    castle::container::avl_tree<uint16_t, uint32_t, 5U> compared(compare);
    CASTLE_SAMPLE_CHECK(compared.empty());

    CASTLE_SAMPLE_CHECK(tree.insert(30U, 300U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tree.insert(10U, 100U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tree.emplace(20U, 200U) == castle::status::ok);
    uint32_t moved = 400U;
    CASTLE_SAMPLE_CHECK(tree.insert(40U, castle::move(moved)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tree.insert(50U, 500U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tree.full());
    CASTLE_SAMPLE_CHECK(tree.insert(60U, 600U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(tree.size() == 5U);

    const uint16_t ordered_keys[5] = {10U, 20U, 30U, 40U, 50U};
    castle::size_type index = 0U;
    for (auto it = tree.begin(); it != tree.end(); ++it, ++index)
    {
        CASTLE_SAMPLE_CHECK(it->first == ordered_keys[index]);
    }
    CASTLE_SAMPLE_CHECK(index == 5U);

    auto found = tree.find(20U);
    CASTLE_SAMPLE_CHECK(found != tree.end());
    found->second = 250U;
    CASTLE_SAMPLE_CHECK(tree.contains(40U));
    CASTLE_SAMPLE_CHECK(*tree.mapped(40U) == 400U);
    *tree.mapped(40U) = 401U;

    auto before = tree.find(30U);
    --before;
    CASTLE_SAMPLE_CHECK(before->first == 20U);
    ++before;
    CASTLE_SAMPLE_CHECK(before->first == 30U);

    CASTLE_SAMPLE_CHECK(tree.lower_bound(15U)->first == 20U);
    CASTLE_SAMPLE_CHECK(tree.upper_bound(40U)->first == 50U);

    const auto& ctree = tree;
    CASTLE_SAMPLE_CHECK(ctree.find(50U) != ctree.cend());
    CASTLE_SAMPLE_CHECK(ctree.lower_bound(21U)->first == 30U);
    CASTLE_SAMPLE_CHECK(ctree.upper_bound(20U)->first == 30U);
    CASTLE_SAMPLE_CHECK(*ctree.mapped(40U) == 401U);

    auto next = tree.erase(tree.find(30U));
    CASTLE_SAMPLE_CHECK(next != tree.end());
    CASTLE_SAMPLE_CHECK(next->first == 40U);
    CASTLE_SAMPLE_CHECK(!tree.contains(30U));

    CASTLE_SAMPLE_CHECK(tree.erase(10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(tree.erase(10U) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(tree.size() == 3U);

    tree.clear();
    CASTLE_SAMPLE_CHECK(tree.empty());
    CASTLE_SAMPLE_CHECK(tree.begin() == tree.end());
    return 0;
}
