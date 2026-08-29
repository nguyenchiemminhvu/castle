#include "castle/buffers/ring_buffer.h"
#include "castle/iterators/iterator.h"

#include <algorithm>
#include <numeric>
#include <iostream>

using namespace castle::buffers;

// Prints "label: a b c d e" by walking the given iterator range.
template <typename It>
static void print_range(const char* label, It first, It last)
{
    std::cout << label << ':';
    for (It it = first; it != last; ++it)
    {
        std::cout << ' ' << *it;
    }
    std::cout << std::endl;
}

int main()
{
    ring_buffer<int, 5> buffer;
    for (int i = 1; i <= 7; ++i)
    {
        (buffer.push(i * 10)
        ? std::cout << "pushed " << i * 10 << std::endl
        : std::cout << "failed to push " << i * 10 << std::endl);
    }

    // ── forward iteration (logical FIFO order, oldest -> newest) ─────────────
    std::cout << "range-for  :";
    for (int value : buffer)
    {
        std::cout << ' ' << value;
    }
    std::cout << std::endl;

    print_range("begin/end  ", buffer.begin(), buffer.end());
    print_range("cbegin/cend", buffer.cbegin(), buffer.cend());

    // ── reverse iteration (newest -> oldest) ─────────────────────────────────
    print_range("rbegin/rend", buffer.rbegin(), buffer.rend());
    print_range("crbegin/end", buffer.crbegin(), buffer.crend());

    // ── random-access arithmetic ─────────────────────────────────────────────
    auto it = buffer.begin();
    std::cout << "\n*begin()        = " << *it << std::endl;
    std::cout << "*(begin() + 2)  = " << *(it + 2) << std::endl;   // operator+
    std::cout << "2 + begin()     = " << *(2 + it) << std::endl;   // n + iterator
    std::cout << "begin()[3]      = " << it[3] << std::endl;       // operator[]

    it += 4;                                                       // operator+=
    std::cout << "after += 4      = " << *it << std::endl;
    it -= 1;                                                       // operator-=
    std::cout << "after -= 1      = " << *it << std::endl;

    auto last = buffer.end() - 1;                                  // operator-(n)
    std::cout << "*(end() - 1)    = " << *last << std::endl;
    std::cout << "end() - begin() = " << (buffer.end() - buffer.begin()) << std::endl; // difference

    // ── pre/post increment and decrement ─────────────────────────────────────
    auto walk = buffer.begin();
    std::cout << "\npost-increment  : " << *walk++;
    std::cout << " then " << *walk << std::endl;
    std::cout << "pre-decrement   : " << *--walk << std::endl;

    // ── comparisons ──────────────────────────────────────────────────────────
    std::cout << "\nbegin() <  end()  : " << std::boolalpha << (buffer.begin() < buffer.end()) << std::endl;
    std::cout << "end()   >  begin(): " << (buffer.end() > buffer.begin()) << std::endl;
    std::cout << "begin() == begin(): " << (buffer.begin() == buffer.begin()) << std::endl;
    std::cout << "begin() != end()  : " << (buffer.begin() != buffer.end()) << std::endl;

    // ── generic iterator algorithms (castle::) ───────────────────────────────
    std::cout << "\ndistance()      = " << castle::distance(buffer.begin(), buffer.end()) << std::endl;
    std::cout << "*next(begin(),2)= " << *castle::next(buffer.begin(), 2) << std::endl;
    std::cout << "*prev(end(),1)  = " << *castle::prev(buffer.end(), 1) << std::endl;

    auto adv = buffer.begin();
    castle::advance(adv, 3);
    std::cout << "*advance(it,3)  = " << *adv << std::endl;

    // ── STL algorithm compatibility (std::) ──────────────────────────────────
    const int sum = std::accumulate(buffer.begin(), buffer.end(), 0);
    std::cout << "\nstd::accumulate = " << sum << std::endl;

    const auto found = std::find(buffer.begin(), buffer.end(), 30);
    std::cout << "std::find(30)   = ";
    if (found != buffer.end())
    {
        std::cout << "found at index " << (found - buffer.begin()) << std::endl;
    }
    else
    {
        std::cout << "not found" << std::endl;
    }

    const auto max_it = std::max_element(buffer.begin(), buffer.end());
    std::cout << "std::max_element= " << *max_it << std::endl;

    return 0;
}