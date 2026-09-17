#include "sample_support.hpp"

#include "castle/container/spsc_buffer_desc.hpp"

#include <stdint.h>

// Scenario: a UART RX DMA producer fills fixed buffers while a foreground task
// consumes the committed buffers without copying the payload.
namespace
{
using dma_buffers_t = castle::container::spsc_buffer_desc<uint8_t, 16U, 3U>;

void simulate_uart_dma_receive(dma_buffers_t::buffer_handle& dma_write,
                               const uint8_t* data,
                               castle::size_type count)
{
    auto view = dma_write.write_view();
    const castle::size_type copy_count = count < view.size() ? count : view.size();

    for (castle::size_type i = 0U; i < copy_count; ++i)
    {
        view[i] = data[i];
    }

    // In a real driver this is usually the DMA-complete ISR reporting the
    // number of bytes transferred by the hardware.
    (void)dma_write.commit(copy_count);
}
} // namespace

int main()
{
    uint8_t storage[3U][16U] = {};
    dma_buffers_t buffers(&storage[0][0]);

    const uint8_t message[] = {'O', 'K', '\r', '\n'};

    dma_buffers_t::buffer_handle dma_write; 
    CASTLE_SAMPLE_CHECK(
        buffers.acquire_write(dma_write) == castle::status::ok
    );
    CASTLE_SAMPLE_CHECK(dma_write.capacity() == 16U);

    simulate_uart_dma_receive(dma_write, message, sizeof(message));
    CASTLE_SAMPLE_CHECK(!dma_write.is_valid());
    CASTLE_SAMPLE_CHECK(buffers.readable());

    // Foreground task: take the ready buffer and process only the transferred
    // bytes. No memcpy is needed between DMA storage and the parser.
    dma_buffers_t::buffer_handle reader;
    CASTLE_SAMPLE_CHECK(
        buffers.acquire_read(reader) == castle::status::ok
    );

    const auto data = reader.read_view();
    CASTLE_SAMPLE_CHECK(data.size() == sizeof(message));
    CASTLE_SAMPLE_CHECK(data[0] == 'O');
    CASTLE_SAMPLE_CHECK(data[1] == 'K');
    CASTLE_SAMPLE_CHECK(data[2] == '\r');
    CASTLE_SAMPLE_CHECK(data[3] == '\n');

    CASTLE_SAMPLE_CHECK(reader.release() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(buffers.empty());

    // Multiple DMA buffers can be reserved ahead of time for ping/pong or
    // descriptor-ring operation. FIFO order is retained on the read side.
    auto first = buffers.acquire_write();
    auto second = buffers.acquire_write();
    CASTLE_SAMPLE_CHECK(first.is_valid());
    CASTLE_SAMPLE_CHECK(second.is_valid());
    CASTLE_SAMPLE_CHECK(first.commit(1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(second.commit(1U) == castle::status::ok);

    return 0;
}
