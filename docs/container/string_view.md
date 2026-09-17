# Non-owning string view

## Overview

A lightweight view over a character sequence. It does not copy or own the characters.

## Header

```cpp
#include "castle/container/string_view.h"
```

**Namespace:** `castle::container`

## Main API

| API | Purpose |
|---|---|
| `basic_string_view<CharT>` | Pointer plus length string view. |
| Constructors | Build from pointer/length or compatible character sources. |
| `size()` / `empty()` / `data()` | View state. |
| `operator[]` / iterators | Read characters and iterate. |
| `find()` | Search for a subview. |
| Comparison helpers | Compare views without copying when provided by the header. |

## Example

```cpp
castle::container::string_view text("HELLO");
auto pos = text.find(castle::container::string_view("LL"));
```

## Embedded notes

- The referenced characters must outlive the view.
- `find()` uses a simple bounded search, which is predictable but not a specialized string-search algorithm.
