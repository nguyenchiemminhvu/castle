# CASTLE Workshop — Speaking Content

## Slide 1 — CASTLE: C++ built for bounded systems

Hi everyone, thanks for being here. Today I want to introduce the CASTLE library. But more than the library, I want to talk about the way of thinking behind it.

CASTLE stands for Compliant Automotive Safety Template Library for Embedded. It is a C++17 header-only library for embedded software. Its goal is simple. It makes familiar C++ tools work well in systems where memory, timing, and errors really matter.

The diagram on this slide shows where CASTLE fits. Read it from the top down. At the top are the automotive applications, such as ADAS, infotainment, and diagnostics. Middleware serves those applications, and it sits on the platform. The platform is the execution environment. It can be bare metal, embedded Linux, Zephyr, QNX, VxWorks, or AUTOSAR Classic. At the bottom is the board support package, which provides the bootloader, the MCU or SoC drivers, and the hardware interfaces like CAN, Ethernet, and SPI.

CASTLE is not a layer in that stack. It sits beside the middleware as a cross-cutting library. Both the middleware and the application code can use it. It is a C++ toolbox that replaces the STL and other general-purpose libraries where those do not fit an embedded target. Because it is a toolbox rather than a layer, the question is not where CASTLE sits. The question is how its tools should change the code we write.

However, this is not a deep technical talk. We won't go through every container or every header. Instead, we'll look at the problems CASTLE tries to solve, and then at the habits those problems should build when we write and review code.

The main idea is this: make the important costs visible before the code runs.

**Transition:** So why does that matter? Look at the platform layer in the diagram. That is where our firmware actually runs.

---

## Slide 2 — Embedded systems pay for hidden decisions

When many C++ features are useful. On a desktop or a server, their cost is usually fine. The problem is not that these features are bad. The problem is that an embedded device has a small budget, and often strict timing rules.

Let's look at a few examples.

Dynamic allocation. How much memory is free can depend on what ran before. An allocation can fail. And over time, memory can get fragmented, so the free memory is hard to use. A line of code that looks simple can cost more than you expect.

Exceptions. They can jump across many function calls, so the flow is hard to follow.

Virtual functions. They add an extra step at runtime. It can be hard to see which function actually runs.

Standard library parts. They may need things that your target does not have.

And any code where the worst-case time is unclear makes timing analysis hard.

So modern C++ is not wrong for embedded work. We just need to be careful. Before we use a feature, ask three questions. How much memory can it use? How can it fail? How much work can it do?

**Transition:** CASTLE starts with the first one: memory.

---

## Slide 3 — Make the memory budget visible

Look at the type in this example. The vector can hold sixteen items. That number is written right in the type. It is not in a config file, and it does not change at runtime. The container keeps its items inside itself.

This changes how we think. We don't ask, "How big can this get?" We ask, "How many samples do we really need?" Then we connect that number to the data rate, the time between readers, and how much memory this part of the system is allowed to use.

The container does not grow by itself. When it is full, `push_back` returns `status::full`. Then the caller decides what to do. Reject the new item? Slow down the sender? Overwrite old data using a rule we wrote down? Or log an error? Only the application knows what losing data really means. So the decision belongs to the application.

This does not mean every operation is fast. It means the size limit and the "full" case are clear. That is a good start for a memory budget.

**Transition:** Once failure is visible, we can treat it as normal flow.

---

## Slide 4 — Make failure an ordinary result

Here is a small change in mindset. In embedded code, a full buffer is not a rare error. An empty queue is not a bug. These things can happen every day. So we should handle them as normal.

CASTLE uses a status type for this. `status::ok` means success. The other values tell us what went wrong. The destination is full. The source is empty. An item was not found. An argument is not valid. A system call failed. Some simple operations just return true or false.

Now the caller can see the failure choice right next to the call. Should we retry? Drop the data? Move to a safe state? Report a fault? These are product decisions. They should not be hidden inside a general-purpose container.

