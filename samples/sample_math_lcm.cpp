#include "castle/math/lcm.h"

#include <iostream>

using namespace castle::math;

int main()
{
    int a = 12;
    int b = 18;
    int result = lcm(a, b);
    std::cout << "LCM of " << a << " and " << b << " is " << result << std::endl;

    lcm_v<12, 18> compile_time_result;
    std::cout << "Compile-time LCM of 12 and 18 is " << compile_time_result.value << std::endl;

    return 0;
}