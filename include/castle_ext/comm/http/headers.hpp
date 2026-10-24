// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
#ifndef CASTLE_EXT_COMM_HTTP_HEADERS_HPP
#define CASTLE_EXT_COMM_HTTP_HEADERS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/comm/http/types.hpp"

namespace castle
{
namespace comm
{
namespace http
{

/** @brief Owning, compile-time bounded collection of HTTP fields. */
template <size_type MaxHeaders = 16U,
          size_type MaxNameSize = 64U,
          size_type MaxValueSize = 256U>
class basic_headers
{
public:
    using size_type = castle::size_type;

private:
    struct field
    {
        char name[MaxNameSize + 1U]{};
        char value[MaxValueSize + 1U]{};
        size_type name_size = 0U;
        size_type value_size = 0U;
    };

    static_assert(MaxHeaders > 0U, "HTTP header capacity must be non-zero");
    static_assert(MaxNameSize > 0U, "HTTP header-name capacity must be non-zero");
    static_assert(MaxValueSize > 0U, "HTTP header-value capacity must be non-zero");

public:
    basic_headers() CASTLE_DEFAULT;

    void clear() CASTLE_NOEXCEPT { size_ = 0U; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == MaxHeaders; }

    castle::status add(string_view name, string_view value) CASTLE_NOEXCEPT
    {
        if (!valid_name(name) || !valid_value(value)) { return castle::status::invalid_argument; }
        if (size_ == MaxHeaders) { return castle::status::full; }
        if (name.size() > MaxNameSize || value.size() > MaxValueSize) { return castle::status::out_of_range; }
        copy(fields_[size_], name, value);
        ++size_;
        return castle::status::ok;
    }

    castle::status set(string_view name, string_view value) CASTLE_NOEXCEPT
    {
        if (!valid_name(name) || !valid_value(value)) { return castle::status::invalid_argument; }
        if (name.size() > MaxNameSize || value.size() > MaxValueSize) { return castle::status::out_of_range; }
        for (size_type i = 0U; i < size_; ++i)
        {
            if (ascii_iequal(view_name(fields_[i]), name))
            {
                copy(fields_[i], name, value);
                return castle::status::ok;
            }
        }
        return add(name, value);
    }

    bool remove(string_view name) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size_; ++i)
        {
            if (ascii_iequal(view_name(fields_[i]), name))
            {
                for (size_type j = i + 1U; j < size_; ++j) { fields_[j - 1U] = fields_[j]; }
                --size_;
                return true;
            }
        }
        return false;
    }

    header_view operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= size_) { return header_view{}; }
        return header_view{view_name(fields_[index]), view_value(fields_[index])};
    }

    string_view get(string_view name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size_; ++i)
        {
            if (ascii_iequal(view_name(fields_[i]), name)) { return view_value(fields_[i]); }
        }
        return string_view();
    }

    bool contains(string_view name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < size_; ++i)
        {
            if (ascii_iequal(view_name(fields_[i]), name)) { return true; }
        }
        return false;
    }

private:
    static bool token_char(char c) CASTLE_NOEXCEPT
    {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
               (c >= 'a' && c <= 'z') || c == '!' || c == '#' || c == '$' ||
               c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' ||
               c == '-' || c == '.' || c == '^' || c == '_' || c == '`' ||
               c == '|' || c == '~';
    }

    static bool valid_name(string_view name) CASTLE_NOEXCEPT
    {
        if (name.empty()) { return false; }
        size_type first = 0U;
        if (name[0U] == ':')
        {
            if (name.size() == 1U) { return false; }
            first = 1U; // HTTP/2 pseudo-header field.
        }
        for (size_type i = first; i < name.size(); ++i)
        {
            if (!token_char(name[i])) { return false; }
        }
        return true;
    }

    static bool valid_value(string_view value) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < value.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(value[i]);
            if (c == 0U || c == '\r' || c == '\n' || (c < 32U && c != '\t') || c == 127U) { return false; }
        }
        return true;
    }

    static void copy(field& out, string_view name, string_view value) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < name.size(); ++i) { out.name[i] = name[i]; }
        out.name[name.size()] = '\0';
        for (size_type i = 0U; i < value.size(); ++i) { out.value[i] = value[i]; }
        out.value[value.size()] = '\0';
        out.name_size = name.size();
        out.value_size = value.size();
    }

    static string_view view_name(field const& value) CASTLE_NOEXCEPT
    {
        return string_view(value.name, value.name_size);
    }

    static string_view view_value(field const& value) CASTLE_NOEXCEPT
    {
        return string_view(value.value, value.value_size);
    }

    field fields_[MaxHeaders]{};
    size_type size_ = 0U;
};

template <size_type MaxHeaders, size_type MaxNameSize, size_type MaxValueSize>
using headers = basic_headers<MaxHeaders, MaxNameSize, MaxValueSize>;

} // namespace http
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_HTTP_HEADERS_HPP
