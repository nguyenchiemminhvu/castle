#include <gtest/gtest.h>

#include "castle/container/spsc_buffer_desc.hpp"

namespace
{
using dma_buffers_t = castle::container::spsc_buffer_desc<uint8_t, 8U, 3U>;

TEST(DmaBufferDescTest, MemberTypesAndConstants)
{
    static_assert(castle::meta::is_same<dma_buffers_t::value_type, uint8_t>::value,
                  "value_type must match TBuffer");
    static_assert(dma_buffers_t::static_capacity == 3U,
                  "static_capacity must match N_BUFFERS");
    static_assert(dma_buffers_t::static_buffer_size == 8U,
                  "static_buffer_size must match BUFFER_SIZE");

    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);
    EXPECT_TRUE(buffers.is_valid());
    EXPECT_EQ(buffers.capacity(), 3U);
    EXPECT_EQ(buffers.buffer_capacity(), 8U);
}

TEST(DmaBufferDescTest, NullStorageIsInvalid)
{
    dma_buffers_t buffers(nullptr);
    EXPECT_FALSE(buffers.is_valid());
    EXPECT_FALSE(buffers.writable());
    EXPECT_FALSE(buffers.readable());

    dma_buffers_t::buffer_handle buffer_handle;
    EXPECT_EQ(buffers.acquire_write(buffer_handle), castle::status::invalid_config);
    EXPECT_FALSE(buffer_handle.is_valid());
    EXPECT_EQ(buffers.acquire_read(buffer_handle), castle::status::invalid_config);
    EXPECT_FALSE(buffer_handle.is_valid());
}

TEST(DmaBufferDescTest, AcquireWriteCommitReadRelease)
{
    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);

    dma_buffers_t::buffer_handle writer;
    ASSERT_EQ(buffers.acquire_write(writer), castle::status::ok);
    ASSERT_TRUE(writer.is_valid());
    EXPECT_TRUE(writer.is_write());
    EXPECT_EQ(writer.size(), 0U);
    EXPECT_EQ(writer.capacity(), 8U);
    EXPECT_EQ(writer.data(), &storage[0][0]);
    EXPECT_EQ(writer.write_view().size(), 8U);

    writer.write_view()[0] = 0x11U;
    writer.write_view()[1] = 0x22U;
    ASSERT_EQ(writer.commit(2U), castle::status::ok);
    EXPECT_FALSE(writer.is_valid());
    EXPECT_TRUE(buffers.readable());

    dma_buffers_t::buffer_handle reader;
    ASSERT_EQ(buffers.acquire_read(reader), castle::status::ok);
    ASSERT_TRUE(reader.is_valid());
    EXPECT_TRUE(reader.is_read());
    EXPECT_EQ(reader.size(), 2U);
    EXPECT_EQ(reader.data(), &storage[0][0]);

    const auto view = reader.read_view();
    ASSERT_EQ(view.size(), 2U);
    EXPECT_EQ(view[0], 0x11U);
    EXPECT_EQ(view[1], 0x22U);

    ASSERT_EQ(reader.release(), castle::status::ok);
    EXPECT_FALSE(reader.is_valid());
    EXPECT_TRUE(buffers.writable());
    EXPECT_TRUE(buffers.empty());
}

TEST(DmaBufferDescTest, BuffersAreContiguousAndAdvanceInFifoOrder)
{
    uint16_t storage[3U][4U] = {};
    using buffers_t = castle::container::spsc_buffer_desc<uint16_t, 4U, 3U>;
    buffers_t buffers(&storage[0][0]);

    auto a = buffers.acquire_write();
    auto b = buffers.acquire_write();
    auto c = buffers.acquire_write();

    ASSERT_TRUE(a.is_valid());
    ASSERT_TRUE(b.is_valid());
    ASSERT_TRUE(c.is_valid());
    EXPECT_EQ(a.data(), &storage[0][0]);
    EXPECT_EQ(b.data(), &storage[1][0]);
    EXPECT_EQ(c.data(), &storage[2][0]);

    EXPECT_EQ(buffers.acquire_write().is_valid(), false);

    ASSERT_EQ(a.commit(1U), castle::status::ok);
    ASSERT_EQ(b.commit(1U), castle::status::ok);
    ASSERT_EQ(c.commit(1U), castle::status::ok);

    auto r0 = buffers.acquire_read();
    auto r1 = buffers.acquire_read();
    auto r2 = buffers.acquire_read();
    auto r3 = buffers.acquire_read();

    EXPECT_EQ(r0.data(), &storage[0][0]);
    EXPECT_EQ(r1.data(), &storage[1][0]);
    EXPECT_EQ(r2.data(), &storage[2][0]);
    EXPECT_FALSE(r3.is_valid());

    EXPECT_EQ(r0.release(), castle::status::ok);
    EXPECT_EQ(r1.release(), castle::status::ok);
    EXPECT_EQ(r2.release(), castle::status::ok);
}

TEST(DmaBufferDescTest, InvalidCommitDoesNotLoseWriteReservation)
{
    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);
    auto writer = buffers.acquire_write();
    ASSERT_TRUE(writer.is_valid());

    EXPECT_EQ(writer.commit(9U), castle::status::out_of_range);
    EXPECT_TRUE(writer.is_valid());
    EXPECT_TRUE(writer.is_write());
    EXPECT_TRUE(buffers.empty());

    EXPECT_EQ(writer.commit(8U), castle::status::ok);
    EXPECT_FALSE(writer.is_valid());
}

