#include "castle/math/gcd.h"

#include <iostream>

using namespace castle::math;

int main()
{
    int a = 48;
    int b = 18;
    int result = gcd(a, b);
    std::cout << "GCD of " << a << " and " << b << " is " << result << std::endl;

    gcd_v<48, 18> compile_time_result;
    std::cout << "Compile-time GCD of 48 and 18 is " << compile_time_result.value << std::endl;

    return 0;
}