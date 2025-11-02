#ifndef TEST_MAIN_H
#define TEST_MAIN_H

#include <Arduino.h>

// Test function type
using TestFn = void(*)();

// Test case structure
struct TestCase {
    const char* name;
    TestFn fn;
    uint16_t line;
};

#define TEST_ENTRY(fn) { #fn, fn, __LINE__ }

// Test function declarations
void test_basic_wifimanager_instantiation();

#endif // TEST_MAIN_H

