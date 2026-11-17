#include <exception>
#include <iostream>

#include "test_framework.hpp"

int main() {
    int passed = 0;
    int failed = 0;

    for (const auto& test : testfw::registry()) {
        try {
            test.body();
            ++passed;
            std::cout << "[PASS] " << test.name << "\n";
        } catch (const std::exception& error) {
            ++failed;
            std::cout << "[FAIL] " << test.name << "\n       " << error.what() << "\n";
        } catch (...) {
            ++failed;
            std::cout << "[FAIL] " << test.name << "\n       error desconocido\n";
        }
    }

    std::cout << "\n" << passed << "/" << (passed + failed)
              << " casos aprobados (" << testfw::assertions() << " aserciones)\n";
    return failed == 0 ? 0 : 1;
}
