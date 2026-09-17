# `castle::container::spsc_buffer_desc`

`castle::container::spsc_buffer_desc` is a fixed-capacity, non-owning descriptor ring for DMA-oriented Single producer/Single consumer paths.

## Design goals

The implementation is intended for castle's embedded constraints:

- no heap allocation;
- no exceptions;
- no RTTI;
- no virtual functions;
- no STL dependency;
- fixed compile-time buffer count and buffer size;
- externally supplied storage, so the application can place buffers in a DMA-capable SRAM/section;
- deterministic O(1) acquisition and release operations;
- zero-copy hand-off through `castle::container::array_view`;
- explicit ownership transitions.

## Template

```cpp
namespace castle
{
template <typename TBuffer,
          size_type BUFFER_SIZE,
          size_type N_BUFFERS,
          typename TState = castle::atomic<uint8_t>>
class spsc_buffer_desc;
}
```

`TBuffer` is the element type in each buffer. `BUFFER_SIZE` is the number of elements in each buffer. `N_BUFFERS` is the number of slots. `TState` stores the per-slot lifecycle state and defaults to `castle::atomic<uint8_t>`.

The default atomic state makes the normal single-producer/single-consumer DMA hand-off suitable for an ISR/task or DMA/task arrangement. A plain `uint8_t` state can be supplied when synchronization is guaranteed externally.

## Ownership model

A slot follows this lifecycle:

```text
        acquire_write()
              |
              v
            writing
              |
              v
           commit(n)
              |
              v
             ready
              |
              v
         acquire_read()
              |
              v
            reading
              |
              v
            release()
              |
              v
             free
```

`cancel()` is the exceptional write-side path:

```text
writing --cancel()--> free
```

The buffer_handle handed to the writer becomes invalid after a successful `commit()`. The buffer_handle handed to the reader becomes invalid after a successful `release()`. This makes ownership transfer explicit and avoids treating an old handle as an owner after the slot changes state.

A 32-bit per-slot generation counter prevents an old copied buffer_handle from controlling a slot after it has been released and reused. Generation `0` is skipped.

## Storage ownership

The class does **not** allocate or own the payload memory.

The application supplies contiguous storage:

```cpp
uint8_t rx_storage[4U][256U];

castle::container::spsc_buffer_desc<uint8_t, 256U, 4U> rx(
    &rx_storage[0U][0U]
);
```

The descriptor ring stores pointers to:

```text
rx_storage[0] -> 256 bytes
rx_storage[1] -> 256 bytes
rx_storage[2] -> 256 bytes
rx_storage[3] -> 256 bytes
```

The storage must remain valid for the entire lifetime of the descriptor object and must be physically/architecturally suitable for the DMA controller. Cache maintenance, alignment, MPU attributes, and DMA-accessible memory placement remain platform responsibilities.

## Write side

Acquire the next free buffer:

```cpp
castle::container::spsc_buffer_desc<uint8_t, 256U, 4U>::buffer_handle dma_write;

if (rx.acquire_write(dma_write) == castle::status::ok)
{
    uint8_t* address = dma_write.data();
    const auto writable = dma_write.write_view();

    // Configure DMA with address and writable.size().
    // DMA writes into writable.data().
}
```

The convenience form is also available:

```cpp
auto dma_write = rx.acquire_write();
if (dma_write.is_valid())
{
    start_dma(dma_write.data(), dma_write.capacity());
}
```

Once the hardware reports the transferred length, publish that amount:

```cpp
(void)dma_write.commit(transferred_bytes);
```

A successful commit changes the slot from `writing` to `ready`.

The descriptor ring advances its write cursor when a buffer is acquired, so multiple buffers can be reserved ahead of time for ping/pong or descriptor-ring DMA operation.

## Read side

The reader acquires only committed buffers and receives the exact valid element count:

```cpp
castle::container::spsc_buffer_desc<uint8_t, 256U, 4U>::buffer_handle reader;

if (rx.acquire_read(reader) == castle::status::ok)
{
    const auto packet = reader.read_view();
    parse(packet.data(), packet.size());
    (void)reader.release();
}
```

The convenience form is also available:

```cpp
auto dma_read = rx.acquire_read();
```

`read_view()` is read-only and contains only the number of elements published by `commit()`.

This means a 256-byte buffer can contain a 37-byte UART packet without copying it into another application buffer.

## FIFO behavior

Writes are acquired in ring order and reads are acquired in the same ring order.

It is valid to reserve multiple write buffers before committing them:

```cpp
auto a = buffers.acquire_write();
auto b = buffers.acquire_write();

fill_a(a);
fill_b(b);

(void)b.commit(size_b);
(void)a.commit(size_a);
```

