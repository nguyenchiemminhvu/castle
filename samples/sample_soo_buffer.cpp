#include "castle/buffers/soo_buffer.h"

#include <iostream>

using namespace castle::buffers;

struct small_context
{
    char data[16];
};

struct large_context
{
    char data[128];
};

int main()
{
    soo_buffer<int> int_buffer(42);
    std::cout << "Value: " << *int_buffer.get() << std::endl;

    soo_buffer<small_context> small_buffer(small_context{});
    std::cout << "Small buffer is heap allocated: " << small_buffer.is_heap() << std::endl;

    // soo_buffer<large_context> large_buffer(large_context{});
    // std::cout << "Large buffer is heap allocated: " << large_buffer.is_heap() << std::endl; // cause a warning

    // soo_buffer<large_context, 32, false> large_buffer_no_heap(large_context{}); // This will cause a compile-time error

    return 0;
}