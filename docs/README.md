# Castle 2.0 documentation

The layout mirrors `include/castle`: for example, `include/castle/container/vector.h` is documented by `docs/container/vector.md`.

## Design focus

Castle is aimed at embedded systems where deterministic behavior, fixed memory use, and small dependencies matter. The documentation emphasizes fixed-capacity storage, explicit lifetime, status-code error handling, and the small number of places where runtime type erasure or virtual dispatch is intentionally used.

## How to read a page

Each page uses the same simple structure:

1. Overview — what the header is for.
2. Header — the include path and namespace.
3. Main API — the important types/functions in plain language.
4. Example — a small usage example.
5. Embedded notes — limits, ownership, determinism, and common pitfalls.
