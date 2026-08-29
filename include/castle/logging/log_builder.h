#pragma once

#include "castle/buffers/fixed_string.h"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace castle
{
namespace logging
{

// ============================================================================
// Configuration
// ============================================================================

inline constexpr std::size_t default_log_capacity = 256U;


// ============================================================================
// Log level
// ============================================================================

enum class log_level : std::uint8_t
{
    debug   = 0U,
    info    = 1U,
    warning = 2U,
    error   = 3U,
    none    = 4U
};


// ============================================================================
// Callback types
// ============================================================================

/**
 * Sink callback.
 *
 * The logger does not own the sink.
 *
 * `context` is supplied by the application and is typically a pointer to
 * UART, CAN, RTT, ITM, memory buffer, etc.
 */
using sink_fn = void (*)(void* context, std::string_view record) noexcept;


/**
 * Timestamp callback.
 *
 * Returns elapsed time in milliseconds.
 *
 * Returning uint32_t is intentional:
 *
 * - deterministic
 * - small
 * - suitable for embedded timers
 * - rollover is well-defined
 *
 * The logger itself does not depend on std::chrono.
 */
using timestamp_fn = std::uint32_t (*)(void* context) noexcept;


/**
 * Optional filter callback.
 *
 * Return true if the specified level should be emitted.
 *
 * The context pointer belongs to the application.
 */
using filter_fn = bool (*)(void* context, log_level level) noexcept;


// ============================================================================
// Sink
// ============================================================================

/**
 * Non-owning sink descriptor.
 *
 * No virtual function.
 * No RTTI.
 * No heap.
 */
struct log_sink
{
    sink_fn function;
    void*   context;

    constexpr bool valid() const noexcept
    {
        return function != nullptr;
    }

    void write(std::string_view record) const noexcept
    {
        if (function != nullptr)
        {
            function(context, record);
        }
    }
};


// ============================================================================
// Clock
// ============================================================================

/**
 * Non-owning clock descriptor.
 */
struct log_clock
{
    timestamp_fn function;
    void*        context;

    constexpr bool valid() const noexcept
    {
        return function != nullptr;
    }

    std::uint32_t milliseconds() const noexcept
    {
        if (function != nullptr)
        {
            return function(context);
        }

        return 0U;
    }
};


// ============================================================================
// Default helpers
// ============================================================================

namespace detail
{

inline const char* level_label(log_level level) noexcept
{
    switch (level)
    {
        case log_level::debug:
        {
            return "DEBUG";
        }
        case log_level::info:
        {
            return "INFO ";
        }
        case log_level::warning:
        {
            return "WARN ";
        }
        case log_level::error:
        {
            return "ERROR";
        }
        case log_level::none:
        default:
        {
            return "?????";
        }
    }
}


/**
 * Default level filter.
 *
 * Kept as a normal function instead of a polymorphic object.
 */
inline bool level_filter(void* context, log_level level) noexcept
{
    const auto minimum = *static_cast<const log_level*>(context);

    return static_cast<std::uint8_t>(level)
        >= static_cast<std::uint8_t>(minimum);
}


/**
 * Append one argument.
 */
template <std::size_t N, typename T>
void append_argument(buffers::fixed_string<N>& buffer, T&& value) noexcept
{
    buffer.append(static_cast<T&&>(value));
}


/**
 * Recursive argument builder.
 */
template <std::size_t N>
void build_message(buffers::fixed_string<N>&) noexcept
{
}


/**
 * Recursive argument builder.
 *
 * Example:
 *
 * build_message(buffer,
 *               "speed=",
 *               speed,
 *               ", rpm=",
 *               rpm);
 */
template <std::size_t N, typename T, typename... Rest>
void build_message(buffers::fixed_string<N>& buffer, T&& first, Rest&&... rest) noexcept
{
    append_argument(
        buffer,
        static_cast<T&&>(first)
    );

    build_message(
        buffer,
        static_cast<Rest&&>(rest)...
    );
}


/**
 * Default formatter.
 *
 * Result:
 *
 * [123ms][INFO ] speed=100
 *
 * The formatter writes directly into the caller-owned fixed_string.
 *
 * No temporary std::string.
 * No stream.
 * No heap.
 */
template <std::size_t N>
void default_format(
    buffers::fixed_string<N>& output,
    log_level level,
    std::uint32_t timestamp,
    std::string_view message) noexcept
{
    output.append("[");
    output.append(timestamp);
    output.append("ms][");
    output.append(level_label(level));
    output.append("] ");
    output.append(message);
}

} // namespace detail


// ============================================================================
// Formatter
// ============================================================================

/**
 * Formatter callback.
 *
 * The formatter does not own the output buffer.
 *
 * The logger supplies a fixed_string<N>, therefore the formatter is also
 * allocation-free and bounded.
 */
template <std::size_t MaxLen>
using formatter_fn =
    void (*)(
        buffers::fixed_string<MaxLen>& output,
        log_level level,
        std::uint32_t timestamp,
        std::string_view message) noexcept;


// ============================================================================
// log_builder
// ============================================================================

/**
 * Fixed-capacity, non-owning, allocation-free logger.
 *
 * Design properties:
 *
 *   - no heap allocation
 *   - no RTTI
 *   - no virtual functions
 *   - no dynamic_cast
 *   - no singleton
 *   - compile-time maximum record length
 *   - caller/application owns sink
 *   - deterministic memory footprint
 *   - suitable for freestanding-style embedded code
 *
 * Example:
 *
 *     uart_driver uart;
 *
 *     castle::logging::log_builder<256> log{
 *         {
 *             &uart_log_sink,
 *             &uart
 *         }
 *     };
 *
 *     log.info("speed=", speed);
 *     log.warning("temperature=", temperature);
 *     log.error("CAN error=", error);
 */
template <std::size_t MaxLen = default_log_capacity>
class log_builder
{
    static_assert(MaxLen >= 16U,
                  "log_builder requires at least 16 bytes");

public:

    static constexpr std::size_t max_length = MaxLen;

    using buffer_type = buffers::fixed_string<MaxLen>;
    using formatter_type = formatter_fn<MaxLen>;


    // ========================================================================
    // Construction
    // ========================================================================

    /**
     * Construct a logger with a sink.
     *
     * Default configuration:
     *
     *     minimum level = debug
     *     default formatter
     *     no timestamp
     */
    explicit constexpr log_builder(log_sink sink) noexcept
        : sink_(sink)
        , clock_{nullptr, nullptr}
        , filter_(&detail::level_filter)
        , filter_context_(&minimum_level_)
        , formatter_(&detail::default_format<MaxLen>)
        , minimum_level_(log_level::debug)
    {
    }


    /**
     * Construct with sink + clock.
     */
    constexpr log_builder(log_sink sink, log_clock clock) noexcept
        : sink_(sink)
        , clock_(clock)
        , filter_(&detail::level_filter)
        , filter_context_(&minimum_level_)
        , formatter_(&detail::default_format<MaxLen>)
        , minimum_level_(log_level::debug)
    {
    }


    /**
     * Construct with complete configuration.
     *
     * The application owns all objects referenced by the callbacks.
     */
    constexpr log_builder(
        log_sink sink,
        log_clock clock,
        formatter_type formatter,
        filter_fn filter = &detail::level_filter,
        void* filter_context = nullptr) noexcept
        : sink_(sink)
        , clock_(clock)
        , filter_(filter)
        , filter_context_(filter_context)
        , formatter_(formatter)
        , minimum_level_(log_level::debug)
    {
        /*
         * If no filter context is supplied, use the logger's own minimum
         * level.
         */
        if (filter_context_ == nullptr)
        {
            filter_context_ = &minimum_level_;
        }

        if (formatter_ == nullptr)
        {
            formatter_ = &detail::default_format<MaxLen>;
        }
    }


    log_builder(const log_builder&) = delete;
    log_builder& operator=(const log_builder&) = delete;

    log_builder(log_builder&&) = delete;
    log_builder& operator=(log_builder&&) = delete;


    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * Change the minimum log level.
     *
     * No RTTI is required because the default filter directly references
     * minimum_level_.
     */
    void set_min_level(log_level level) noexcept
    {
        minimum_level_ = level;
    }


    log_level min_level() const noexcept
    {
        return minimum_level_;
    }


    /**
     * Replace the sink.
     *
     * The logger does not take ownership.
     */
    void set_sink(log_sink sink) noexcept
    {
        sink_ = sink;
    }


    /**
     * Replace the timestamp source.
     */
    void set_clock(log_clock clock) noexcept
    {
        clock_ = clock;
    }


    /**
     * Replace the formatter.
     */
    void set_formatter(formatter_type formatter) noexcept
    {
        formatter_ =
            (formatter != nullptr)
                ? formatter
                : &detail::default_format<MaxLen>;
    }


    /**
     * Replace the filter.
     *
     * `context` must remain alive while the logger uses it.
     */
    void set_filter(filter_fn filter, void* context) noexcept
    {
        filter_ = filter;
        filter_context_ = context;

        if (filter_ == nullptr)
        {
            filter_ = &detail::level_filter;
            filter_context_ = &minimum_level_;
        }
    }


    // ========================================================================
    // Filtering
    // ========================================================================

    bool is_enabled(log_level level) const noexcept
    {
        if (filter_ == nullptr)
        {
            return false;
        }

        return filter_(
            filter_context_,
            level
        );
    }


    // ========================================================================
    // Core logging API
    // ========================================================================

    template <typename... Args>
    void log(log_level level, Args&&... args) noexcept
    {
        /*
         * Important:
         *
         * Filtering happens BEFORE creating the fixed_string buffers.
         *
         * This matters on embedded systems because it avoids stack usage for
         * disabled log levels.
         */
        if (!is_enabled(level))
        {
            return;
        }

        /*
         * Message buffer.
         *
         * Entirely stack allocated.
         *
         * Size = MaxLen + fixed_string bookkeeping.
         */
        buffer_type message;

        detail::build_message(
            message,
            static_cast<Args&&>(args)...
        );

        /*
         * Final record buffer.
         */
        buffer_type record;

        const std::uint32_t timestamp =
            clock_.milliseconds();

        formatter_(
            record,
            level,
            timestamp,
            message.view());

        /*
         * The sink receives a string_view.
         *
         * It must consume/copy the data synchronously.
         *
         * It must NOT store the pointer after returning.
         */
        sink_.write(record.view());
    }


    // ========================================================================
    // Convenience APIs
    // ========================================================================

    template <typename... Args>
    void debug(Args&&... args) noexcept
    {
        log(log_level::debug, static_cast<Args&&>(args)...);
    }


    template <typename... Args>
    void info(Args&&... args) noexcept
    {
        log(log_level::info, static_cast<Args&&>(args)...);
    }


    template <typename... Args>
    void warning(Args&&... args) noexcept
    {
        log(log_level::warning, static_cast<Args&&>(args)...);
    }


    template <typename... Args>
    void error(Args&&... args) noexcept
    {
        log(log_level::error, static_cast<Args&&>(args)...);
    }


    // Compatibility-style names if desired.

    template <typename... Args>
    void log_debug(Args&&... args) noexcept
    {
        debug(static_cast<Args&&>(args)...);
    }


    template <typename... Args>
    void log_info(Args&&... args) noexcept
    {
        info(static_cast<Args&&>(args)...);
    }


    template <typename... Args>
    void log_warning(Args&&... args) noexcept
    {
        warning(static_cast<Args&&>(args)...);
    }


    template <typename... Args>
    void log_error(Args&&... args) noexcept
    {
        error(static_cast<Args&&>(args)...);
    }


private:

    log_sink sink_;
    log_clock clock_;
    filter_fn filter_;
    void* filter_context_;
    formatter_type formatter_;

    /*
     * Stored inside the logger itself so the default filter requires no
     * external object and no dynamic allocation.
     */
    log_level minimum_level_;
};

} // namespace logging
} // namespace castle
