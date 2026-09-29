// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file node.hpp
 * @brief Hardware-independent LIN master and slave node facades.
 *
 * Nodes use fixed inline callback storage and return status values rather than
 * throwing. A target adapter is responsible for break/sync generation, UART
 * framing, bus timing, and calling these APIs from its driver/event loop.
 */
#ifndef CASTLE_EXT_PROTOCOLS_LIN_NODE_HPP
#define CASTLE_EXT_PROTOCOLS_LIN_NODE_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle_ext/protocols/lin/checksum.hpp"
#include "castle_ext/protocols/lin/schedule.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace lin
{

/**
 * @brief LIN master/commander facade that emits headers and master-published responses.
 *
 * Call `transmit_slot()` for the next schedule entry. The method emits a
 * header first; when the entry's owner is `master`, it then asks the configured
 * provider for a response and transmits it. A slave-owned response is supplied
 * by a slave node through the bus adapter. Timing between slots is external.
 */
template <
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class master_node CASTLE_FINAL
{
    static_assert(CallbackStorageSize > 0U,
                  "master_node callback storage must be non-zero");
    static_assert(CallbackStorageAlignment > 0U
                  && (CallbackStorageAlignment & (CallbackStorageAlignment - 1U)) == 0U,
                  "master_node callback storage alignment must be a power of two");

public:
    using header_transmitter_type = castle::callbacks::function<
        castle::status(CASTLE_CONST header&), CallbackStorageSize, CallbackStorageAlignment>;
    using response_transmitter_type = castle::callbacks::function<
        castle::status(CASTLE_CONST frame&), CallbackStorageSize, CallbackStorageAlignment>;
    using response_provider_type = castle::callbacks::function<
        castle::status(uint8_t, frame&), CallbackStorageSize, CallbackStorageAlignment>;
    using response_handler_type = castle::callbacks::function<
        void(CASTLE_CONST frame&), CallbackStorageSize, CallbackStorageAlignment>;

    master_node() CASTLE_NOEXCEPT
        : header_transmitter_{}, response_transmitter_{}, response_provider_{},
          response_handler_{}, header_count_(0U), transmitted_response_count_(0U),
          received_response_count_(0U), invalid_count_(0U)
    {
    }

    /** @brief Installs a header adapter callback. */
    void configure_header_transmitter(header_transmitter_type&& callback) CASTLE_NOEXCEPT
    {
        header_transmitter_ = CASTLE_MOVE(callback);
    }

    template <typename Callback, typename = castle::meta::enable_if_t<
        !castle::meta::is_same<castle::meta::decay_t<Callback>, header_transmitter_type>::value>>
    void configure_header_transmitter(Callback&& callback)
    {
        header_transmitter_type wrapper(CASTLE_FORWARD<Callback>(callback));
        header_transmitter_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Installs the adapter callback for responses published by the master. */
    void configure_response_transmitter(response_transmitter_type&& callback) CASTLE_NOEXCEPT
    {
        response_transmitter_ = CASTLE_MOVE(callback);
    }

    template <typename Callback, typename = castle::meta::enable_if_t<
        !castle::meta::is_same<castle::meta::decay_t<Callback>, response_transmitter_type>::value>>
    void configure_response_transmitter(Callback&& callback)
    {
        response_transmitter_type wrapper(CASTLE_FORWARD<Callback>(callback));
        response_transmitter_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Installs an application provider for master-published response data. */
    void configure_response_provider(response_provider_type&& callback) CASTLE_NOEXCEPT
    {
        response_provider_ = CASTLE_MOVE(callback);
    }

    template <typename Callback, typename = castle::meta::enable_if_t<
        !castle::meta::is_same<castle::meta::decay_t<Callback>, response_provider_type>::value>>
    void configure_response_provider(Callback&& callback)
    {
        response_provider_type wrapper(CASTLE_FORWARD<Callback>(callback));
        response_provider_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Installs a listener for validated responses received from the bus. */
    void configure_response_handler(response_handler_type&& callback) CASTLE_NOEXCEPT
    {
        response_handler_ = CASTLE_MOVE(callback);
    }

    template <typename Callback, typename = castle::meta::enable_if_t<
        !castle::meta::is_same<castle::meta::decay_t<Callback>, response_handler_type>::value>>
    void configure_response_handler(Callback&& callback)
    {
        response_handler_type wrapper(CASTLE_FORWARD<Callback>(callback));
        response_handler_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Clears the header transmitter callback. */
    void clear_header_transmitter() CASTLE_NOEXCEPT { header_transmitter_ = header_transmitter_type{}; }
    /** @brief Clears the response transmitter callback. */
    void clear_response_transmitter() CASTLE_NOEXCEPT { response_transmitter_ = response_transmitter_type{}; }
    /** @brief Clears the response provider callback. */
    void clear_response_provider() CASTLE_NOEXCEPT { response_provider_ = response_provider_type{}; }
    /** @brief Clears the receive handler callback. */
    void clear_response_handler() CASTLE_NOEXCEPT { response_handler_ = response_handler_type{}; }

    /** @brief Successfully emitted headers (modulo 2^32). */
    CASTLE_NODISCARD uint32_t header_count() CASTLE_CONST CASTLE_NOEXCEPT { return header_count_; }
    /** @brief Successfully transmitted master-published responses (modulo 2^32). */
    CASTLE_NODISCARD uint32_t transmitted_response_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return transmitted_response_count_;
    }
    /** @brief Structurally and checksum-valid bus responses (modulo 2^32). */
    CASTLE_NODISCARD uint32_t received_response_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return received_response_count_;
    }
    /** @brief Invalid frames or headers rejected (modulo 2^32). */
    CASTLE_NODISCARD uint32_t invalid_count() CASTLE_CONST CASTLE_NOEXCEPT { return invalid_count_; }

    /** @brief Emits the logical header for one frame ID. */
    castle::status send_header(uint32_t identifier) CASTLE_NOEXCEPT
    {
        header candidate{};
        const castle::status built = make_header(identifier, candidate);
        if (!castle::succeeded(built))
        {
            ++invalid_count_;
            return built;
        }
        if (!header_transmitter_)
        {
            return castle::status::not_configured;
        }
        const castle::status result = header_transmitter_(candidate);
        if (castle::succeeded(result))
        {
            ++header_count_;
        }
        return result;
    }

    /** @brief Obtains and transmits the master's response for an identifier. */
    castle::status send_master_response(uint32_t identifier) CASTLE_NOEXCEPT
    {
        if (!is_valid_identifier(identifier))
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        if (!response_provider_ || !response_transmitter_)
        {
            return castle::status::not_configured;
        }

        frame candidate{};
        const castle::status provided = response_provider_(static_cast<uint8_t>(identifier), candidate);
        if (!castle::succeeded(provided))
        {
            return provided;
        }
        if (!candidate.valid() || candidate.identifier != identifier || !validate_checksum(candidate))
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }

        const castle::status transmitted = response_transmitter_(candidate);
        if (castle::succeeded(transmitted))
        {
            ++transmitted_response_count_;
        }
        return transmitted;
    }

    /** @brief Emits a schedule slot's header and, if master-owned, its response. */
    castle::status transmit_slot(CASTLE_CONST schedule_entry& entry) CASTLE_NOEXCEPT
    {
        if (!entry.valid())
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        const castle::status header_result = send_header(entry.identifier);
        if (!castle::succeeded(header_result))
        {
            return header_result;
        }
        if (entry.owner == response_owner::slave)
        {
            return castle::status::ok;
        }
        return send_master_response(entry.identifier);
    }

    /** @brief Validates and dispatches one response observed on the LIN bus. */
    castle::status receive_response(CASTLE_CONST frame& value) CASTLE_NOEXCEPT
    {
        if (!value.valid() || !validate_checksum(value))
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        ++received_response_count_;
        if (!response_handler_)
        {
            return castle::status::not_configured;
        }
        response_handler_(value);
        return castle::status::ok;
    }

private:
    header_transmitter_type header_transmitter_;
    response_transmitter_type response_transmitter_;
    response_provider_type response_provider_;
    response_handler_type response_handler_;
    uint32_t header_count_;
    uint32_t transmitted_response_count_;
    uint32_t received_response_count_;
    uint32_t invalid_count_;
};

/**
 * @brief LIN slave/responder facade that provides published responses and accepts subscriptions.
 *
 * The header adapter calls `handle_header()` when a valid break/sync/PID
 * sequence has been received. If this node publishes that frame ID, the
 * configured provider supplies the complete, checksummed response; otherwise
 * it should return `castle::status::not_found`. The driver then serializes the
 * returned data and checksum on the bus.
 */
template <
    castle::size_type CallbackStorageSize = castle::inplace_storage_reserved,
    castle::size_type CallbackStorageAlignment = castle::inplace_alignment_default>
class slave_node CASTLE_FINAL
{
    static_assert(CallbackStorageSize > 0U,
                  "slave_node callback storage must be non-zero");
    static_assert(CallbackStorageAlignment > 0U
                  && (CallbackStorageAlignment & (CallbackStorageAlignment - 1U)) == 0U,
                  "slave_node callback storage alignment must be a power of two");

public:
    using response_provider_type = castle::callbacks::function<
        castle::status(uint8_t, frame&), CallbackStorageSize, CallbackStorageAlignment>;
    using response_handler_type = castle::callbacks::function<
        void(CASTLE_CONST frame&), CallbackStorageSize, CallbackStorageAlignment>;

    slave_node() CASTLE_NOEXCEPT
        : response_provider_{}, response_handler_{}, prepared_response_count_(0U),
          received_response_count_(0U), invalid_count_(0U)
    {
    }

    /** @brief Installs the provider for response frames published by this slave. */
    void configure_response_provider(response_provider_type&& callback) CASTLE_NOEXCEPT
    {
        response_provider_ = CASTLE_MOVE(callback);
    }

    template <typename Callback, typename = castle::meta::enable_if_t<
        !castle::meta::is_same<castle::meta::decay_t<Callback>, response_provider_type>::value>>
    void configure_response_provider(Callback&& callback)
    {
        response_provider_type wrapper(CASTLE_FORWARD<Callback>(callback));
        response_provider_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Installs a listener for valid responses this node subscribes to. */
    void configure_response_handler(response_handler_type&& callback) CASTLE_NOEXCEPT
    {
        response_handler_ = CASTLE_MOVE(callback);
    }

    template <typename Callback, typename = castle::meta::enable_if_t<
        !castle::meta::is_same<castle::meta::decay_t<Callback>, response_handler_type>::value>>
    void configure_response_handler(Callback&& callback)
    {
        response_handler_type wrapper(CASTLE_FORWARD<Callback>(callback));
        response_handler_ = CASTLE_MOVE(wrapper);
    }

    /** @brief Clears the response provider callback. */
    void clear_response_provider() CASTLE_NOEXCEPT { response_provider_ = response_provider_type{}; }
    /** @brief Clears the receive handler callback. */
    void clear_response_handler() CASTLE_NOEXCEPT { response_handler_ = response_handler_type{}; }

    /** @brief Successfully prepared slave-published responses (modulo 2^32). */
    CASTLE_NODISCARD uint32_t prepared_response_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return prepared_response_count_;
    }
    /** @brief Structurally and checksum-valid received responses (modulo 2^32). */
    CASTLE_NODISCARD uint32_t received_response_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return received_response_count_;
    }
    /** @brief Invalid headers, PIDs, or responses rejected (modulo 2^32). */
    CASTLE_NODISCARD uint32_t invalid_count() CASTLE_CONST CASTLE_NOEXCEPT { return invalid_count_; }

    /** @brief Prepares a published response for a validated header; output changes only on success. */
    castle::status handle_header(CASTLE_CONST header& request, frame& output) CASTLE_NOEXCEPT
    {
        if (!request.valid())
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        if (!response_provider_)
        {
            return castle::status::not_configured;
        }

        frame candidate{};
        const castle::status provided = response_provider_(request.identifier, candidate);
        if (!castle::succeeded(provided))
        {
            return provided;
        }
        if (!candidate.valid() || candidate.identifier != request.identifier
            || !validate_checksum(candidate))
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        output = candidate;
        ++prepared_response_count_;
        return castle::status::ok;
    }

    /** @brief Decodes and handles a received PID byte. */
    castle::status handle_protected_identifier(uint8_t pid, frame& output) CASTLE_NOEXCEPT
    {
        header request{};
        const castle::status decoded = decode_header(pid, request);
        if (!castle::succeeded(decoded))
        {
            ++invalid_count_;
            return decoded;
        }
        return handle_header(request, output);
    }

    /** @brief Validates checksum and dispatches a received response to application code. */
    castle::status receive_response(CASTLE_CONST frame& value) CASTLE_NOEXCEPT
    {
        if (!value.valid() || !validate_checksum(value))
        {
            ++invalid_count_;
            return castle::status::invalid_argument;
        }
        ++received_response_count_;
        if (!response_handler_)
        {
            return castle::status::not_configured;
        }
        response_handler_(value);
        return castle::status::ok;
    }

private:
    response_provider_type response_provider_;
    response_handler_type response_handler_;
    uint32_t prepared_response_count_;
    uint32_t received_response_count_;
    uint32_t invalid_count_;
};

} // namespace lin
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_LIN_NODE_HPP
