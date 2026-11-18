#include "sample_support.hpp"

#include "castle/callbacks/delegate.hpp"

#include <stdint.h>

namespace
{
int add_pair(int lhs, int rhs) noexcept
{
    return lhs + rhs;
}

void add_to_total(uint32_t value) noexcept;

struct multiplier
{
    int factor = 1;

    int operator()(int value, int bias) const noexcept
    {
        return (value * factor) + bias;
    }
};

struct accumulator
{
    int total = 0;

    void add(int value) noexcept
    {
        total += value;
    }

    int read(int value) const noexcept
    {
        return total + value;
    }
};

accumulator g_accumulator{};
uint32_t g_total = 0U;

void add_to_total(uint32_t value) noexcept
{
    g_total += value;
}
} // namespace

int main()
{
    castle::callbacks::delegate_ptr<int(int, int)> free_delegate{&add_pair};
    CASTLE_SAMPLE_CHECK(free_delegate(2, 3) == 5);

    auto owning_functor =
        castle::callbacks::make_delegate_ft<int(int, int)>(
            [offset = 4](int lhs, int rhs) noexcept
            {
                return lhs - rhs + offset;
            });
    CASTLE_SAMPLE_CHECK(owning_functor(9, 2) == 11);

    multiplier functor{3};
    castle::callbacks::delegate_ft<multiplier, int(int, int)> owning_named_functor{functor};
    CASTLE_SAMPLE_CHECK(owning_named_functor(4, 1) == 13);

    castle::callbacks::delegate_ftr<multiplier, int(int, int)> referenced_functor{functor};
    CASTLE_SAMPLE_CHECK(referenced_functor(5, 2) == 17);

    auto deduced_reference = castle::callbacks::make_delegate_ftr<int(int, int)>(functor);
    CASTLE_SAMPLE_CHECK(deduced_reference(2, 1) == 7);

    accumulator local{};
    castle::callbacks::delegate_member<accumulator, void(int)> member_delegate{local, &accumulator::add};
    member_delegate(6);
    CASTLE_SAMPLE_CHECK(local.total == 6);

    castle::callbacks::delegate_ptr<void(uint32_t)> void_delegate{&add_to_total};
    void_delegate(3U);
    CASTLE_SAMPLE_CHECK(g_total == 3U);

    castle::callbacks::delegate_ptr_ct<&add_pair> compile_time_free{};
    CASTLE_SAMPLE_CHECK(compile_time_free(7, 8) == 15);

    castle::callbacks::delegate_ft_ct<multiplier, int(int, int)> compile_time_functor{};
    CASTLE_SAMPLE_CHECK(compile_time_functor(3, 1) == 4);

    castle::callbacks::delegate_member_ct<&accumulator::add> compile_time_member{local};
    compile_time_member(5);
    CASTLE_SAMPLE_CHECK(local.total == 11);

    const accumulator const_local{9};
    castle::callbacks::delegate_member_ct<&accumulator::read> compile_time_const_member{const_local};
    CASTLE_SAMPLE_CHECK(compile_time_const_member(4) == 13);

    castle::callbacks::delegate_ins_ct<g_accumulator, &accumulator::add> compile_time_instance{};
    compile_time_instance(10);
    CASTLE_SAMPLE_CHECK(g_accumulator.total == 10);

    castle::callbacks::delegate_ins_ct<g_accumulator, &accumulator::read> compile_time_const_instance{};
    CASTLE_SAMPLE_CHECK(compile_time_const_instance(5) == 15);

    return 0;
}
