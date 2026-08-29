#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// CASTLE logging — embedded-friendly, allocation-free on the hot path.
//
// Design goals (vs. the previous std::ostringstream / std::string version):
//   • No heap allocation once the logger is constructed.  Every log record is
//     assembled in a stack-allocated fixed_string<N>, whose size N is a
//     compile-time template parameter.
//   • Deterministic upper bound on record size — critical for safety-critical
//     embedded systems (bounded stack usage, no fragmentation, no surprises).
//   • Records exceeding the compile-time budget are truncated and marked with
//     a trailing "..." so the operator can spot silently-clipped messages.
//   • Interfaces use std::string_view — zero-copy handoff to sinks/formatters.
//   • Same public shape as before: log(), log_info(), macros, singleton, DI.
//
// C++17 required (std::string_view, inline variables).
// ─────────────────────────────────────────────────────────────────────────────

#include "castle/buffers/fixed_string.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <ostream>

namespace castle
{
namespace logging
{

// ─────────────────────────────────────────────────────────────────────────────
// Compile-time defaults
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Default maximum length (in bytes, not counting the trailing NUL) of a single
 * assembled log record.  Chosen to fit comfortably on typical embedded stacks
 * while still leaving headroom for a short prefix + message.  Override by
 * instantiating basic_logger<N> explicitly.
 */
inline constexpr std::size_t default_log_capacity = 256;

// ─────────────────────────────────────────────────────────────────────────────
// Log level
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Severity levels. Ordered from least to most severe.
 * Use log_level::none as the minimum level to silence all output.
 */
enum class log_level : int
{
    debug   = 0,
    info    = 1,
    warning = 2,
    error   = 3,
    none    = 4  ///< Sentinel — disables all output when set as minimum level.
};

// ─────────────────────────────────────────────────────────────────────────────
// Interfaces — string_view based for zero-copy delivery.
// ─────────────────────────────────────────────────────────────────────────────

/** Write a pre-formatted record to an output destination. */
class i_log_sink
{
public:
    virtual ~i_log_sink() = default;
    virtual void write(std::string_view record) = 0;
};

/**
 * Convert raw log data into a formatted representation.  Because virtual
 * methods cannot be templates, the formatter emits its output as one or more
 * std::string_view chunks through a type-erased append callback; the logger
 * routes those chunks into its own fixed_string.
 */
class i_log_formatter
{
public:
    virtual ~i_log_formatter() = default;

    using append_fn = void(*)(void* ctx, std::string_view chunk);

    virtual void format(log_level        level,
                        long long        elapsed_ms,
                        std::string_view message,
                        append_fn        append,
                        void*            ctx) const = 0;
};

/** Decide whether a record at the given level should be emitted. */
class i_log_filter
{
public:
    virtual ~i_log_filter() = default;
    virtual bool is_enabled(log_level level) const noexcept = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Default implementations  (SRP — one responsibility each)
// ─────────────────────────────────────────────────────────────────────────────

/** Routes records to any std::ostream.  Default stream: std::cout. */
class ostream_sink final : public i_log_sink
{
public:
    explicit ostream_sink(std::ostream& out = std::cout) : out_(out) {}

    void write(std::string_view record) override
    {
        out_.write(record.data(), static_cast<std::streamsize>(record.size()));
        out_.put('\n');
    }

private:
    std::ostream& out_;
};

// ─── internal helpers ────────────────────────────────────────────────────────
namespace detail
{

inline const char* level_label(log_level lv) noexcept
{
    switch (lv)
    {
        case log_level::debug:   return "DEBUG";
        case log_level::info:    return "INFO ";
        case log_level::warning: return "WARN ";
        case log_level::error:   return "ERROR";
        default:                 return "?????";
    }
}

// Base case — nothing left to append.
template <std::size_t N>
inline void build_message(buffers::fixed_string<N>& /*buf*/) noexcept {}

// Recursive case — append the first argument, then recurse on the rest.
template <std::size_t N, typename T, typename... Rest>
void build_message(buffers::fixed_string<N>& buf, T&& first, Rest&&... rest)
{
    buf.append(std::forward<T>(first));
    build_message(buf, std::forward<Rest>(rest)...);
}

/** Concatenate any number of heterogeneous arguments into @p buf. */
template <std::size_t N, typename... Args>
void make_message(buffers::fixed_string<N>& buf, Args&&... args)
{
    build_message(buf, std::forward<Args>(args)...);
}

} // namespace detail

/** Formats records as:  [<elapsed_ms>ms][<LEVEL>] <message> */
class default_formatter final : public i_log_formatter
{
public:
    void format(log_level        level,
                long long        elapsed_ms,
                std::string_view message,
                append_fn        append,
                void*            ctx) const override
    {
        // Render the fixed prefix into a small stack scratch, then hand off.
        char prefix[48];
        const int n = std::snprintf(prefix, sizeof(prefix),
                                    "[%lldms][%s] ",
                                    elapsed_ms,
                                    detail::level_label(level));
        if (n > 0)
        {
            const std::size_t len =
                (static_cast<std::size_t>(n) < sizeof(prefix))
                    ? static_cast<std::size_t>(n)
                    : sizeof(prefix) - 1;
            append(ctx, std::string_view{ prefix, len });
        }
        append(ctx, message);
    }
};

/** Passes records whose level >= the configured minimum level. */
class level_filter final : public i_log_filter
{
public:
    explicit level_filter(log_level min_level = log_level::debug) noexcept
        : min_level_(min_level) {}

