# Callback function wrappers

## Overview

A family of signature-aware callable wrappers. They cover free functions, functors/lambdas, runtime-bound member functions, and compile-time-bound function pointers.

## Header

```cpp
#include "castle/callbacks/function.h"
```

**Namespace:** `castle::callbacks`

## Main API

| API | Purpose |
|---|---|
| `i_function<R(Args...)>` | Minimal type-erased interface used by registries. |
| `function<R(Args...)>` | Stores a free/static function pointer. |
| `function_f<Callable, R(Args...)>` | Stores a callable object or lambda by value. |
| `function_m<Obj, R(Args...)>` | Binds a runtime object reference/pointer with a member function. |
| `function_ct<Func>` | Compile-time-bound free/static function wrapper. |
| `function_ct_m<MemFunc>` | Compile-time-bound member function with a runtime object. |
| `function_ct_im<...>` | Compile-time-bound member callback with the object supplied through the template configuration. |
| Factory helpers | `make_...` helpers are provided where type deduction is useful. |

## Example

```cpp
int add(int a, int b) { return a + b; }
castle::callbacks::function<int(int,int)> cb(&add);
int value = cb(2, 3);
```

## Embedded notes

- Only the small `i_function` interface uses virtual dispatch.
- Compile-time-bound variants avoid storing the function pointer itself and are easier for the optimizer to inline.
- The wrappers do not use dynamic allocation.
