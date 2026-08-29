#include "castle/types/safe_cast.h"

#include <iostream>

using namespace castle::types;

int main()
{
    int32_t int_value = 123456;
    float float_value = SAFE_CAST<int32_t, float>(int_value);
    std::cout << "int_value: " << int_value << ", float_value: "<< float_value << std::endl;

    bool bool_value = SAFE_CAST<int32_t, bool>(int_value);
    std::cout << "int_value: " << int_value << ", bool_value: "<< bool_value << std::endl;

    uint16_t uint16_value = SAFE_CAST<int32_t, uint16_t>(int_value);
    std::cout << "int_value: " << int_value << ", uint16_value: "<< uint16_value << std::endl;

    return 0;
}