# `castle/callbacks/inplace_function.h` — Type-erased callable, no heap

**Header:** `castle/callbacks/inplace_function.h`
**Namespace:** `castle::callbacks`
**Sample:** [`samples/sample_inplace_function.cpp`](../../samples/sample_inplace_function.cpp)

## Purpose

`inplace_function<Sig, StorageSize, StorageAlignment>` is a
`std::function`-like type-erased callable whose storage is a
**compile-time-sized inline buffer** — **no heap allocation** is ever
performed, and no exception can leave the constructor.

Use it when you need `std::function`-style polymorphism (hold *any*
callable with the given signature) but must remain heap-free — e.g.
when queued in `inplace_callback_registry`, `inplace_event_dispatcher`,
`inplace_tick_timer`, or `inplace_signal_event`.

## Design notes

- Storage is `alignas(StorageAlignment) char storage_[StorageSize]`,
  with `StorageSize = 64`, `StorageAlignment = alignof(std::max_align_t)`
  by default.
- Two `static_assert`s in the constructor reject callables that don't
  fit:
  - `sizeof(Callable) <= StorageSize`
  - `alignof(Callable) <= StorageAlignment`
- Erasure is done through four function pointers (invoke / destroy /
  copy / move) — a tiny custom vtable stored in the object itself.
- Copy and move are supported (unlike `std::function`, no heap
  operations are involved).
- Uses placement `new` on the raw buffer; the destructor calls
  `~Callable` through the destroy trampoline.

## API

| Member                                 | Notes                               |
| -------------------------------------- | ----------------------------------- |
| `inplace_function()`                   | Empty                               |
| `inplace_function(Callable&&)`         | SBO-storable callable only          |
| Copy / move / assignment               | All in-buffer                       |
| `operator()(Args...)`                  | Dispatches through invoke pointer   |
| Explicit `bool` (typically)            | Truthy when non-empty               |

## Example

```cpp
#include <castle/callbacks/inplace_function.h>
using namespace castle::callbacks;

int captured = 10;
inplace_function<void(int)> cb = [captured](int v) {
    std::printf("%d\n", v + captured);
};
cb(5);   // 15

// Enforce fitness at compile time by shrinking storage:
using tiny_cb = inplace_function<void(int), /*StorageSize=*/32>;
```

## Notes / pitfalls

- Increase `StorageSize` if you get a `static_assert` about "too
  large" — but remember that every instance carries that many bytes
  even when empty.
- Not thread-safe on its own.

## See also

- `castle/callbacks/function.h` — non-owning callable interface.
- `castle/buffers/soo_buffer.h` — same SBO idea applied to arbitrary
  types, with an optional heap fallback.
