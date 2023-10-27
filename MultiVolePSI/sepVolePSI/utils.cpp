#include <iostream>
#include "Defines.h"

using oc::u64;

void print_debug(u64 idx, std::string scope, std::string content)
{
    std::cout << "[" << idx << ":" << scope << "] " << content << std::endl;
}