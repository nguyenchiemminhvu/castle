#include "sample_support.hpp"

#include "castle/memory/addressof.hpp"

#include <stdint.h>

struct RegisterRef
{
    RegisterRef* operator&() { return nullptr; }
    RegisterRef const* operator&() const { return nullptr; }

    uint32_t value;
};

int main()
{
    RegisterRef reg{7U};
    RegisterRef const const_reg{9U};

    CASTLE_SAMPLE_CHECK(castle::memory::addressof(reg) != nullptr);
    CASTLE_SAMPLE_CHECK(castle::memory::addressof(reg)->value == 7U);
    CASTLE_SAMPLE_CHECK(castle::memory::addressof(const_reg)->value == 9U);
    CASTLE_SAMPLE_CHECK(castle::memory::addressof(reg) != reg.operator&());
    CASTLE_SAMPLE_CHECK(castle::memory::addressof(const_reg) != const_reg.operator&());
    return 0;
}
