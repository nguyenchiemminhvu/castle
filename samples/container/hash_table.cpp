#include "sample_support.hpp"

#include "castle/container/hash_table.hpp"

#include <stdint.h>

namespace
{
struct modulo_hash
{
    castle::size_type operator()(const uint16_t& value) const noexcept
    {
        return static_cast<castle::size_type>(value % 4U);
    }
};
} // namespace

int main()
{
    castle::container::hash_table<uint16_t, uint32_t, 4U, modulo_hash> table{modulo_hash()};
    CASTLE_SAMPLE_CHECK(table.capacity() == 4U);
    CASTLE_SAMPLE_CHECK(table.available() == 4U);
    CASTLE_SAMPLE_CHECK(table.empty());

    CASTLE_SAMPLE_CHECK(table.insert(1U, 10U) == castle::status::ok);
    uint32_t moved = 50U;
    CASTLE_SAMPLE_CHECK(table.insert(5U, castle::move(moved)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.emplace(9U, 90U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.try_emplace(13U, 130U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.full());
    CASTLE_SAMPLE_CHECK(table.insert(17U, 170U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(table.insert(5U, 500U) == castle::status::already_exists);

    CASTLE_SAMPLE_CHECK(table.contains(5U));
    CASTLE_SAMPLE_CHECK(*table.get(5U) == 50U);

    auto it = table.find(9U);
    CASTLE_SAMPLE_CHECK(it != table.end());
    it->second = 99U;
    CASTLE_SAMPLE_CHECK(*table.get(9U) == 99U);

    const auto& ctable = table;
    CASTLE_SAMPLE_CHECK(ctable.find(1U) != ctable.cend());
    CASTLE_SAMPLE_CHECK(ctable.get(13U) != nullptr);

    castle::size_type iterated = 0U;
    for (auto entry = table.begin(); entry != table.end(); ++entry)
    {
        ++iterated;
    }
    CASTLE_SAMPLE_CHECK(iterated == table.size());

    CASTLE_SAMPLE_CHECK(table.insert_or_assign(5U, 55U) == castle::status::ok);
    uint32_t reassigned = 56U;
    CASTLE_SAMPLE_CHECK(table.insert_or_assign(5U, castle::move(reassigned)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(*table.get(5U) == 56U);

    CASTLE_SAMPLE_CHECK(table.erase(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!table.contains(1U));
    CASTLE_SAMPLE_CHECK(table.erase(1U) == castle::status::not_found);

    CASTLE_SAMPLE_CHECK(table.insert(17U, 170U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.contains(17U));

    table.clear();
    CASTLE_SAMPLE_CHECK(table.empty());
    CASTLE_SAMPLE_CHECK(table.begin() == table.end());

    castle::container::hash_table<uint16_t, uint32_t, 4U> listed{{castle::pair<const uint16_t, uint32_t>(1U, 10U),
                                                                   castle::pair<const uint16_t, uint32_t>(1U, 11U),
                                                                   castle::pair<const uint16_t, uint32_t>(2U, 20U)}};
    CASTLE_SAMPLE_CHECK(listed.size() == 2U);
    CASTLE_SAMPLE_CHECK(*listed.get(1U) == 10U);
    return 0;
}
