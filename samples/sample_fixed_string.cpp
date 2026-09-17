#include "castle/buffers/fixed_string.h"
#include "castle/iterators/iterator.h"

#include <algorithm>
#include <iostream>

using namespace castle::buffers;

int main()
{
    fixed_string<32> fs;
    fs.append("Hello, ");
    fs.append("world!");
    std::cout << fs.c_str() << std::endl;

    // ── forward iteration (character order, excludes trailing NUL) ───────────
    std::cout << "range-for  : ";
    for (char c : fs)
    {
        std::cout << c;
    }
    std::cout << std::endl;

    std::cout << "begin/end  : ";
    for (auto it = fs.begin(); it != fs.end(); ++it)
    {
        std::cout << *it;
    }
    std::cout << std::endl;

    std::cout << "cbegin/cend: ";
    for (auto it = fs.cbegin(); it != fs.cend(); ++it)
    {
        std::cout << *it;
    }
    std::cout << std::endl;

    // ── reverse iteration ────────────────────────────────────────────────────
    std::cout << "rbegin/rend: ";
    for (auto it = fs.rbegin(); it != fs.rend(); ++it)
    {
        std::cout << *it;
    }
    std::cout << std::endl;

    std::cout << "crbegin/end: ";
    for (auto it = fs.crbegin(); it != fs.crend(); ++it)
    {
        std::cout << *it;
    }
    std::cout << std::endl;

    // ── random-access arithmetic ─────────────────────────────────────────────
    auto it = fs.begin();
    std::cout << "\n*begin()        = '" << *it << "'" << std::endl;
    std::cout << "*(begin() + 7)  = '" << *(it + 7) << "'" << std::endl;  // operator+
    std::cout << "begin()[4]      = '" << it[4] << "'" << std::endl;      // operator[]
    std::cout << "*(end() - 1)    = '" << *(fs.end() - 1) << "'" << std::endl;
    std::cout << "end() - begin() = " << (fs.end() - fs.begin()) << std::endl;

    // ── generic iterator algorithms (castle::) ───────────────────────────────
    std::cout << "\ndistance()      = " << castle::distance(fs.begin(), fs.end()) << std::endl;
    std::cout << "*next(begin(),1)= '" << *castle::next(fs.begin(), 1) << "'" << std::endl;
    std::cout << "*prev(end(),1)  = '" << *castle::prev(fs.end(), 1) << "'" << std::endl;

    // ── STL algorithm compatibility (std::) ──────────────────────────────────
    const auto l_count = std::count(fs.begin(), fs.end(), 'l');
    std::cout << "\nstd::count('l') = " << l_count << std::endl;

    const auto found = std::find(fs.begin(), fs.end(), 'w');
    std::cout << "std::find('w')  = ";
    if (found != fs.end())
    {
        std::cout << "found at index " << (found - fs.begin()) << std::endl;
    }
    else
    {
        std::cout << "not found" << std::endl;
    }

    // Mutate through iterators: std algorithms write back into the buffer.
    std::replace(fs.begin(), fs.end(), 'o', '0');
    std::cout << "std::replace    = " << fs.c_str() << std::endl;

    // ── the remaining append/formatting demos ────────────────────────────────
    std::cout << std::endl;

    fs.clear();
    fs.append("This is a very long string that will be truncated.");
    std::cout << fs.c_str() << std::endl;

    fs.clear();
    fs.append(1234567890);
    std::cout << fs.c_str() << std::endl;

    fs.clear();
    fs.append(static_cast<const char*>(nullptr));
    std::cout << fs.c_str() << std::endl;

    fs.clear();
    fs.append("        no whitespace                                     ");
    fs.trim_whitespaces();
    std::cout << "[" << fs.c_str() << "]" << std::endl;

    return 0;
}