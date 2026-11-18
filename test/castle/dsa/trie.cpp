#include <gtest/gtest.h>

#include "castle/dsa/trie.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::contiguous_alphabet;
using castle::dsa::digit_alphabet;
using castle::dsa::lowercase_alphabet;
using castle::dsa::trie;

using binary_alphabet = contiguous_alphabet<char, 'a', 2U>;

// Non-contiguous alphabet: the four DNA bases.
struct dna_alphabet
{
    using symbol_type = char;
    static constexpr size_type size = 4U;

    static constexpr size_type index_of(char symbol) noexcept
    {
        return symbol == 'A' ? 0U : symbol == 'C' ? 1U : symbol == 'G' ? 2U : symbol == 'T' ? 3U : size;
    }
};

uint32_t next_random(uint32_t& state)
{
    state = state * 1664525U + 1013904223U;
    return state >> 8U;
}

TEST(TrieTest, NodeCapacityFollowsDepthWidthAndKeyBounds)
{
    // Complete trie by default: every key of 0..3 digits, one node per key.
    static_assert(trie<digit_alphabet, 3U>::static_capacity == 1111U, "1 + 10 + 100 + 1000 keys");
    static_assert(trie<digit_alphabet, 3U>::static_node_capacity == 1111U, "one node per key");
    // Layers are capped by the key count: 1 + min(2,3) + min(4,3) + min(8,3).
    static_assert(trie<binary_alphabet, 3U, 3U>::static_node_capacity == 9U, "layer bound");
    // LeetCode 212 bounds: 1 + 26 + 676 + 17576 + 7 * 30000.
    static_assert(trie<lowercase_alphabet, 10U, 30000U>::static_node_capacity == 228279U, "word search bound");

    static_assert(sizeof(trie<binary_alphabet, 3U, 3U>::node_handle) == 1U, "9 nodes fit uint8_t");
    static_assert(sizeof(trie<digit_alphabet, 3U>::node_handle) == 2U, "1111 nodes need uint16_t");
    static_assert(sizeof(trie<lowercase_alphabet, 10U, 30000U>::node_handle) == 4U, "228279 nodes need uint32_t");
    static_assert(sizeof(trie<lowercase_alphabet, 10U, 30000U>::count_type) == 2U, "30000 keys fit uint16_t");
    static_assert(trie<binary_alphabet, 3U, 3U>::npos == 9U, "npos is one past the pool");
    static_assert(trie<lowercase_alphabet, 4U, 5U>::static_width == 26U, "width from alphabet");
    static_assert(trie<lowercase_alphabet, 4U, 5U>::static_max_depth == 4U, "depth");

    // Saturating helpers.
    static_assert(castle::dsa::detail::trie_saturating_add(static_cast<size_type>(-2), 5U) == static_cast<size_type>(-1),
                  "saturates on overflow");
    static_assert(castle::dsa::detail::trie_full_key_count(26U, 20U) == static_cast<size_type>(-1),
                  "26^20 keys saturate");
    EXPECT_EQ(castle::dsa::detail::trie_node_capacity(26U, 2U, 1000U), 1U + 26U + 676U);
    EXPECT_EQ(castle::dsa::detail::trie_node_capacity(2U, 3U, 3U), 9U);
    EXPECT_EQ(castle::dsa::detail::trie_saturating_add(static_cast<size_type>(-2), 5U), static_cast<size_type>(-1));
}

TEST(TrieTest, ContiguousAlphabetMapsInsideAndRejectsOutside)
{
    EXPECT_EQ(lowercase_alphabet::index_of('a'), 0U);
    EXPECT_EQ(lowercase_alphabet::index_of('z'), 25U);
    EXPECT_GE(lowercase_alphabet::index_of('A'), 26U);
    EXPECT_GE(lowercase_alphabet::index_of('{'), 26U);
    EXPECT_GE(lowercase_alphabet::index_of(static_cast<char>(-100)), 26U);
    EXPECT_EQ(digit_alphabet::index_of('7'), 7U);
    EXPECT_GE(digit_alphabet::index_of('/'), 10U);

    using high_bytes = contiguous_alphabet<uint8_t, 0xF0U, 16U>;
    EXPECT_EQ(high_bytes::index_of(0xFFU), 15U);
    EXPECT_GE(high_bytes::index_of(0x00U), 16U);

    using signed_range = contiguous_alphabet<int32_t, -2, 5>;
    EXPECT_EQ(signed_range::index_of(-2), 0U);
    EXPECT_EQ(signed_range::index_of(2), 4U);
    EXPECT_GE(signed_range::index_of(-3), 5U);
    EXPECT_GE(signed_range::index_of(3), 5U);
}

