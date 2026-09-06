#ifndef CASTLE_CONTAINER_STRING_H
#define CASTLE_CONTAINER_STRING_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/error/status.h"
#include "castle/container/string_view.h"

namespace castle
{
namespace container
{

// Fixed-capacity owning string. Capacity excludes the terminating null.
template <typename CharT, size_type N>
class basic_string
{
    static_assert(N > 0U, "basic_string requires a positive capacity");

public:
    using value_type = CharT;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = CharT&;
    using const_reference = CASTLE_CONST CharT&;
    using pointer = CharT*;
    using const_pointer = CASTLE_CONST CharT*;
    using iterator = CharT*;
    using const_iterator = CASTLE_CONST CharT*;
    using view_type = basic_string_view<CharT>;

    static CASTLE_CONSTEXPR size_type static_capacity = N;

    basic_string() CASTLE_NOEXCEPT
        : size_(0U)
    {
        data_[0U] = CharT();
    }

    basic_string(const_pointer text) CASTLE_NOEXCEPT
        : size_(0U)
    {
        data_[0U] = CharT();
        if (assign(text) != status::ok)
        {
            clear();
        }
    }

    basic_string(view_type view) CASTLE_NOEXCEPT
        : size_(0U)
    {
        data_[0U] = CharT();
        if (assign(view) != status::ok)
        {
            clear();
        }
    }

    basic_string(CASTLE_CONST basic_string& other) CASTLE_NOEXCEPT
        : size_(other.size_)
    {
        copy_chars(data_, other.data_, size_);
    }

    basic_string& operator=(CASTLE_CONST basic_string& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            size_ = other.size_;
            copy_chars(data_, other.data_, size_);
        }
        return *this;
    }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    size_type length() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    iterator begin() CASTLE_NOEXCEPT { return data_; }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    iterator end() CASTLE_NOEXCEPT { return data_ + size_; }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + size_; }
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + size_; }

    pointer data() CASTLE_NOEXCEPT { return data_; }
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    const_pointer c_str() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    reference operator[](size_type index) CASTLE_NOEXCEPT { return data_[index]; }
    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }
    reference front() CASTLE_NOEXCEPT { return data_[0U]; }
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }
    reference back() CASTLE_NOEXCEPT { return data_[size_ - 1U]; }
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[size_ - 1U]; }

    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        data_[0U] = CharT();
    }

    status assign(const_pointer text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            clear();
            return status::invalid_argument;
        }
        size_type length = 0U;
        while (text[length] != CharT())
        {
            ++length;
            if (length > N)
            {
                return status::full;
            }
        }
        copy_chars(data_, text, length);
        size_ = length;
        data_[size_] = CharT();
        return status::ok;
    }

    status assign(view_type view) CASTLE_NOEXCEPT
    {
        if (view.size() > N)
        {
            return status::full;
        }
        copy_chars(data_, view.data(), view.size());
        size_ = view.size();
        data_[size_] = CharT();
        return status::ok;
    }

    status push_back(CharT value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }
        data_[size_++] = value;
        data_[size_] = CharT();
        return status::ok;
    }

    status pop_back() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        --size_;
        data_[size_] = CharT();
        return status::ok;
    }

    status append(const_pointer text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            return status::invalid_argument;
        }
        CASTLE_CONST size_type old_size = size_;
        size_type length = 0U;
        while (text[length] != CharT())
        {
            ++length;
            if (length > N - old_size)
            {
                return status::full;
            }
        }
        copy_chars(data_ + old_size, text, length);
        size_ = old_size + length;
        data_[size_] = CharT();
        return status::ok;
    }

    status append(view_type view) CASTLE_NOEXCEPT
    {
        if (view.size() > N - size_)
        {
            return status::full;
        }
        copy_chars(data_ + size_, view.data(), view.size());
        size_ += view.size();
        data_[size_] = CharT();
        return status::ok;
    }

    status append(CASTLE_CONST basic_string& other) CASTLE_NOEXCEPT
    {
        return append(view_type(other.data_, other.size_));
    }

    status insert(size_type position, view_type view) CASTLE_NOEXCEPT
    {
        if (position > size_)
        {
            return status::out_of_range;
        }
        if (view.size() > N - size_)
        {
            return status::full;
        }
        for (size_type i = size_; i > position; --i)
        {
            data_[i + view.size() - 1U] = data_[i - 1U];
        }
        for (size_type i = 0U; i < view.size(); ++i)
        {
            data_[position + i] = view[i];
        }
        size_ += view.size();
        data_[size_] = CharT();
        return status::ok;
    }

    status erase(size_type position, size_type count = static_cast<size_type>(-1)) CASTLE_NOEXCEPT
    {
        if (position > size_)
        {
            return status::out_of_range;
        }
        if (count > size_ - position)
        {
            count = size_ - position;
        }
        for (size_type i = position; i + count < size_; ++i)
        {
            data_[i] = data_[i + count];
        }
        size_ -= count;
        data_[size_] = CharT();
        return status::ok;
    }

    status resize(size_type count, CharT value = CharT()) CASTLE_NOEXCEPT
    {
        if (count > N)
        {
            return status::full;
        }
        if (count > size_)
        {
            for (size_type i = size_; i < count; ++i)
            {
                data_[i] = value;
            }
        }
        size_ = count;
        data_[size_] = CharT();
        return status::ok;
    }

    size_type find(CharT value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_).find(value, offset);
    }

    size_type find(view_type value, size_type offset = 0U) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_).find(value, offset);
    }

    int compare(view_type other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_).compare(other);
    }

    view_type view() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return view_type(data_, size_);
    }

private:
    static void copy_chars(pointer dst, const_pointer src, size_type count) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < count; ++i)
        {
            dst[i] = src[i];
        }
    }

    CharT data_[N + 1U];
    size_type size_;
};

using string = basic_string<char, 128U>;

using u8string = basic_string<char, 128U>;

template <typename CharT, size_type N>
bool operator==(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs.view()) == 0;
}

template <typename CharT, size_type N>
bool operator!=(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename CharT, size_type N>
bool operator==(CASTLE_CONST basic_string<CharT, N>& lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs) == 0;
}

template <typename CharT, size_type N>
bool operator==(basic_string_view<CharT> lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return rhs == lhs;
}

template <typename CharT, size_type N>
bool operator!=(CASTLE_CONST basic_string<CharT, N>& lhs, basic_string_view<CharT> rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename CharT, size_type N>
bool operator!=(basic_string_view<CharT> lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename CharT, size_type N>
bool operator<(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return lhs.compare(rhs.view()) < 0;
}

template <typename CharT, size_type N>
bool operator>(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

template <typename CharT, size_type N>
bool operator<=(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

template <typename CharT, size_type N>
bool operator>=(CASTLE_CONST basic_string<CharT, N>& lhs, CASTLE_CONST basic_string<CharT, N>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_STRING_H
