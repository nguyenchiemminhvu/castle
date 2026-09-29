# Linux File

## Overview
`basic_file` is a move-only, allocation-free owner for ordinary POSIX files and BSP-exposed device nodes. It provides bounded byte I/O plus seek, size, truncate and sync operations, while sharing descriptor lifetime and transfer behavior with CASTLE's FD module.

## Header
`#include "castle_ext/io/file/linux_file.hpp"`

## Dependencies
- `castle/core/config.hpp`
- `castle/core/compiler.hpp`
- `castle/core/types.hpp`
- `castle/error/status.hpp`
- [`../fd/descriptor.md`](../fd/descriptor.md)

## Public API
| API | Description |
|---|---|
| `seek_origin` | Selects beginning, current position or end as the seek origin. |
| `file_config` | Fixed-size path/access/create/truncate/append/non-blocking configuration. |
| `validate_config(config)` | Checks path and access-flag consistency without a system call. |
| `basic_file` / `linux_file` | Move-only owner exposing `open`, `close`, `native_handle`, `configuration`, `is_open`. |
| `read_some`, `read_exact` | Reads a partial or exact number of bytes, reporting actual bytes. |
| `write_some`, `write_all` | Writes a partial or complete buffer, reporting actual bytes. |
| `seek`, `tell`, `size` | Changes or queries the file position and current file size. |
| `truncate`, `sync` | Changes file length or calls `fsync()`. |

## Usage Example
```cpp
#include "castle_ext/io/file/linux_file.hpp"
#include <assert.h>

int main()
{
    castle::io::file::basic_file file;
    const castle::io::file::file_config cfg(
        "/tmp/castle_capture.bin", true, true, true, true);
    assert(file.open(cfg) == castle::status::ok);

    const char bytes[] = {'U', 'B', 'X'};
    castle::size_type written = 0U;
    assert(file.write_all(bytes, sizeof(bytes), written) == castle::status::ok);
    assert(written == sizeof(bytes));
    assert(file.sync() == castle::status::ok);
    assert(file.close() == castle::status::ok);
}
```
See `samples/castle_ext/io/file/linux_file.cpp` for a standalone sample.

## Constraints & Notes
- The path is a borrowed null-terminated string and must outlive any use that reads `configuration()`; the kernel resolves it during `open()`.
- No heap allocation, exceptions, RTTI, virtual functions, STL types or internal locking.
- `read_some` and `write_some` are partial-transfer APIs. Prefer `read_exact`/`write_all` only where blocking behavior is acceptable.
- `append`, `truncate` and `exclusive` require write access; `exclusive` also requires create.
- `seek`, `fstat`, `ftruncate` and `fsync` are POSIX operations and return `not_configured` when POSIX support is disabled.
- File offsets are shared kernel state. Synchronize externally when multiple execution contexts use one file object or descriptor.
