#include "alloc.h"
#include <vector>
#include <iostream>

int main()
{
    std::vector<int, __default_alloc_template<int>> vec;
    for (int i = 0; i < 100; i++)
    {
        vec.push_back(i);
    }
    for (int val : vec) {
        std::cout << val <<"    " << std::endl;
    }
    return 0;
}