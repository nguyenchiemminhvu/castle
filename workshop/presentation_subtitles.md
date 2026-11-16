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

**Transition:** Okay, enough charts. Let's look at some real code. I've got seven short samples, and each one shows a different part of the library. No slides with bullets from here on, just code.

---

## Slide 15 — Sample 1: Composite design pattern

Let's see some sample code, start with the composite pattern. Picture a typical ECU. You've got an HMI app, a DTC service, a diag service, and a few drivers: GPIO, CAN, SPI.

Then we group them, layer by layer, with `tie_composite`. And here's the important part: the groups don't copy anything. They only hold references to the objects we already have. And a group can hold another group. See `hal`? It holds the GPIO driver plus the whole `bus_drivers` group.

At the end, one `system` object wraps all three layers. Now when we want to shut down, it's one call. `for_each` walks every component, top-down and left to right, and calls `shutdown` on each one.

The number of children and their types are all fixed at compile time. So there's no runtime list to manage, and no virtual base class either.

**Transition:** So that's how we group objects. Next, let's look at controlling *when* a callback actually runs.

---

## Slide 16 — Sample 2: Callback execution policy

This one is about execution policies. The idea is simple: you wrap a callback, and the wrapper decides whether the call really happens.

First up is `once`. We call it twice, first with 2, then with 5. But only the first call goes through, so `hits` ends up as 2. That's really handy for things like one-time init, or a warning you only want to log once.

Next is `every_n` with an interval of three. We call it three times, and the callback fires once, on the third call. So `ticks` is 1. Think of a slow status print in a fast loop, or a watchdog that only needs every third tick.

The last one is `on_change`. It remembers the last value it saw, and it only calls you when the value is different. We didn't give it a starting value, so the very first call always fires. Then 2 is different from 1, so it fires again, and `changed_value` ends up as 2. If we called it with 2 a second time, nothing would happen. This is perfect for things like a gear position or a door state, where you only care when it changes.

**Transition:** All three of these still store a callable at runtime. But sometimes we already know the target at compile time. So let's go one step further.

---

## Slide 17 — Sample 3: Compile-time callback binding

Here we bind the callback at compile time.

At the top, we have a plain free function, `add_pair`. With `delegate_ptr_ct`, the function address is a template argument. So the delegate doesn't store a pointer at all. Call it with 7 and 8, and you get 15.

At the bottom, we do the same thing with a functor type. `delegate_ft_ct` takes the type, `multiplier`, plus the signature. It just creates a default `multiplier` when you call it. So `factor` is 1, and 3 times 1 plus 1 gives 4.

In both cases, there's no runtime state and no indirection to worry about. The compiler can see exactly what gets called, so it can inline it.

One small catch: the functor has to be default-constructible. If you need state, like a different factor, this isn't the right tool. Use the runtime delegate instead.

**Transition:** Okay, callbacks on their own are useful. But usually we want many listeners for one event. That's where the event dispatcher comes in.

---

## Slide 18 — Sample 4: Synchronous event dispatcher

You saw a tiny version of this earlier in the talk. Here's a slightly bigger one.

We've got two events, `SensorReadyEvent` and `SystemShutdownEvent`. They're just tag types. We don't even define them, we only declare them.

Each event gets its own config. Look at the two numbers in there. The `2U` means at most two listeners for that event. And the signature says what the callback looks like. Sensor ready passes a sample count. Shutdown passes nothing.

Then we put both configs into one dispatcher type. We register a callback for each event, and we keep the subscription objects. Those are our handles, so we can unsubscribe later.

Finally, we dispatch. It's synchronous, so every listener runs right there, inside the `dispatch_event` call. No queue, no extra thread.

**Transition:** The dispatcher is keyed by event type. If you'd rather think in terms of one signal and a few connected slots, CASTLE has that too.

---

## Slide 19 — Sample 5: Signal and slot

If you've used Qt or Boost.Signals, this one will feel familiar.

We have a small `counter` struct with an `add` member function. Then we make a signal. Again, the capacity is in the type. This signal can hold up to four slots, and each slot takes a `uint32_t`.

We connect the signal straight to a member function. We pass the object and `&counter::add`. No lambda, no `std::bind`. Then we emit 5, and `total` goes up by 5.

`connect` gives us back a connection object, the same idea as the subscription in the last sample. That way it's always clear who owns the link, and when it goes away.

**Transition:** Alright, that's it for callbacks and events. Let's switch to something completely different: physical units.

---

## Slide 20 — Sample 6: Units

Units are one of my favorite parts. A lot of embedded bugs are really unit bugs. Degrees mixed up with radians, or meters mixed up with millimeters.

Look at the GPS struct at the top. Latitude and longitude are stored as `geo_degrees<double>`, not as plain doubles. So you can't mix a latitude up with some random number by accident. The type tells you it's an angle, and it tells you the unit.

In the middle, we take 18 degrees and turn it into arcseconds and milliarcseconds. Just construct the other unit, and the scaling is done for you. No magic numbers like 3600 sprinkled through the code.

At the bottom, we compute speed. 100 meters divided by 4 seconds gives meters per second, 25. Then speed times 10 seconds gives meters again, 250. The types come out right on their own. If you tried to store that speed in a `meters` variable, it just wouldn't compile.

**Transition:** Everything so far came from the core library. For the last sample, let's step into the extensions layer and look at diagnostics.

---

## Slide 21 — Sample 7: CASTLE extensions, diagnostics

Last one. This is the diagnostic manager from the extensions.

We have three tags. CAN timeout, low battery, and comm lost. The whole setup is one type.

CAN timeout is a simple event. It goes active as soon as it's reported. Low battery uses `after_cycles<3U>`. So it has to stay reported for three update cycles before it counts. That's our debounce, and it's written right there in the type.

Comm lost is a group. It's active only when *both* CAN timeout and low battery are active. That's what `all_of` means.

Then we hook up a recovery action for comm lost. Here it just bumps a restart counter. In a real system, maybe you'd restart a module or go to a safe state.

We report both faults. Then, from a timer, we call `update`. First call, low battery is at one of three. Second call, two of three. On the third call, low battery is confirmed. So the group becomes active, and the recovery runs.

So all the rules, the debounce, the group logic, and the recovery, live in one place. And there's no heap and no hidden thread.

And that's it for the samples for today workshop. Thanks again, everyone.