TEST(TrieTest, EmptyTrieState)
{
    trie<lowercase_alphabet, 4U, 8U> words;
    EXPECT_TRUE(words.empty());
    EXPECT_FALSE(words.full());
    EXPECT_EQ(words.size(), 0U);
    EXPECT_EQ(words.capacity(), 8U);
    EXPECT_EQ(words.max_depth(), 4U);
    EXPECT_EQ(words.node_count(), 1U);
    EXPECT_EQ(words.node_capacity(), (trie<lowercase_alphabet, 4U, 8U>::static_node_capacity));
    EXPECT_FALSE(words.contains(""));
    EXPECT_FALSE(words.contains_prefix(""));
    EXPECT_EQ(words.prefix_count(""), 0U);
    EXPECT_EQ(words.find(""), words.root());
    EXPECT_EQ(words.find("a"), words.npos);
    EXPECT_EQ(words.advance(words.root(), 'a'), words.npos);
    EXPECT_FALSE(words.is_terminal(words.root()));
}

TEST(TrieTest, InsertSharesPrefixesAndCountsKeys)
{
    trie<lowercase_alphabet, 6U, 8U, uint16_t> words;
    EXPECT_EQ(words.insert("car", 1U), status::ok);
    EXPECT_EQ(words.insert("cart", 2U), status::ok);
    EXPECT_EQ(words.insert("care", 3U), status::ok);
    EXPECT_EQ(words.insert("dog", 4U), status::ok);

    EXPECT_EQ(words.size(), 4U);
    EXPECT_EQ(words.node_count(), 9U); // root + c,a,r,t,e + d,o,g
    EXPECT_TRUE(words.contains("car"));
    EXPECT_TRUE(words.contains("cart"));
    EXPECT_FALSE(words.contains("ca"));
    EXPECT_FALSE(words.contains("carts"));
    EXPECT_FALSE(words.contains("cat"));

    EXPECT_TRUE(words.contains_prefix("ca"));
    EXPECT_FALSE(words.contains_prefix("cb"));
    EXPECT_EQ(words.prefix_count(""), 4U);
    EXPECT_EQ(words.prefix_count("c"), 3U);
    EXPECT_EQ(words.prefix_count("car"), 3U);
    EXPECT_EQ(words.prefix_count("cart"), 1U);
    EXPECT_EQ(words.prefix_count("x"), 0U);

    uint16_t value = 0U;
    EXPECT_EQ(words.try_value(words.find("care"), value), status::ok);
    EXPECT_EQ(value, 3U);
    value = 77U;
    EXPECT_EQ(words.try_value(words.find("ca"), value), status::not_found);
    EXPECT_EQ(words.try_value(words.npos, value), status::not_found);
    EXPECT_EQ(value, 77U);
}

TEST(TrieTest, NavigationByHandle)
{
    trie<lowercase_alphabet, 6U, 8U> words;
    ASSERT_EQ(words.insert("to"), status::ok);
    ASSERT_EQ(words.insert("tea"), status::ok);

    const auto t = words.advance(words.root(), 't');
    ASSERT_NE(t, words.npos);
    EXPECT_EQ(words.prefix_count(t), 2U);
    EXPECT_FALSE(words.is_terminal(t));
    const auto to = words.advance(t, 'o');
    EXPECT_TRUE(words.is_terminal(to));
    EXPECT_EQ(words.prefix_count(to), 1U);
    EXPECT_EQ(words.advance(t, 'x'), words.npos);
    EXPECT_EQ(words.advance(t, 'T'), words.npos); // outside the alphabet
    EXPECT_EQ(words.advance(words.npos, 'a'), words.npos);
    EXPECT_EQ(words.advance(words.advance(t, 'e'), 'a'), words.find("tea")); // chaining
    EXPECT_EQ(words.prefix_count(words.npos), 0U);
    EXPECT_FALSE(words.is_terminal(words.npos));
    EXPECT_EQ(words.find("toe"), words.npos);
}

