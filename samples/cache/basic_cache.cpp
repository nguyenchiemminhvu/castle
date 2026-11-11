#include "sample_support.hpp"

#include "castle/cache/basic_cache.hpp"
#include "castle/cache/lru.hpp"

namespace
{
// Composite key that castle::hash does not know about.
struct node_id
{
    uint8_t bus;
    uint8_t address;
};

struct node_id_hash
{
    castle::size_type operator()(const node_id& id) const noexcept
    {
        return castle::hash<uint16_t>()(static_cast<uint16_t>((id.bus << 8U) | id.address));
    }
};

struct node_id_equal
{
    bool operator()(const node_id& lhs, const node_id& rhs) const noexcept
    {
        return (lhs.bus == rhs.bus) && (lhs.address == rhs.address);
    }
};
} // namespace

// Spell basic_cache directly to pair any policy with a custom key, hash, and equality.
int main()
{
    castle::cache::basic_cache<node_id, int16_t, 2U, castle::cache::lru_policy, node_id_hash, node_id_equal> temps;

    temps.put(node_id{0U, 0x48U}, 215); // 21.5 C
    temps.put(node_id{1U, 0x48U}, 198); // same address, other bus
    CASTLE_SAMPLE_CHECK(temps.size() == 2U);
    CASTLE_SAMPLE_CHECK(*temps.peek(node_id{1U, 0x48U}) == 198);

    temps.put(node_id{0U, 0x48U}, 220); // update refreshes recency
    temps.put(node_id{2U, 0x10U}, 250); // evicts bus 1 sensor
    CASTLE_SAMPLE_CHECK(!temps.contains(node_id{1U, 0x48U}));
    CASTLE_SAMPLE_CHECK(*temps.get(node_id{0U, 0x48U}) == 220);

    CASTLE_SAMPLE_CHECK(temps.erase(node_id{2U, 0x10U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(temps.erase(node_id{2U, 0x10U}) == castle::status::not_found);
    temps.clear();
    CASTLE_SAMPLE_CHECK(temps.empty());
    CASTLE_SAMPLE_CHECK(temps.evict() == castle::status::empty);

    return 0;
}
