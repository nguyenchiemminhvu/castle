#include "sample_support.hpp"

#include "castle/algorithm/algorithm.hpp"

int main()
{
    int values[] = {4, 1, 7, 1, 9, 2, 1, 5};
    int comparison[] = {4, 1, 7, 1, 9, 2, 1, 5};
    int copy_buffer[8] = {};
    int filtered[8] = {};
    int squares[8] = {};
    int sums[8] = {};

    CASTLE_SAMPLE_CHECK(castle::find(values, values + 8, 7) == values + 2);
    CASTLE_SAMPLE_CHECK(castle::find_if(values, values + 8, [](int value) { return value > 8; }) == values + 4);
    CASTLE_SAMPLE_CHECK(castle::find_if_not(values, values + 8, [](int value) { return value < 9; }) == values + 4);
    CASTLE_SAMPLE_CHECK(castle::count(values, values + 8, 1) == 3U);
    CASTLE_SAMPLE_CHECK(castle::count_if(values, values + 8, [](int value) { return (value % 2) != 0; }) == 6U);
    CASTLE_SAMPLE_CHECK(castle::all_of(values, values + 8, [](int value) { return value > 0; }));
    CASTLE_SAMPLE_CHECK(castle::any_of(values, values + 8, [](int value) { return value == 9; }));
    CASTLE_SAMPLE_CHECK(castle::none_of(values, values + 8, [](int value) { return value < 0; }));
    CASTLE_SAMPLE_CHECK(castle::equal(values, values + 8, comparison));
    CASTLE_SAMPLE_CHECK(castle::equal(values, values + 8, comparison, comparison + 8));

    comparison[3] = 8;
    const auto mismatch_pair = castle::mismatch(values, values + 8, comparison);
    CASTLE_SAMPLE_CHECK(mismatch_pair.first == values + 3);
    CASTLE_SAMPLE_CHECK(mismatch_pair.second == comparison + 3);

    CASTLE_SAMPLE_CHECK(castle::copy(values, values + 8, copy_buffer) == copy_buffer + 8);
    CASTLE_SAMPLE_CHECK(castle::equal(values, values + 8, copy_buffer));
    CASTLE_SAMPLE_CHECK(castle::copy_n(values, 3, copy_buffer) == copy_buffer + 3);
    CASTLE_SAMPLE_CHECK(copy_buffer[0] == 4 && copy_buffer[1] == 1 && copy_buffer[2] == 7);
    CASTLE_SAMPLE_CHECK(castle::copy_if(values, values + 8, filtered, [](int value) { return value > 4; }) == filtered + 3);
    CASTLE_SAMPLE_CHECK(filtered[0] == 7 && filtered[1] == 9 && filtered[2] == 5);

    int moved[3] = {1, 2, 3};
    int moved_to[3] = {};
    CASTLE_SAMPLE_CHECK(castle::move(moved, moved + 3, moved_to) == moved_to + 3);
    CASTLE_SAMPLE_CHECK(moved_to[0] == 1 && moved_to[1] == 2 && moved_to[2] == 3);

    int filled[4] = {};
    castle::fill(filled, filled + 4, 3);
    CASTLE_SAMPLE_CHECK(filled[0] == 3 && filled[3] == 3);
    CASTLE_SAMPLE_CHECK(castle::fill_n(filled, 2, 9) == filled + 2);
    CASTLE_SAMPLE_CHECK(filled[0] == 9 && filled[1] == 9 && filled[2] == 3);

    CASTLE_SAMPLE_CHECK(castle::transform(values, values + 8, squares, [](int value) { return value * value; }) == squares + 8);
    CASTLE_SAMPLE_CHECK(squares[0] == 16 && squares[4] == 81);
    CASTLE_SAMPLE_CHECK(castle::transform(values, values + 8, comparison, sums, [](int lhs, int rhs) { return lhs + rhs; }) == sums + 8);
    CASTLE_SAMPLE_CHECK(sums[0] == 8 && sums[3] == 9);

    int generated[4] = {};
    int generator = 0;
    castle::generate(generated, generated + 4, [&generator]() { return ++generator; });
    CASTLE_SAMPLE_CHECK(generated[0] == 1 && generated[3] == 4);

    int replaced[] = {1, 2, 2, 3};
    castle::replace(replaced, replaced + 4, 2, 5);
    castle::replace_if(replaced, replaced + 4, [](int value) { return value > 4; }, 8);
    CASTLE_SAMPLE_CHECK(replaced[1] == 8 && replaced[2] == 8);

    int removed[] = {1, 2, 3, 2, 4};
    int* removed_end = castle::remove(removed, removed + 5, 2);
    CASTLE_SAMPLE_CHECK(removed_end == removed + 3);
    CASTLE_SAMPLE_CHECK(removed[0] == 1 && removed[1] == 3 && removed[2] == 4);

    int removed_if[] = {1, 2, 3, 4, 5};
    int* removed_if_end = castle::remove_if(removed_if, removed_if + 5, [](int value) { return (value % 2) == 0; });
    CASTLE_SAMPLE_CHECK(removed_if_end == removed_if + 3);
    CASTLE_SAMPLE_CHECK(removed_if[0] == 1 && removed_if[1] == 3 && removed_if[2] == 5);

    int unique_values[] = {1, 1, 2, 2, 3, 3};
    int* unique_end = castle::unique(unique_values, unique_values + 6);
    CASTLE_SAMPLE_CHECK(unique_end == unique_values + 3);
    CASTLE_SAMPLE_CHECK(unique_values[0] == 1 && unique_values[1] == 2 && unique_values[2] == 3);

    int custom_unique[] = {1, 3, 5, 2, 4, 6};
    int* custom_unique_end = castle::unique(custom_unique, custom_unique + 6, [](int lhs, int rhs) { return (lhs % 2) == (rhs % 2); });
    CASTLE_SAMPLE_CHECK(custom_unique_end == custom_unique + 2);
    CASTLE_SAMPLE_CHECK(custom_unique[0] == 1 && custom_unique[1] == 2);

    int sortable[] = {9, 2, 7, 1, 5, 4, 8, 3};
    castle::sort(sortable, sortable + 8);
    CASTLE_SAMPLE_CHECK(sortable[0] == 1 && sortable[7] == 9);
    castle::sort(sortable, sortable + 8, [](int lhs, int rhs) { return lhs > rhs; });
    CASTLE_SAMPLE_CHECK(sortable[0] == 9 && sortable[7] == 1);

    int ordered[] = {1, 2, 3, 4, 5, 7, 7, 9};
    CASTLE_SAMPLE_CHECK(castle::lower_bound(ordered, ordered + 8, 7) == ordered + 5);
    CASTLE_SAMPLE_CHECK(castle::upper_bound(ordered, ordered + 8, 7) == ordered + 7);
    CASTLE_SAMPLE_CHECK(castle::binary_search(ordered, ordered + 8, 5));
    CASTLE_SAMPLE_CHECK(!castle::binary_search(ordered, ordered + 8, 6));

    CASTLE_SAMPLE_CHECK(castle::min(4, 2) == 2);
    CASTLE_SAMPLE_CHECK(castle::max(4, 2) == 4);
    CASTLE_SAMPLE_CHECK(castle::min3(7, 3, 5) == 3);
    CASTLE_SAMPLE_CHECK(castle::max3(7, 3, 5) == 7);
    CASTLE_SAMPLE_CHECK(castle::min_element(ordered, ordered + 8) == ordered);
    CASTLE_SAMPLE_CHECK(castle::max_element(ordered, ordered + 8) == ordered + 7);

    return 0;
}