One more thing. A status is for an expected result at runtime. An assert is for a broken rule in our code. For example, using an object outside its valid range. CASTLE has a setting to control how asserts behave.

**Transition:** The same idea works for abstraction and dispatch.

---

## Slide 5 — Pay for abstraction deliberately

CASTLE does not try to remove abstraction. We still want clean, expressive code. It uses templates, type traits, and small helper types. That way, the code stays easy to read, and it does not need a lot of runtime machinery.

The default choice is compile-time selection. If the type is known at compile time, the compiler can generate the right code directly. We don't need a class hierarchy at runtime just to pick one known action.

For callbacks, CASTLE has fixed-storage options. The function wrapper keeps the callable in a small local buffer. It does not use the heap. If the callable is too big, the compiler gives you an error. So you find the problem before the code runs, not in the field.

Now, be careful with virtual functions. The core data structures do not use them. But some interface parts do. Observer, visitor, and delegate interfaces use virtual functions. That is a deliberate choice, not a rule that virtual functions never show up. When you use one of these, you should know why it helps, and what it costs.

**Transition:** These ideas are not only for containers. They show up across many modules.

---

## Slide 6 — One library, two layers

This slide shows the shape of the project. There are two layers.

The first layer is the core. It has the basic building blocks. Containers. Memory helpers. Utilities and algorithms. Bit operations. Time and math. Callbacks and events. Sync tools. Iterators. And error handling.

The second layer is the extensions. They use the core for more specific jobs. In the source, you will find checksums and codecs, crypto, diagnostics, filters, geodetic math, parsers, protocols like CAN and NMEA, serialization, and hardware I/O.

Why does this matter? Because the same thinking goes all the way from a small data structure to a real firmware feature. For example, the protocol and serialization parts use fixed buffers. They don't assume data can grow forever.

One practical note. Not every extension works on every target. Some use POSIX or Linux APIs. Some use pthreads. There is also a Zephyr GPIO backend. So only include what your platform supports.

**Transition:** So what does this look like in everyday code?

---

## Slide 7 — A different everyday coding habit

This example is short. We create an event dispatcher for one event, called `TickEvent`. The number four is a fixed limit for this event. Then we register a callback, dispatch the event, and unsubscribe.

Let's look at the three questions on the slide.

In the type: how much can exist? The limit is written in the type. You can see it without searching.

At the call site: what can go wrong? The register and dispatch calls are where the work happens. Read them, and think about failure.

In the owner: who decides how long it lives? Here, the subscription object is the handle. We call `unsubscribe` when we are done. The owner must make sure the callback is not used after that.

So before you add a buffer, decide its size. Before you add a callback, decide who owns it. The code has a few more lines than usual. Those lines are not noise. They show decisions the system had to make anyway.

**Transition:** Now, these ideas help us ask better questions in a code review.

---

## Slide 8 — A code review asks different questions

Now think about reviewing code, not writing it. We still check correctness, readability, and tests. But we also ask four questions about resources.

First, memory. Where does the storage live? Who owns it? Is it a borrowed pointer, or does the object own the data? What is the maximum size?

Second, capacity and failure. What happens when the queue is full? When the buffer is empty? When a lookup fails? Is the failure handled, or is it quietly ignored?

Third, timing. What is the worst case? Is that amount of work okay for the task period? A bounded container helps. But it does not replace understanding the algorithm and counting the operations.

Fourth, concurrency and lifetime. Can this run from more than one task? Can it run from an interrupt? Who protects the shared data? Can a callback or a subscription live longer than the object it points to?

A good review can answer these from the design and the code. A passing test alone is not enough.

**Transition:** These habits make code more predictable. But we must be clear about what the library can prove, and what it can't.

---

## Slide 9 — Predictability improves; it still must be proven

CASTLE helps us reason about each component. Known storage limits help with RAM planning. Clear statuses help us decide how to recover. Avoiding heap, exceptions, RTTI, and the STL means less hidden runtime work to count. And consistent patterns make code easier for a team to maintain.