    bool is_enabled(log_level level) const noexcept override
    {
        return static_cast<int>(level) >= static_cast<int>(min_level_);
    }

    /** Update the minimum level at runtime. */
    void set_min_level(log_level min_level) noexcept { min_level_ = min_level; }

private:
    log_level min_level_;
};

// ─────────────────────────────────────────────────────────────────────────────
// basic_logger<MaxLen> — thread-safe, bounded, allocation-free on hot path.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Thread-safe logger with a compile-time bounded record size.
 *
 * Template parameter:
 *   MaxLen — maximum length (bytes) of a fully formatted record.  Records
 *            exceeding this budget are truncated with a trailing "...".
 *
 * Records flow through: filter → build message → formatter → sink.
 * Both the message buffer and the record buffer live on the caller's stack —
 * no dynamic allocation occurs per log call.
 *
 * Default collaborators (used when not injected):
 *   sink      — ostream_sink(std::cout)
 *   formatter — default_formatter
 *   filter    — level_filter(log_level::debug)   [accepts all levels]
 *
 * Thread safety: a single std::mutex serialises all log() calls and
 * configuration changes; safe to share across threads.
 */
template <std::size_t MaxLen = default_log_capacity>
class basic_logger
{
public:
    static constexpr std::size_t max_length = MaxLen;
    using clock_t = std::chrono::steady_clock;

    // ── Constructors ──────────────────────────────────────────────────────

    /** Default: all levels enabled, output to std::cout. */
    basic_logger()
        : sink_     (std::make_shared<ostream_sink>())
        , formatter_(std::make_shared<default_formatter>())
        , filter_   (std::make_shared<level_filter>())
        , start_    (clock_t::now())
    {}

    /** Logs to the provided stream; all other settings at their defaults. */
    explicit basic_logger(std::ostream& stream)
        : sink_     (std::make_shared<ostream_sink>(stream))
        , formatter_(std::make_shared<default_formatter>())
        , filter_   (std::make_shared<level_filter>())
        , start_    (clock_t::now())
    {}

    /** Full dependency-injection constructor — supply all three collaborators. */
    basic_logger(std::shared_ptr<i_log_sink>      sink,
                 std::shared_ptr<i_log_formatter> formatter,
                 std::shared_ptr<i_log_filter>    filter)
        : sink_     (std::move(sink))
        , formatter_(std::move(formatter))
        , filter_   (std::move(filter))
        , start_    (clock_t::now())
    {}

    basic_logger(const basic_logger&)            = delete;
    basic_logger& operator=(const basic_logger&) = delete;
    basic_logger(basic_logger&&)                 = delete;
    basic_logger& operator=(basic_logger&&)      = delete;

    // ── Runtime configuration ─────────────────────────────────────────────

    void set_sink(std::shared_ptr<i_log_sink> sink)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        sink_ = std::move(sink);
    }

    void set_formatter(std::shared_ptr<i_log_formatter> formatter)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        formatter_ = std::move(formatter);
    }

    void set_filter(std::shared_ptr<i_log_filter> filter)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        filter_ = std::move(filter);
    }

    /**
     * Convenience: adjust minimum level on the active filter.
     * No-op when the current filter is not a level_filter instance.
     */
    void set_min_level(log_level level)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (auto* f = dynamic_cast<level_filter*>(filter_.get()))
        {
            f->set_min_level(level);
        }
    }

    // ── Core logging API ─────────────────────────────────────────────────

    /**
     * Log with an explicit level and any number of heterogeneous arguments.
     * Assembly and formatting occur in stack-local fixed_string<MaxLen>
     * instances — no heap traffic, bounded stack cost.
     */
    template <typename... Args>
    void log(log_level level, Args&&... args)
    {
        std::lock_guard<std::mutex> lk(mutex_);

        if (!filter_->is_enabled(level)) return;

        // 1) Assemble the user message into a bounded buffer.
        buffers::fixed_string<MaxLen> msg;
        detail::make_message(msg, std::forward<Args>(args)...);

        // 2) Format prefix + message into the record buffer.
        buffers::fixed_string<MaxLen> record;
        formatter_->format(
            level,
            elapsed_ms(),
            msg.view(),
            &basic_logger::append_thunk,
            &record);

        // 3) Ship it.
        sink_->write(record.view());
    }

    // ── Convenience wrappers ──────────────────────────────────────────────

    template <typename... Args> void log_debug   (Args&&... a) { log(log_level::debug,   std::forward<Args>(a)...); }
    template <typename... Args> void log_info    (Args&&... a) { log(log_level::info,    std::forward<Args>(a)...); }
    template <typename... Args> void log_warning (Args&&... a) { log(log_level::warning, std::forward<Args>(a)...); }
    template <typename... Args> void log_error   (Args&&... a) { log(log_level::error,   std::forward<Args>(a)...); }