TEST(TrieTest, RejectedInsertionsLeaveTrieUnchanged)
{
    trie<lowercase_alphabet, 3U, 2U, uint8_t> words;
    EXPECT_EQ(words.insert("ab", 5U), status::ok);
    EXPECT_EQ(words.insert("ab", 9U), status::already_exists);
    EXPECT_EQ(words.insert("abcd"), status::out_of_range);
    EXPECT_EQ(words.insert("aB"), status::invalid_argument);
    EXPECT_EQ(words.insert("a1c"), status::invalid_argument);
    EXPECT_EQ(words.size(), 1U);
    EXPECT_EQ(words.node_count(), 3U);

    uint8_t value = 0U;
    EXPECT_EQ(words.try_value(words.find("ab"), value), status::ok);
    EXPECT_EQ(value, 5U);

    EXPECT_EQ(words.insert("abc"), status::ok);
    EXPECT_TRUE(words.full());
    EXPECT_EQ(words.insert("b"), status::full);
    EXPECT_EQ(words.insert("ab"), status::already_exists); // duplicates are reported before fullness
    EXPECT_EQ(words.node_count(), 4U);
    EXPECT_FALSE(words.contains("b"));
}

TEST(TrieTest, EmptyKeyLivesAtRoot)
{
    trie<lowercase_alphabet, 2U, 3U, uint8_t> words;
    EXPECT_EQ(words.insert("", 7U), status::ok);
    EXPECT_TRUE(words.contains(""));
    EXPECT_TRUE(words.is_terminal(words.root()));
    EXPECT_EQ(words.size(), 1U);
    EXPECT_EQ(words.node_count(), 1U);
    EXPECT_EQ(words.insert("a"), status::ok);
    EXPECT_EQ(words.prefix_count(""), 2U);

    EXPECT_EQ(words.erase(""), status::ok);
    EXPECT_FALSE(words.contains(""));
    EXPECT_TRUE(words.contains("a"));
    EXPECT_EQ(words.erase(""), status::not_found);
}

TEST(TrieTest, EraseFreesOnlyUnsharedNodes)
{
    trie<lowercase_alphabet, 6U, 8U, uint16_t> words;
    ASSERT_EQ(words.insert("car", 1U), status::ok);
    ASSERT_EQ(words.insert("cart", 2U), status::ok);
    ASSERT_EQ(words.insert("dog", 3U), status::ok);
    ASSERT_EQ(words.node_count(), 8U);

    // A key that is a prefix of another keeps every node.
    EXPECT_EQ(words.erase("car"), status::ok);
    EXPECT_FALSE(words.contains("car"));
    EXPECT_TRUE(words.contains("cart"));
    EXPECT_EQ(words.node_count(), 8U);
    uint16_t value = 0U;
    EXPECT_EQ(words.try_value(words.find("car"), value), status::not_found);

    // The only key below "c" frees the whole branch.
    EXPECT_EQ(words.erase("cart"), status::ok);
    EXPECT_EQ(words.node_count(), 4U);
    EXPECT_EQ(words.find("c"), words.npos);
    EXPECT_FALSE(words.contains_prefix("c"));

    EXPECT_EQ(words.erase("cart"), status::not_found);
    EXPECT_EQ(words.erase("do"), status::not_found);   // prefix, not a key
    EXPECT_EQ(words.erase("dogs"), status::not_found); // longer than any path
    EXPECT_EQ(words.erase("D"), status::not_found);    // outside the alphabet
    EXPECT_EQ(words.erase("abcdefg"), status::not_found);

    EXPECT_EQ(words.erase("dog"), status::ok);
    EXPECT_TRUE(words.empty());
    EXPECT_EQ(words.node_count(), 1U);
    EXPECT_EQ(words.advance(words.root(), 'd'), words.npos);
}

TEST(TrieTest, HandlesToFreedNodesAreInertUntilNextInsert)
{
    trie<lowercase_alphabet, 4U, 4U> words;
    ASSERT_EQ(words.insert("ab"), status::ok);
    ASSERT_EQ(words.insert("cd"), status::ok);
    const auto c = words.find("c");
    const auto d = words.find("cd");

    ASSERT_EQ(words.erase("ab"), status::ok);
    ASSERT_EQ(words.erase("cd"), status::ok); // "c" now links the free list through its first child slot
    EXPECT_EQ(words.prefix_count(c), 0U);
    EXPECT_FALSE(words.is_terminal(d));
    EXPECT_EQ(words.advance(c, 'a'), words.npos);
    EXPECT_EQ(words.advance(c, 'd'), words.npos);
    EXPECT_EQ(words.node_count(), 1U);

    // Freed nodes are reused; the pool never grows past its bound.
    ASSERT_EQ(words.insert("dcba"), status::ok);
    EXPECT_EQ(words.node_count(), 5U);
    EXPECT_TRUE(words.contains("dcba"));
}

