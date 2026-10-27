// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Fixed-capacity DNS record, response and resolver configuration types.
 */
#ifndef CASTLE_EXT_PROTOCOLS_DNS_TYPES_HPP
#define CASTLE_EXT_PROTOCOLS_DNS_TYPES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"
#include "castle_ext/net/endpoint.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace dns
{

/** @brief DNS resource-record query types supported by CASTLE's first DNS client. */
enum class record_type : uint16_t
{
    a = 1U,
    ns = 2U,
    cname = 5U,
    mx = 15U,
    aaaa = 28U
};

/** @brief DNS classes. The initial client sends and accepts Internet-class records. */
enum class record_class : uint16_t
{
    internet = 1U
};

/** @brief Resource-record section in which a record appeared. */
enum class section : uint8_t
{
    answer = 0U,
    authority = 1U,
    additional = 2U
};

/** @brief RFC 1035 response-code values. */
enum class response_code : uint8_t
{
    no_error = 0U,
    format_error = 1U,
    server_failure = 2U,
    name_error = 3U,
    not_implemented = 4U,
    refused = 5U
};

/** @brief Decoded DNS resource record. Names are null-terminated and limited to 255 octets. */
struct resource_record
{
    record_type type = record_type::a;
    record_class klass = record_class::internet;
    section source_section = section::answer;
    uint32_t ttl = 0U;
    net::endpoint address{};     /**< Populated for A and AAAA; port is zero. */
    char owner[256U]{};          /**< RR owner name in presentation form. */
    char target[256U]{};         /**< CNAME, NS, or MX exchange name. */
    uint16_t preference = 0U;    /**< MX preference; zero for other types. */
};

/**
 * @brief Fixed-capacity parsed DNS response.
 * @tparam MaxRecords Maximum supported records retained across answer, authority and additional sections.
 */
template <size_type MaxRecords = 8U>
struct response
{
    static_assert(MaxRecords > 0U, "DNS response must retain at least one resource record");

    uint16_t transaction_id = 0U;
    uint16_t flags = 0U;
    uint16_t question_count = 0U;
    uint16_t answer_count = 0U;
    uint16_t authority_count = 0U;
    uint16_t additional_count = 0U;
    record_type question_type = record_type::a;
    record_class question_class = record_class::internet;
    response_code rcode = response_code::no_error;
    bool authoritative = false;
    bool truncated = false;
    bool recursion_desired = false;
    bool recursion_available = false;
    char question_name[256U]{};
    castle::container::array<resource_record, MaxRecords> records{};
    size_type record_count = 0U;

    /** @brief Clears the decoded response without allocating or changing its capacity. */
    void clear() CASTLE_NOEXCEPT
    {
        transaction_id = 0U;
        flags = 0U;
        question_count = 0U;
        answer_count = 0U;
        authority_count = 0U;
        additional_count = 0U;
        question_type = record_type::a;
        question_class = record_class::internet;
        rcode = response_code::no_error;
        authoritative = false;
        truncated = false;
        recursion_desired = false;
        recursion_available = false;
        question_name[0U] = '\0';
        record_count = 0U;
    }
};

/** @brief Timeout and retry policy shared by synchronous and polled resolution. */
struct client_config
{
    uint32_t timeout_ms = 1000U;       /**< Per UDP attempt and per TCP readiness wait. */
    uint8_t retries = 2U;              /**< Retransmissions after the first UDP attempt. */
    uint16_t transaction_id_seed = 0xCA57U;
};

} // namespace dns
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_DNS_TYPES_HPP
