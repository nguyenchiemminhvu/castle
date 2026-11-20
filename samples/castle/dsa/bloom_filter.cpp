#include "sample_support.hpp"

#include "castle/dsa/bloom_filter.hpp"
#include "castle/dsa/string_view.hpp"

int main()
{
    using castle::dsa::string_view;

    // 1) Skip a slow lookup: "is this user name taken?" without hitting the database.
    //    A "no" answer is always right, so only "maybe" answers need the real query.
    castle::dsa::bloom_filter_for<string_view, 16U> taken_names; // 160 bits, 7 probes
    taken_names.insert(string_view("alice"));
    taken_names.insert(string_view("bob"));
    taken_names.insert(string_view("carol"));

    CASTLE_SAMPLE_CHECK(taken_names.contains(string_view("bob")));    // maybe taken: ask the database
    CASTLE_SAMPLE_CHECK(!taken_names.contains(string_view("dave")));  // certainly free: no query needed

    // 2) Drop duplicate messages: insert() returns true only for a key that is certainly new.
    castle::dsa::bloom_filter<uint32_t, 256U, 4U> seen_ids;
    const uint32_t incoming[] = {101U, 102U, 101U, 103U, 102U};
    castle::size_type processed = 0U;
    for (const uint32_t id : incoming)
    {
        if (seen_ids.insert(id))
        {
            ++processed; // first time this id arrives
        }
    }
    CASTLE_SAMPLE_CHECK(processed == 3U);

    // 3) Block known-bad URLs on a device with little RAM.
    castle::dsa::bloom_filter_for<string_view, 8U> blocklist{
        string_view("malware.example"),
        string_view("phishing.example"),
    };
    CASTLE_SAMPLE_CHECK(blocklist.contains(string_view("phishing.example")));
    CASTLE_SAMPLE_CHECK(!blocklist.contains(string_view("castle.example")));

    // Keys cannot be erased one by one; clear() starts over.
    blocklist.clear();
    CASTLE_SAMPLE_CHECK(blocklist.empty());

    return 0;
}