TEST(TrieTest, RandomOperationsMatchReferenceAndRespectPoolBound)
{
    // Every key over {a, b} of length 0..3: 15 keys. At most 3 stored at once; pool of 9 nodes.
    using small_trie = trie<binary_alphabet, 3U, 3U, uint8_t>;
    const char* const keys[] = {"", "a", "b", "aa", "ab", "ba", "bb",
                                "aaa", "aab", "aba", "abb", "baa", "bab", "bba", "bbb"};
    constexpr size_type key_count = 15U;

    small_trie words;
    bool stored[key_count] = {};
    size_type stored_count = 0U;
    uint32_t state = 12345U;

    for (size_type step = 0U; step < 3000U; ++step)
    {
        const size_type k = next_random(state) % key_count;
        if ((next_random(state) & 1U) != 0U)
        {
            const status expected = stored[k] ? status::already_exists
                                  : (stored_count == 3U) ? status::full
                                  : status::ok;
            ASSERT_EQ(words.insert(keys[k], static_cast<uint8_t>(k)), expected);
            if (expected == status::ok)
            {
                stored[k] = true;
                ++stored_count;
            }
        }
        else
        {
            ASSERT_EQ(words.erase(keys[k]), stored[k] ? status::ok : status::not_found);
            if (stored[k])
            {
                stored[k] = false;
                --stored_count;
            }
        }

        // Reference: keys starting with each prefix, and the live prefix count (= nodes in use).
        size_type live_prefixes = 1U; // the root
        for (size_type p = 0U; p < key_count; ++p)
        {
            const castle::dsa::string_view prefix(keys[p]);
            size_type below = 0U;
            for (size_type q = 0U; q < key_count; ++q)
            {
                if (stored[q] && castle::dsa::string_view(keys[q]).starts_with(prefix))
                {
                    ++below;
                }
            }
            ASSERT_EQ(words.prefix_count(prefix), below);
            ASSERT_EQ(words.contains(prefix), stored[p]);
            if ((p > 0U) && (below > 0U))
            {
                ++live_prefixes;
            }
            uint8_t value = 0xFFU;
            ASSERT_EQ(words.try_value(words.find(prefix), value), stored[p] ? status::ok : status::not_found);
            if (stored[p])
            {
                ASSERT_EQ(value, p);
            }
        }
        ASSERT_EQ(words.size(), stored_count);
        ASSERT_EQ(words.node_count(), live_prefixes);
        ASSERT_LE(words.node_count(), words.node_capacity());
    }
}

TEST(TrieTest, DisjointKeysFillThePoolExactly)
{
    trie<binary_alphabet, 3U, 3U> words;
    ASSERT_EQ(words.insert("aaa"), status::ok);
    ASSERT_EQ(words.insert("abb"), status::ok);
    ASSERT_EQ(words.insert("bab"), status::ok);
    EXPECT_EQ(words.node_count(), words.node_capacity());
    for (size_type round = 0U; round < 4U; ++round)
    {
        ASSERT_EQ(words.erase("bab"), status::ok);
        ASSERT_EQ(words.insert("bba"), status::ok);
        ASSERT_EQ(words.erase("bba"), status::ok);
        ASSERT_EQ(words.insert("bab"), status::ok);
    }
    EXPECT_EQ(words.node_count(), words.node_capacity());
}

TEST(TrieTest, ClearReturnsEveryNode)
{
    trie<lowercase_alphabet, 4U, 4U> words;
    ASSERT_EQ(words.insert("abc"), status::ok);
    ASSERT_EQ(words.insert("abd"), status::ok);
    ASSERT_EQ(words.erase("abd"), status::ok); // leaves a node on the free list
    words.clear();
    EXPECT_TRUE(words.empty());
    EXPECT_EQ(words.node_count(), 1U);
    EXPECT_FALSE(words.contains("abc"));
    EXPECT_EQ(words.find("a"), words.npos);

    ASSERT_EQ(words.insert("zz"), status::ok);
    EXPECT_TRUE(words.contains("zz"));
    EXPECT_EQ(words.node_count(), 3U);
}

TEST(TrieTest, CustomAlphabetAndPayload)
{
    struct gene
    {
        uint16_t id;
        bool coding;
    };
    trie<dna_alphabet, 5U, 4U, gene> genome;
    EXPECT_EQ(genome.insert("GATTA", gene{7U, true}), status::ok);
    EXPECT_EQ(genome.insert("GAC", gene{9U, false}), status::ok);
    EXPECT_EQ(genome.insert("GAX"), status::invalid_argument);
    EXPECT_EQ(genome.prefix_count("GA"), 2U);

    gene found{0U, false};
    EXPECT_EQ(genome.try_value(genome.find("GATTA"), found), status::ok);
    EXPECT_EQ(found.id, 7U);
    EXPECT_TRUE(found.coding);
}