The read side still waits for `a` first. Until `a` is committed, `acquire_read()` returns `castle::status::empty`. This keeps the data path ordered even if a later reserved DMA operation completes earlier.

## Status values

The class uses `castle::status` instead of exceptions:

| Operation | Condition | Result |
|---|---|---|
| `acquire_write` | storage pointer is null | `status::invalid_config` |
| `acquire_write` | next slot is not free | `status::full` |
| `acquire_read` | storage pointer is null | `status::invalid_config` |
| `acquire_read` | next slot is not ready | `status::empty` |
| `commit(count)` | buffer_handle is not the active write handle | `status::invalid_argument` |
| `commit(count)` | `count > BUFFER_SIZE` | `status::out_of_range` |
| `release()` | buffer_handle is not the active read handle | `status::invalid_argument` |
| `cancel()` | buffer_handle is not the active write handle | `status::invalid_argument` |

## SPSC concurrency contract

The default `TState = castle::atomic<uint8_t>` is designed around a single producer and a single consumer.

Typical ownership looks like this:

```text
                producer side
             DMA / ISR / task
                    |
               acquire_write
                    |
                 writing
                    |
                commit(n)
                    |
                    |  atomic state hand-off
                    v
                  ready
                    |
               acquire_read
                    |
                 reading
                    |
                release()
                    |
                    v
                producer can
                reuse the slot
```

The class is not an MPMC queue. If multiple independent producers or consumers use the same object, place an external lock or other synchronization around the acquisition/commit/release protocol.

`clear()` is a quiescent operation and must not race with DMA, interrupt handlers, writers, or readers.

## DMA cache and memory-order considerations

`castle::atomic<uint8_t>` provides the CPU-side state synchronization. Every hand-off is a release store paired with an acquire load on the same per-slot state: `commit()`/`release()`/`cancel()` release-publish the buffer_handle's buffer contents and size, and `acquire_write()`/`acquire_read()` (and any `is_valid()` check) acquire-load the state before touching that data. This is the same asymmetric acquire/release discipline used by `mirrored_ring_buffer`, and is cheaper than the sequentially-consistent default that a plain `state = value` / `(T)state` conversion would use. A non-atomic `TState` (e.g. plain `uint8_t`) falls back to an ordinary load/store, since it is only valid when the caller already guarantees ordering externally.

This does **not** solve DMA cache coherency.

On a cached MCU, the application may still need to:

- put the buffers in DMA-visible/non-cacheable memory;
- clean cache lines before DMA reads memory produced by the CPU;
- invalidate cache lines before the CPU reads memory produced by DMA;
- obey the target's required DMA/CPU barriers.

Those actions are deliberately outside this generic container.

## API reference

### Manager

```cpp
bool is_valid() const;
static constexpr size_type capacity();
static constexpr size_type buffer_capacity();

bool writable() const;
bool readable() const;
bool full() const;
bool empty() const;

status acquire_write(buffer_handle& out);
buffer_handle acquire_write();

status acquire_read(buffer_handle& out);
buffer_handle acquire_read();

void clear();
```

`capacity()` returns the number of buffer_handle slots. `buffer_capacity()` returns the number of `TBuffer` elements in one slot.

### Descriptor

```cpp
bool is_valid() const;
bool is_write() const;
bool is_read() const;

size_type capacity() const;
size_type max_size() const;
size_type size() const;

TBuffer* data();
const TBuffer* data() const;

castle::container::array_view<TBuffer> write_view();
castle::container::array_view<const TBuffer> read_view() const;

status commit(size_type count);
status release();
status cancel();
```

`write_view()` exposes the full writable capacity. `read_view()` exposes only the committed data length.

## UART RX example

A typical zero-copy UART DMA pipeline is:

```text
UART peripheral
      |
      v
     DMA
      |
      v
  acquire_write()
      |
      v
 [ fixed buffer ]
      |
 DMA complete ISR
      |
  commit(count)
      |
      v
  acquire_read()
      |
      v
    parser
      |
   release()
      |
      v
 [ buffer free ]
```

The parser consumes the DMA buffer directly. No intermediate heap object and no payload copy are required.

## TX example

The same object can be used in the opposite direction:

```cpp
auto tx_write = tx_buffers.acquire_write();
if (tx_write.is_valid())
{
    auto out = tx_write.write_view();
    // Format the frame directly into the DMA-capable buffer.
    const castle::size_type n = encode_frame(out);
    (void)tx_write.commit(n);
}

auto tx_read = tx_buffers.acquire_read();
if (tx_read.is_valid())
{
    // Give tx_read.data() and tx_read.size() to the DMA transmitter.
    start_tx_dma(tx_read.data(), tx_read.size());

    // Call release() when the DMA transmission has completed.
}
```