TEST(DmaBufferDescTest, CommitOutOfOrderPreservesFifoReadOrder)
{
    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);

    auto first = buffers.acquire_write();
    auto second = buffers.acquire_write();
    ASSERT_TRUE(first.is_valid());
    ASSERT_TRUE(second.is_valid());

    second.write_view()[0] = 0xB2U;
    first.write_view()[0] = 0xA1U;

    ASSERT_EQ(second.commit(1U), castle::status::ok);
    EXPECT_TRUE(buffers.empty());

    ASSERT_EQ(first.commit(1U), castle::status::ok);

    auto r0 = buffers.acquire_read();
    ASSERT_TRUE(r0.is_valid());
    EXPECT_EQ(r0.data(), &storage[0][0]);
    EXPECT_EQ(r0.read_view()[0], 0xA1U);
    ASSERT_EQ(r0.release(), castle::status::ok);

    auto r1 = buffers.acquire_read();
    ASSERT_TRUE(r1.is_valid());
    EXPECT_EQ(r1.data(), &storage[1][0]);
    EXPECT_EQ(r1.read_view()[0], 0xB2U);
    ASSERT_EQ(r1.release(), castle::status::ok);
}

TEST(DmaBufferDescTest, StaleDescriptorCannotControlReusedBuffer)
{
    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);

    auto old_writer = buffers.acquire_write();
    ASSERT_TRUE(old_writer.is_valid());
    ASSERT_EQ(old_writer.commit(1U), castle::status::ok);

    auto reader = buffers.acquire_read();
    ASSERT_TRUE(reader.is_valid());
    ASSERT_EQ(reader.release(), castle::status::ok);

    auto new_writer = buffers.acquire_write();
    ASSERT_TRUE(new_writer.is_valid());
    EXPECT_FALSE(old_writer.is_valid());
    EXPECT_EQ(old_writer.cancel(), castle::status::invalid_argument);
    EXPECT_TRUE(new_writer.is_valid());
    EXPECT_EQ(new_writer.cancel(), castle::status::ok);
}

TEST(DmaBufferDescTest, ReleaseAndCancelAreStateSpecific)
{
    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);

    auto writer = buffers.acquire_write();
    ASSERT_TRUE(writer.is_valid());
    EXPECT_EQ(writer.release(), castle::status::invalid_argument);
    EXPECT_EQ(writer.cancel(), castle::status::ok);
    EXPECT_FALSE(writer.is_valid());

    buffers.clear();
    auto reader_source = buffers.acquire_write();
    ASSERT_TRUE(reader_source.is_valid());
    ASSERT_EQ(reader_source.commit(1U), castle::status::ok);
    auto reader = buffers.acquire_read();
    ASSERT_TRUE(reader.is_valid());
    EXPECT_EQ(reader.commit(1U), castle::status::invalid_argument);
    EXPECT_EQ(reader.cancel(), castle::status::invalid_argument);
    EXPECT_EQ(reader.release(), castle::status::ok);
}

TEST(DmaBufferDescTest, ClearReleasesAndInvalidatesDescriptors)
{
    uint8_t storage[3U][8U] = {};
    dma_buffers_t buffers(&storage[0][0]);

    auto first = buffers.acquire_write();
    auto second = buffers.acquire_write();
    ASSERT_TRUE(first.is_valid());
    ASSERT_TRUE(second.is_valid());

    buffers.clear();

    EXPECT_FALSE(first.is_valid());
    EXPECT_FALSE(second.is_valid());
    EXPECT_TRUE(buffers.writable());
    EXPECT_TRUE(buffers.empty());

    auto next = buffers.acquire_write();
    ASSERT_TRUE(next.is_valid());
    EXPECT_EQ(next.data(), &storage[0][0]);
    EXPECT_EQ(next.cancel(), castle::status::ok);
}

TEST(DmaBufferDescTest, DefaultAtomicStateSupportsSpscHandshake)
{
    uint8_t storage[2U][4U] = {};
    using buffers_t = castle::container::spsc_buffer_desc<uint8_t, 4U, 2U, castle::atomic<uint8_t>>;
    buffers_t buffers(&storage[0][0]);

    auto writer = buffers.acquire_write();
    ASSERT_TRUE(writer.is_valid());
    writer.write_view()[0] = 0x5AU;
    ASSERT_EQ(writer.commit(1U), castle::status::ok);

    auto reader = buffers.acquire_read();
    ASSERT_TRUE(reader.is_valid());
    EXPECT_EQ(reader.read_view()[0], 0x5AU);
    EXPECT_EQ(reader.release(), castle::status::ok);
}

TEST(DmaBufferDescTest, PlainStateCanBeUsedWhenSynchronizationIsExternal)
{
    uint8_t storage[2U][4U] = {};
    using buffers_t = castle::container::spsc_buffer_desc<uint8_t, 4U, 2U, uint8_t>;
    buffers_t buffers(&storage[0][0]);

    auto writer = buffers.acquire_write();
    ASSERT_TRUE(writer.is_valid());
    writer.write_view()[0] = 0x33U;
    ASSERT_EQ(writer.commit(1U), castle::status::ok);

    auto reader = buffers.acquire_read();
    ASSERT_TRUE(reader.is_valid());
    EXPECT_EQ(reader.read_view()[0], 0x33U);
    EXPECT_EQ(reader.release(), castle::status::ok);
}

} // namespace
