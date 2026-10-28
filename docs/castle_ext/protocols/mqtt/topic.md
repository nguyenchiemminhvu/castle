# MQTT Topic Utilities

## Overview
Validates MQTT Topic Names and Topic Filters and evaluates hierarchical filter matching, including `+`, terminal `#`, empty levels, and the special `$` system-topic rule.

## Header
`#include "castle_ext/protocols/mqtt/topic.hpp"`

## Dependencies
- [`castle/core/compiler.hpp`](../../../../include/castle/core/compiler.hpp)
- [`castle/core/types.hpp`](../../../../include/castle/core/types.hpp)
- [`castle/container/string_view.hpp`](../../../../include/castle/container/string_view.hpp)
- [`castle/error/status.hpp`](../../../../include/castle/error/status.hpp)

## Public API
| Function | Description | Complexity |
|---|---|---|
| `validate_topic_name(string_view)` | Requires a non-empty valid UTF-8 topic name and forbids `+` and `#`. | O(topic length) |
| `validate_topic_filter(string_view)` | Requires `+` to occupy a complete level and `#` to occupy the final complete level. | O(filter length) |
| `topic_matches(filter, topic)` | Tests a Topic Name against a valid Topic Filter. Wildcard filters not beginning with `$` do not match `$` topics. | O(filter length + topic length) |

## Usage Example
```cpp
#include "castle_ext/protocols/mqtt/topic.hpp"

int main()
{
    using namespace castle::protocols::mqtt;
    if (validate_topic_filter("home/+/status") != castle::status::ok) { return 1; }
    return topic_matches("home/+/status", "home/kitchen/status") ? 0 : 2;
}
```

## Constraints & Notes
The functions are allocation-free and operate on non-owning views. Empty topic levels are allowed by MQTT; therefore `a/+` matches `a/`. `#` must be the final complete level. Input UTF-8 is checked for malformed, overlong, surrogate, out-of-range, and Unicode noncharacter sequences. Callers should enforce their own maximum topic/filter byte capacity before copying values into fixed buffers.
