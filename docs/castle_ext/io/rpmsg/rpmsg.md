# RPMsg Umbrella Header

## Overview
Convenience include for the common CASTLE RPMsg API. It intentionally excludes optional platform backends.

## Header
`#include "castle_ext/io/rpmsg/rpmsg.hpp"`

## Dependencies
All dependencies of the included RPMsg core headers.

## Public API
Includes types, message views, Name Service helpers, backend contract, endpoint, and device headers.

## Usage Example
```cpp
#include "castle_ext/io/rpmsg/rpmsg.hpp"
#include "castle_ext/io/rpmsg/backends/mock.hpp"

int main()
{
    castle::io::rpmsg::mock::backend transport;
    castle::io::rpmsg::device<castle::io::rpmsg::mock::backend> dev(transport);
    return dev.initialize() == castle::status::ok ? (dev.shutdown() == castle::status::ok ? 0 : 2) : 1;
}
```

## Constraints & Notes
Including this header does not include OpenAMP, libmetal, Linux, or Zephyr headers. Select a backend explicitly. Public core API remains platform-neutral and uses CASTLE fixed-capacity primitives. C++17 is required.