TEST(TrieTest, NonCharSymbolsUseExplicitLengthViews)
{
    using byte_trie = trie<contiguous_alphabet<uint8_t, 0U, 4U>, 4U, 4U>;
    byte_trie codes;
    const uint8_t first[] = {0U, 3U, 1U};
    const uint8_t second[] = {0U, 3U};
    const uint8_t invalid[] = {4U};
    EXPECT_EQ(codes.insert(byte_trie::key_type(first, 3U)), status::ok);
    EXPECT_EQ(codes.insert(byte_trie::key_type(invalid, 1U)), status::invalid_argument);
    EXPECT_TRUE(codes.contains(byte_trie::key_type(first, 3U)));
    EXPECT_FALSE(codes.contains(byte_trie::key_type(second, 2U)));
    EXPECT_EQ(codes.prefix_count(byte_trie::key_type(second, 2U)), 1U);
}

// LeetCode 212: depth-first search that erases each word as it is found.
template <size_type Rows, size_type Columns>
struct word_search
{
    using dictionary = trie<lowercase_alphabet, 10U, 8U, uint8_t>;

    char (&board)[Rows][Columns];
    const char* const* words;
    dictionary& remaining;
    uint8_t found_mask;

    void visit(size_type row, size_type column, dictionary::node_handle parent)
    {
        const char letter = board[row][column];
        const dictionary::node_handle node = remaining.advance(parent, letter);
        if (node == dictionary::npos)
        {
            return;
        }
        uint8_t word = 0U;
        if (remaining.try_value(node, word) == status::ok)
        {
            found_mask = static_cast<uint8_t>(found_mask | (1U << word));
            EXPECT_EQ(remaining.erase(words[word]), status::ok);
        }
        board[row][column] = '#';
        if (row > 0U)
        {
            visit(row - 1U, column, node);
        }
        if (row + 1U < Rows)
        {
            visit(row + 1U, column, node);
        }
        if (column > 0U)
        {
            visit(row, column - 1U, node);
        }
        if (column + 1U < Columns)
        {
            visit(row, column + 1U, node);
        }
        board[row][column] = letter;
    }

    uint8_t run(size_type count)
    {
        for (size_type i = 0U; i < count; ++i)
        {
            EXPECT_EQ(remaining.insert(words[i], static_cast<uint8_t>(i)), status::ok);
        }
        for (size_type row = 0U; row < Rows; ++row)
        {
            for (size_type column = 0U; column < Columns; ++column)
            {
                visit(row, column, remaining.root());
            }
        }
        return found_mask;
    }
};

TEST(TrieTest, SolvesWordSearchTwoExamples)
{
    {
        char board[4][4] = {{'o', 'a', 'a', 'n'}, {'e', 't', 'a', 'e'}, {'i', 'h', 'k', 'r'}, {'i', 'f', 'l', 'v'}};
        const char* const words[] = {"oath", "pea", "eat", "rain"};
        word_search<4U, 4U>::dictionary remaining;
        word_search<4U, 4U> search{board, words, remaining, 0U};
        EXPECT_EQ(search.run(4U), 0x5U); // "oath" and "eat"
        EXPECT_EQ(remaining.size(), 2U);
        EXPECT_EQ(remaining.node_count(), 1U + 3U + 4U); // only "pea" and "rain" remain
    }
    {
        char board[2][2] = {{'a', 'b'}, {'c', 'd'}};
        const char* const words[] = {"abcb"};
        word_search<2U, 2U>::dictionary remaining;
        word_search<2U, 2U> search{board, words, remaining, 0U};
        EXPECT_EQ(search.run(1U), 0U);
    }
    {
        // Overlapping words: finding "ab" must not hide "abc", and the pruned "ab" path stays inert.
        char board[1][3] = {{'a', 'b', 'c'}};
        const char* const words[] = {"ab", "abc", "cb"};
        word_search<1U, 3U>::dictionary remaining;
        word_search<1U, 3U> search{board, words, remaining, 0U};
        EXPECT_EQ(search.run(3U), 0x7U);
        EXPECT_TRUE(remaining.empty());
        EXPECT_EQ(remaining.node_count(), 1U);
    }
}
} // namespace
