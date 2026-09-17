#include "sample_support.hpp"

#include "castle/core/compiler.hpp"

namespace
{

CASTLE_NODISCARD CASTLE_CONSTEXPR int answer() CASTLE_NOEXCEPT
{
    return 42;
}

template <typename T>
CASTLE_CONSTEXPR int classify() CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (sizeof(T) == sizeof(int))
    {
        return 1;
    }
    return 0;
}

struct Base
{
    virtual ~Base() CASTLE_DEFAULT;
    virtual int value() const CASTLE_NOEXCEPT = 0;
};

struct Derived CASTLE_FINAL : Base
{
    CASTLE_MUTABLE int cache;

    CASTLE_CONSTEXPR Derived() CASTLE_NOEXCEPT
        : cache(0)
    {
    }

    int value() const CASTLE_NOEXCEPT CASTLE_OVERRIDE
    {
        cache = answer();
        return cache;
    }
};

} // namespace

int main()
{
    CASTLE_UNUSED int unused = 0;
    Derived instance;
    CASTLE_SAMPLE_CHECK(answer() == 42);
    CASTLE_SAMPLE_CHECK(classify<int>() == 1);
    CASTLE_SAMPLE_CHECK(instance.value() == 42);
    CASTLE_SAMPLE_CHECK(instance.cache == 42);
    CASTLE_SAMPLE_CHECK(unused == 0);
    return 0;
}
