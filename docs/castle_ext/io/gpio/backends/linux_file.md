# GPIO Linux Value-File Backend

## Overview
Adapts a BSP-exposed GPIO value file (for example a sysfs-style `.../value` node) to the CASTLE pin backend contract. Each logical/raw read or write opens and closes the path for that operation; electrical direction, pull and drive settings remain BSP-owned.

## Header
`#include "castle_ext/io/gpio/backends/linux_file.hpp"`

## Dependencies
- `castle/error/status.hpp`
- [`../types.md`](../types.md)
- POSIX `<fcntl.h>` and `<unistd.h>` when `CASTLE_EXT_ENABLE_GPIO_LINUX_FILE` is defined.

## Public API
| API | Description |
|---|---|
| `pin_handle` | Borrowed value-file path and active-low flag. |
| `backend::configure(handle, config)` | Returns `not_configured`; electrical configuration is external to the value file. |
| `backend::write/read` | Writes or reads one logical value, applying active-low conversion. |
| `backend::write_raw/read_raw` | Writes or reads physical level without active-low conversion. |
| `backend::toggle(handle)` | Reads the logical value and writes its opposite. |

The `backend` definition is available only when `CASTLE_EXT_ENABLE_GPIO_LINUX_FILE` is defined before including the header. Otherwise it is forward-declared so host builds can include higher-level headers without POSIX file access.

## Usage Example
```cpp
#define CASTLE_EXT_ENABLE_GPIO_LINUX_FILE
#include "castle_ext/io/gpio/backends/linux_file.hpp"
#include <assert.h>

int main()
{
    const castle::io::gpio::linux_file::pin_handle pin("/sys/class/gpio/gpio17/value", false);
    const castle::status result = castle::io::gpio::linux_file::backend::write(
        pin, castle::io::gpio::level::high);
    // On a BSP that exposes this value node, result should be status::ok.
    assert(result == castle::status::ok || result == castle::status::system_call_error);
}
```
See `samples/castle_ext/io/gpio/backends/linux_file.cpp` for a host-safe standalone sample.

## Constraints & Notes
- The path is borrowed and must remain valid for each backend operation.
- Reads/writes open and close the value file each time; this is simple but has more syscall overhead than a persistent descriptor.
- `configure()` intentionally does not claim to set direction, pull or drive electrical parameters.
- This adapter has no built-in locking; callers must serialize concurrent access when required.
