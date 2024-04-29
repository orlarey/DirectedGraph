//
//  main.cpp
//  graphlib
//
//  Created by Yann Orlarey on 31/01/2022.
//  Copyright © 2023 Yann Orlarey. All rights reserved.
//

#include <iostream>

#include "tests.hh"

int main(int, const char**)
{
    std::cout << "Tests digraph library\n";

    bool r = true;

    r &= check0();
    r &= check1();
    r &= check2();
    r &= check3();
    r &= check4();
    r &= check5();
    r &= check6();
    // r &= check7();
    r &= check8();
    r &= check9();
    r &= check10();
    r &= check11();
    r &= check12();
    r &= check13();
    r &= check14();
    r &= check15();
    r &= check16();
    r &= check17();
    r &= check18();

    return r ? 0 : 1;
}
