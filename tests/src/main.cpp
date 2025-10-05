#include <iostream>

int main() {
    std::cout << "Hello from tests!" << std::endl;

    // Quick sanity for C++20 compilation (consteval/constexpr and string views)
#if __cplusplus >= 202002L
    constexpr int answer = []{ return 42; }();
    std::cout << "C++ standard: C++20 or later. Answer=" << answer << std::endl;
#else
    std::cout << "C++ standard: pre-C++20" << std::endl;
#endif

    // Basic return code to test CI/run task wiring
    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}
