#include "sample_support.hpp"

#include "castle/dsa/grid.hpp"
#include "castle/dsa/trie.hpp"
#include "castle/dsa/vector.hpp"

namespace
{
// LeetCode 212 (Word Search II): words are lowercase and at most 10 letters long.
constexpr castle::size_type k_max_word_length = 10U;
constexpr castle::size_type k_max_words = 16U;

// The payload of each word is its index in the input list.
using dictionary = castle::dsa::trie<castle::dsa::lowercase_alphabet, k_max_word_length, k_max_words, uint16_t>;
using found_words = castle::dsa::vector<uint16_t, k_max_words>;

constexpr char k_visited = '#'; // outside the alphabet, so advance() never matches it

template <castle::size_type Rows, castle::size_type Columns>
struct board_search
{
    castle::dsa::grid<char, Rows, Columns>& board;
    const char* const* words;
    dictionary& remaining;
    found_words& found;

    // Depth-first walk that follows the trie; recursion depth is bounded by k_max_word_length.
    void visit(castle::size_type row, castle::size_type column, dictionary::node_handle parent)
    {
        const char letter = board(row, column);
        const dictionary::node_handle node = remaining.advance(parent, letter);
        if (node == dictionary::npos)
        {
            return;
        }

        uint16_t word = 0U;
        if (remaining.try_value(node, word) == castle::status::ok)
        {
            CASTLE_SAMPLE_CHECK(found.push_back(word) == castle::status::ok);
            // Report each word once. Erasing frees the trie nodes no other word needs,
            // which prunes the rest of the search.
            CASTLE_SAMPLE_CHECK(remaining.erase(words[word]) == castle::status::ok);
        }
        if (remaining.prefix_count(node) == 0U)
        {
            return; // every word below this prefix is found; the node is freed and inert
        }

        board(row, column) = k_visited;
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
        board(row, column) = letter;
    }
};

template <castle::size_type Rows, castle::size_type Columns>
found_words find_words(castle::dsa::grid<char, Rows, Columns>& board, const char* const* words, castle::size_type count)
{
    dictionary remaining;
    for (castle::size_type i = 0U; i < count; ++i)
    {
        CASTLE_SAMPLE_CHECK(remaining.insert(words[i], static_cast<uint16_t>(i)) == castle::status::ok);
    }

    found_words found;
    board_search<Rows, Columns> search{board, words, remaining, found};
    for (castle::size_type row = 0U; (row < Rows) && !remaining.empty(); ++row)
    {
        for (castle::size_type column = 0U; column < Columns; ++column)
        {
            search.visit(row, column, remaining.root());
        }
    }
    return found;
}

bool contains(const found_words& found, uint16_t word)
{
    for (const uint16_t entry : found)
    {
        if (entry == word)
        {
            return true;
        }
    }
    return false;
}
} // namespace

int main()
{
    // Example 1: board [["o","a","a","n"],["e","t","a","e"],["i","h","k","r"],["i","f","l","v"]].
    {
        castle::dsa::grid<char, 4U, 4U> board{'o', 'a', 'a', 'n',
                                              'e', 't', 'a', 'e',
                                              'i', 'h', 'k', 'r',
                                              'i', 'f', 'l', 'v'};
        const char* const words[] = {"oath", "pea", "eat", "rain"};
        const found_words found = find_words(board, words, 4U);
        CASTLE_SAMPLE_CHECK(found.size() == 2U);
        CASTLE_SAMPLE_CHECK(contains(found, 0U)); // "oath"
        CASTLE_SAMPLE_CHECK(contains(found, 2U)); // "eat"
        CASTLE_SAMPLE_CHECK(board(0U, 0U) == 'o'); // the board is restored
    }

    // Example 2: board [["a","b"],["c","d"]], words ["abcb"] -> no word (a cell is used once).
    {
        castle::dsa::grid<char, 2U, 2U> board{'a', 'b',
                                              'c', 'd'};
        const char* const words[] = {"abcb"};
        CASTLE_SAMPLE_CHECK(find_words(board, words, 1U).empty());
    }

    // Shared prefixes: "abc" and "abcd" are both on this board.
    {
        castle::dsa::grid<char, 2U, 2U> board{'a', 'b',
                                              'd', 'c'};
        const char* const words[] = {"abc", "abcd", "ad", "bd"};
        const found_words found = find_words(board, words, 4U);
        CASTLE_SAMPLE_CHECK(found.size() == 3U);
        CASTLE_SAMPLE_CHECK(contains(found, 0U) && contains(found, 1U) && contains(found, 2U));
    }

    // Prefix lookups over a digit alphabet: a dial-plan of up to 8 routes, 6 digits each.
    {
        castle::dsa::trie<castle::dsa::digit_alphabet, 6U, 8U, uint8_t> routes;
        CASTLE_SAMPLE_CHECK(routes.insert("112", 1U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(routes.insert("1800", 2U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(routes.insert("1801", 2U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(routes.insert("112", 9U) == castle::status::already_exists);
        CASTLE_SAMPLE_CHECK(routes.insert("12a") == castle::status::invalid_argument);
        CASTLE_SAMPLE_CHECK(routes.insert("1234567") == castle::status::out_of_range);

        CASTLE_SAMPLE_CHECK(routes.prefix_count("18") == 2U);
        CASTLE_SAMPLE_CHECK(routes.contains_prefix("180"));
        CASTLE_SAMPLE_CHECK(!routes.contains("180"));

        uint8_t route = 0U;
        CASTLE_SAMPLE_CHECK(routes.try_value(routes.find("112"), route) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(route == 1U);

        CASTLE_SAMPLE_CHECK(routes.erase("1800") == castle::status::ok);
        CASTLE_SAMPLE_CHECK(routes.prefix_count("18") == 1U);
        CASTLE_SAMPLE_CHECK(routes.size() == 2U);
    }

    return 0;
}