private:
    /** Trampoline used by the formatter to push chunks into our fixed_string. */
    static void append_thunk(void* ctx, std::string_view chunk) noexcept
    {
        static_cast<buffers::fixed_string<MaxLen>*>(ctx)->append(chunk);
    }

    long long elapsed_ms() const noexcept
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            clock_t::now() - start_).count();
    }

    std::shared_ptr<i_log_sink>      sink_;
    std::shared_ptr<i_log_formatter> formatter_;
    std::shared_ptr<i_log_filter>    filter_;
    const clock_t::time_point        start_;
    std::mutex                       mutex_;
};

/** Convenience alias: the default-sized logger used by the singleton/macros. */
using logger = basic_logger<default_log_capacity>;

// ─────────────────────────────────────────────────────────────────────────────
// Singleton registry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Process-wide logger singleton (Meyer's pattern) — initialised on first use,
 * destroyed at program exit.  Safe for multi-threaded access in C++11+.
 *
 * Templated on MaxLen so distinct TUs may pick different budgets, but the
 * convenience macros always target the default capacity.
 *
 * Usage:
 *   castle::logger_registry::instance().set_min_level(castle::log_level::info);
 *   castle::logger_registry::instance().log_info("Hello from thread ", id);
 */
template <std::size_t MaxLen = default_log_capacity>
class basic_logger_registry
{
public:
    /** Returns the single shared logger. */
    static basic_logger<MaxLen>& instance() noexcept
    {
        static basic_logger<MaxLen> instance_;
        return instance_;
    }

    /**
     * Convenience factory: reconfigure the global logger with a custom sink,
     * formatter, and filter in one call.
     */
    static void configure(
        std::shared_ptr<i_log_sink>      sink,
        std::shared_ptr<i_log_formatter> formatter = std::make_shared<default_formatter>(),
        std::shared_ptr<i_log_filter>    filter    = std::make_shared<level_filter>())
    {
        instance().set_sink     (std::move(sink));
        instance().set_formatter(std::move(formatter));
        instance().set_filter   (std::move(filter));
    }

    basic_logger_registry()                                        = delete;
    ~basic_logger_registry()                                       = delete;
    basic_logger_registry(const basic_logger_registry&)            = delete;
    basic_logger_registry& operator=(const basic_logger_registry&) = delete;
};

/** Alias matching the pre-refactor public name. */
using logger_registry = basic_logger_registry<default_log_capacity>;

// ─────────────────────────────────────────────────────────────────────────────
// Convenience macros — route through the process-wide singleton
// ─────────────────────────────────────────────────────────────────────────────

/**
 * LOG_DEBUG / LOG_INFO / LOG_WARNING / LOG_ERROR
 *
 * Accept any number of heterogeneous arguments, forwarded directly to the
 * matching logger::log_*() method on the global singleton.
 *
 * Records are assembled in a stack-allocated buffer of
 * castle::default_log_capacity bytes.  Content beyond that budget is
 * truncated and marked with a trailing "..." — no heap allocation, no
 * unbounded stack growth, safe for hard-real-time paths.
 *
 * Example:
 *   LOG_INFO("Satellites in view: ", count, " (fix=", fix, ")");
 *   LOG_ERROR("UART read failed — errno: ", errno);
 */
#define LOG_DEBUG(...)   ::logger_registry::instance().log_debug  (__VA_ARGS__)
#define LOG_INFO(...)    ::logger_registry::instance().log_info   (__VA_ARGS__)
#define LOG_WARNING(...) ::logger_registry::instance().log_warning(__VA_ARGS__)
#define LOG_ERROR(...)   ::logger_registry::instance().log_error  (__VA_ARGS__)

/**
 * LOG_SET_LEVEL(level)
 *
 * Adjust the minimum log level on the global singleton's active filter at
 * runtime.  @p level must be a castle::log_level enumerator.
 *
 * Example:
 *   LOG_SET_LEVEL(castle::log_level::warning);  // suppress DEBUG and INFO
 */
#define LOG_SET_LEVEL(level) ::logger_registry::instance().set_min_level(level)

} // namespace logging
} // namespace castle