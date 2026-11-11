#include "sample_support.hpp"

#include "castle/cache/lfu.hpp"

// Glyph cache for a small display: frequently drawn characters stay rendered, rare ones rotate.
// Equal popularity falls back to least-recently-used order, so eviction stays deterministic.
int main()
{
    castle::cache::lfu<char, uint16_t, 3U> glyphs; // character -> bitmap handle

    glyphs.put('0', 100U);
    glyphs.put('1', 101U);
    glyphs.put('A', 200U);

    // Digits are redrawn every second.
    for (int frame = 0; frame < 5; ++frame)
    {
        CASTLE_SAMPLE_CHECK(glyphs.get('0') != nullptr);
        CASTLE_SAMPLE_CHECK(glyphs.get('1') != nullptr);
    }

    char dropped = '?';
    auto report = [&dropped](const char& ch, uint16_t&) { dropped = ch; };

    glyphs.put('B', 201U, report);
    CASTLE_SAMPLE_CHECK(dropped == 'A'); // least frequently used

    glyphs.put('C', 202U, report);
    CASTLE_SAMPLE_CHECK(dropped == 'B'); // newcomer had the lowest count

    // Narrow counters save RAM when saturation at 255 is acceptable.
    castle::cache::basic_cache<char, uint16_t, 3U, castle::cache::lfu_policy<uint8_t>> compact;
    compact.put('x', 1U);
    CASTLE_SAMPLE_CHECK(*compact.get('x') == 1U);

    return 0;
}
