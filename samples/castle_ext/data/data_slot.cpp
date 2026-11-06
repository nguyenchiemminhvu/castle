#include "sample_support.hpp"

#include "castle_ext/data/data_slot.hpp"

#include "castle/utility/move.hpp"

#include <stdint.h>

namespace
{
// A large structure: readers access it in place instead of copying it.
struct lidar_scan
{
    uint32_t sequence = 0U;
    uint16_t range_mm[360]{};
};
} // namespace

int main()
{
    // Up to two readers may hold older scans while the driver keeps publishing.
    castle_ext::data::data_slot<lidar_scan, 2U> slot;

    // Before the first publication readers see a default value with generation 0.
    castle_ext::data::snapshot<lidar_scan> initial = slot.get_snapshot();
    CASTLE_SAMPLE_CHECK(initial);
    CASTLE_SAMPLE_CHECK(initial.generation() == 0U);
    CASTLE_SAMPLE_CHECK(initial->sequence == 0U);
    initial.reset();
    CASTLE_SAMPLE_CHECK(!initial);
    CASTLE_SAMPLE_CHECK(initial.get() == nullptr);

    // Driver thread: publish copies the scan once.
    lidar_scan scan{};
    scan.sequence = 1U;
    scan.range_mm[0] = 1500U;
    CASTLE_SAMPLE_CHECK(slot.publish(scan) == castle::status::ok);

    // Reader thread: pin the latest scan.
    castle_ext::data::snapshot<lidar_scan> held = slot.get_snapshot();
    CASTLE_SAMPLE_CHECK(held.generation() == 1U);
    CASTLE_SAMPLE_CHECK((*held).range_mm[0] == 1500U);

    // Newer scans do not disturb the held one.
    scan.sequence = 2U;
    scan.range_mm[0] = 1400U;
    CASTLE_SAMPLE_CHECK(slot.publish(scan) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(held->sequence == 1U);
    CASTLE_SAMPLE_CHECK(slot.get_snapshot()->sequence == 2U);

    // Snapshots are move-only handles.
    castle_ext::data::snapshot<lidar_scan> moved = CASTLE_MOVE(held);
    CASTLE_SAMPLE_CHECK(!held);
    CASTLE_SAMPLE_CHECK(moved.get()->sequence == 1U);

    // Holding more old snapshots than MaxReaders makes the writer report status::full.
    castle_ext::data::snapshot<lidar_scan> second = slot.get_snapshot();   // sequence 2
    scan.sequence = 3U;
    CASTLE_SAMPLE_CHECK(slot.publish(scan) == castle::status::ok);
    castle_ext::data::snapshot<lidar_scan> third = slot.get_snapshot();    // sequence 3
    scan.sequence = 4U;
    CASTLE_SAMPLE_CHECK(slot.publish(scan) == castle::status::ok);         // 3 old values held
    scan.sequence = 5U;
    CASTLE_SAMPLE_CHECK(slot.publish(scan) == castle::status::full);
    CASTLE_SAMPLE_CHECK(slot.get_snapshot()->sequence == 4U);

    // Releasing a snapshot gives its buffer back to the writer.
    moved.reset();
    CASTLE_SAMPLE_CHECK(slot.publish(scan) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(slot.get_snapshot().generation() == 5U);
    CASTLE_SAMPLE_CHECK(second->sequence == 2U);
    CASTLE_SAMPLE_CHECK(third->sequence == 3U);

    return 0;
}
