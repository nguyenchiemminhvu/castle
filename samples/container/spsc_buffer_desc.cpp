#include "sample_support.hpp"

#include "castle/container/spsc_buffer_desc.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

int main()
{
    using desc_t = castle::container::spsc_buffer_desc<uint8_t, 4U, 2U>;

    desc_t invalid(nullptr);
    desc_t::buffer_handle invalid_handle;
    CASTLE_SAMPLE_CHECK(!invalid.is_valid());
    CASTLE_SAMPLE_CHECK(invalid.acquire_write(invalid_handle) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(invalid.acquire_read(invalid_handle) == castle::status::invalid_config);

    uint8_t storage[2U][4U] = {};
    desc_t desc(&storage[0U][0U]);
    CASTLE_SAMPLE_CHECK(desc.is_valid());
    CASTLE_SAMPLE_CHECK(desc.capacity() == 2U);
    CASTLE_SAMPLE_CHECK(desc.buffer_capacity() == 4U);
    CASTLE_SAMPLE_CHECK(desc.writable());
    CASTLE_SAMPLE_CHECK(desc.empty());
    CASTLE_SAMPLE_CHECK(!desc.full());

    desc_t::buffer_handle writer;
    CASTLE_SAMPLE_CHECK(desc.acquire_write(writer) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(writer.is_valid());
    CASTLE_SAMPLE_CHECK(writer.is_write());
    CASTLE_SAMPLE_CHECK(!writer.is_read());
    CASTLE_SAMPLE_CHECK(static_cast<bool>(writer));
    CASTLE_SAMPLE_CHECK(writer.capacity() == 4U);
    CASTLE_SAMPLE_CHECK(writer.max_size() == 4U);
    CASTLE_SAMPLE_CHECK(writer.size() == 0U);
    CASTLE_SAMPLE_CHECK(writer.data() == &storage[0U][0U]);
    CASTLE_SAMPLE_CHECK(writer.read_view().empty());

    auto writer_copy = writer;
    auto writer_view = writer.write_view();
    CASTLE_SAMPLE_CHECK(writer_view.size() == 4U);
    writer_view[0U] = 0x10U;
    writer_view[1U] = 0x11U;
    writer_view[2U] = 0x12U;
    CASTLE_SAMPLE_CHECK(writer.commit(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!writer.is_valid());
    CASTLE_SAMPLE_CHECK(!writer_copy.is_valid());
    CASTLE_SAMPLE_CHECK(desc.readable());

    desc_t::buffer_handle reader;
    CASTLE_SAMPLE_CHECK(desc.acquire_read(reader) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(reader.is_valid());
    CASTLE_SAMPLE_CHECK(reader.is_read());
    CASTLE_SAMPLE_CHECK(!reader.is_write());
    CASTLE_SAMPLE_CHECK(reader.size() == 3U);
    CASTLE_SAMPLE_CHECK(reader.data() == &storage[0U][0U]);

    auto readable = reader.read_view();
    CASTLE_SAMPLE_CHECK(readable.size() == 3U);
    CASTLE_SAMPLE_CHECK(readable[0U] == 0x10U);
    CASTLE_SAMPLE_CHECK(readable[2U] == 0x12U);
    CASTLE_SAMPLE_CHECK(reader.release() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(reader.release() == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(desc.empty());

    auto second_writer = desc.acquire_write();
    CASTLE_SAMPLE_CHECK(second_writer.is_valid());
    auto moved_writer = castle::move(second_writer);
    CASTLE_SAMPLE_CHECK(!second_writer.is_valid());
    CASTLE_SAMPLE_CHECK(moved_writer.is_write());
    CASTLE_SAMPLE_CHECK(moved_writer.commit(5U) == castle::status::out_of_range);
    auto second_view = moved_writer.write_view();
    second_view[0U] = 0x20U;
    CASTLE_SAMPLE_CHECK(moved_writer.commit(1U) == castle::status::ok);

    auto third_writer = desc.acquire_write();
    CASTLE_SAMPLE_CHECK(third_writer.is_valid());
    CASTLE_SAMPLE_CHECK(desc.full());
    CASTLE_SAMPLE_CHECK(third_writer.cancel() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(desc.acquire_write(invalid_handle) == castle::status::full);

    auto reader2 = desc.acquire_read();
    CASTLE_SAMPLE_CHECK(reader2.is_valid());
    CASTLE_SAMPLE_CHECK(reader2.read_view().size() == 1U);
    CASTLE_SAMPLE_CHECK(reader2.read_view()[0U] == 0x20U);
    CASTLE_SAMPLE_CHECK(reader2.release() == castle::status::ok);

    auto recycled = desc.acquire_write();
    CASTLE_SAMPLE_CHECK(recycled.is_valid());
    recycled.write_view()[0U] = 0x33U;
    CASTLE_SAMPLE_CHECK(recycled.commit(1U) == castle::status::ok);

    desc.clear();
    CASTLE_SAMPLE_CHECK(desc.empty());
    CASTLE_SAMPLE_CHECK(desc.writable());
    CASTLE_SAMPLE_CHECK(!recycled.is_valid());
    return 0;
}
