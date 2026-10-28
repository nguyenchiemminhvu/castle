// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/** @file topic.hpp @brief MQTT topic-name validation and topic-filter matching. */
#ifndef CASTLE_EXT_PROTOCOLS_MQTT_TOPIC_HPP
#define CASTLE_EXT_PROTOCOLS_MQTT_TOPIC_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace mqtt
{

namespace detail
{
CASTLE_INLINE bool valid_utf8(castle::container::string_view value) CASTLE_NOEXCEPT
{
    if (value.data() == nullptr && !value.empty())
    {
        return false;
    }
    size_type i = 0U;
    while (i < value.size())
    {
        uint32_t cp = static_cast<uint8_t>(value[i++]);
        if (cp == 0U || cp == 0xFFFFU || cp == 0xFFFEU) // LCOV_EXCL_BR_LINE
        {
            return false;
        }
        if (cp < 0x80U)
        {
            continue;
        }
        uint32_t minimum = 0x80U;
        size_type continuation = 0U;
        if ((cp & 0xE0U) == 0xC0U)
        {
            cp &= 0x1FU;
            continuation = 1U;
            minimum = 0x80U;
        }
        else if ((cp & 0xF0U) == 0xE0U)
        {
            cp &= 0x0FU;
            continuation = 2U;
            minimum = 0x800U;
        }
        else if ((cp & 0xF8U) == 0xF0U)
        {
            cp &= 0x07U;
            continuation = 3U;
            minimum = 0x10000U;
        }
        else
        {
            return false;
        }
        if (i + continuation > value.size())
        {
            return false;
        }
        for (size_type j = 0U; j < continuation; ++j)
        {
            uint8_t next = static_cast<uint8_t>(value[i++]);
            if ((next & 0xC0U) != 0x80U)
            {
                return false;
            }
            cp = (cp << 6U) | (next & 0x3FU);
        }
        if (cp < minimum || cp > 0x10FFFFU || (cp >= 0xD800U && cp <= 0xDFFFU) ||
            (cp >= 0xFDD0U && cp <= 0xFDEFU) || (cp & 0xFFFFU) == 0xFFFEU || (cp & 0xFFFFU) == 0xFFFFU)
        {
            return false;
        }
    }
    return true;
}
} // namespace detail

/** @brief Validates an MQTT Topic Name (non-empty UTF-8; wildcards forbidden). */
CASTLE_INLINE castle::status validate_topic_name(castle::container::string_view topic) CASTLE_NOEXCEPT
{
    if (topic.empty() || !detail::valid_utf8(topic))
    {
        return castle::status::invalid_argument;
    }
    for (size_type i = 0U; i < topic.size(); ++i)
    {
        if (topic[i] == '+' || topic[i] == '#')
        {
            return castle::status::invalid_argument;
        }
    }
    return castle::status::ok;
}

/** @brief Validates an MQTT Topic Filter, including whole-level '+' and final-level '#'. */
CASTLE_INLINE castle::status validate_topic_filter(castle::container::string_view filter) CASTLE_NOEXCEPT
{
    if (filter.empty() || !detail::valid_utf8(filter))
    {
        return castle::status::invalid_argument;
    }
    size_type level_start = 0U;
    for (size_type i = 0U; i <= filter.size(); ++i)
    {
        if (i == filter.size() || filter[i] == '/')
        {
            size_type level_size = i - level_start;
            for (size_type j = level_start; j < i; ++j)
            {
                if (filter[j] == '+' && !(level_size == 1U))
                {
                    return castle::status::invalid_argument;
                }
                if (filter[j] == '#' && !(level_size == 1U && i == filter.size()))
                {
                    return castle::status::invalid_argument;
                }
            }
            level_start = i + 1U;
        }
    }
    return castle::status::ok;
}

/** @brief Tests whether a Topic Name is selected by a valid MQTT Topic Filter. */
CASTLE_INLINE bool topic_matches(castle::container::string_view filter,
                                 castle::container::string_view topic) CASTLE_NOEXCEPT
{
    if (!castle::succeeded(validate_topic_filter(filter)) ||
        !castle::succeeded(validate_topic_name(topic)))
    {
        return false;
    }
    // Per MQTT, wildcard filters that do not start with '$' never match '$' system topics.
    if (topic[0U] == '$' && filter[0U] != '$')
    {
        return false;
    }

    size_type fi = 0U;
    size_type ti = 0U;
    while (fi < filter.size())
    {
        if (filter[fi] == '#')
        {
            return true;
        }
        size_type f_end = fi;
        while (f_end < filter.size() && filter[f_end] != '/')
        {
            ++f_end;
        }
        size_type t_end = ti;
        while (t_end < topic.size() && topic[t_end] != '/')
        {
            ++t_end;
        }
        size_type f_len = f_end - fi;
        size_type t_len = t_end - ti;
        bool plus = f_len == 1U && filter[fi] == '+';
        if (!plus)
        {
            if (f_len != t_len)
            {
                return false;
            }
            for (size_type k = 0U; k < f_len; ++k)
            {
                if (filter[fi + k] != topic[ti + k])
                {
                    return false;
                }
            }
        }
        bool f_more = f_end < filter.size();
        bool t_more = t_end < topic.size();
        if (!f_more)
        {
            return !t_more;
        }
        if (!t_more)
        {
            return (f_end + 2U == filter.size() && filter[f_end + 1U] == '#');
        }
        fi = f_end + 1U;
        ti = t_end + 1U;
    }
    return ti == topic.size();
}

} // namespace mqtt
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_MQTT_TOPIC_HPP