But a predictable container does not make a predictable system. We still need to measure worst-case timing on the real microcontroller. We need to size buffers using real traffic and real fault cases. We also need to know the OS, the scheduler, the interrupt rules, the locks, the compiler, and the processor.

A small queue can still be too small. A bounded algorithm can still be too slow. A race condition can still break everything. CASTLE gives us better tools, and better places to make decisions. But it does not replace testing at the system level.

So our goal is not to claim the system is deterministic. Our goal is to design for it, and then check it on the real target.

**Transition:** Let's finish with four habits.

---

## Slide 10 — Design the bounds, then write the code

To close, here are four things to remember.

1. Choose storage and capacity at the start.
2. Make the expected results clear.
3. Keep lifetimes and dispatch easy to see.
4. Measure timing and concurrency on the real target.

When you review or design a component, you should be able to answer three things. How much memory can it use? What happens when it hits the limit? How much work can it do?

CASTLE is not about writing less C++. It is about making fewer hidden decisions. We still want clean, reusable, modern C++. We just want the memory model and the failure rules to be part of the design from day one.

The next time you add a container, a callback, a queue, or a parser, start by asking: what should the limit be? Then write the code.

Thank you for listening.

---

## Slide 11 — Appendix A: Benchmark setup

Here I ran the benchmarks on a 16-CPU host at about 2 GHz, using Google Benchmark. Each CASTLE operation has an STL counterpart, run under the same harness. The metric is CPU time per operation, in nanoseconds.

Three things to keep in mind. First, some rows ran only one iteration and report times near 1.4 times 10 to the 11 nanoseconds. Those values are not usable, so we left them out. Second, rows near 0.2 nanoseconds sit at the benchmark's own loop floor. The compiler probably removed that work, so those rows say nothing about the code. Third, this is one run on a shared machine, and the build flags were not recorded. Treat small gaps as noise until they repeat.

**Transition:** With those limits in mind, here is the overall picture.

---

## Slide 12 — Appendix B: Speed ratio overview

Each bar is a ratio: STL time divided by CASTLE time. A bar past the center line means CASTLE is faster. I treat anything within three percent as parity, because a single run can move that much.

Most rows sit at or near the line. That is the main message. Vector push_back, stack push and pop, and the in-place callback call match the STL within noise.

The clear wins are string_find, at about 2.1 times faster. Array access, bitset to_string, and the hash and tree containers are about 1.2 to 1.3 times faster. Two rows go the other way. Queue push and pop, and string construct, are about five to six percent slower on CASTLE in this run.

**Transition:** The largest group of differences is in the associative containers.

---

## Slide 13 — Appendix C: Associative containers

This chart compares hash map, hash set, tree map, and tree set, for insert and erase. All eight operations are faster on CASTLE in this run, between about 1.17 and 1.32 times.

The hash set insert shows the largest gap, at about 1.32 times. The tree insert operations show the smallest, at about 1.17 to 1.18 times.

Be careful about cause. This capture shows the difference, but not why it exists. Before we name a reason in a review, read the implementation and profile it on the target.

**Transition:** The last appendix slide covers sequences, strings, and bit operations.

---

## Slide 14 — Appendix D: Sequences, strings, and bits

The rows are split into two charts because their scales differ a lot. The left chart goes up to 1000 nanoseconds. The right chart goes up to 70 nanoseconds, so the small operations stay readable.

On the left, vector push_back, vector emplace_back, and string append match the STL closely. Stack push and pop match too. Queue push and pop is about five percent slower on CASTLE.

On the right, array access is about 1.31 times faster, string find is about 2.11 times faster, and bitset to_string is about 1.26 times faster. String construct is about six percent slower on CASTLE.

The takeaway matches the rest of the talk. Outside the highlighted rows, the speed gaps are small. The reason to choose CASTLE is its bounded storage and explicit failure paths. Speed alone should not decide it.
