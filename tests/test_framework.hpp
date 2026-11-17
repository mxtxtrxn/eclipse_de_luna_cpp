#pragma once
// test_framework.hpp — arnes de pruebas minimo, sin dependencias externas.
// Cada TEST_CASE se registra automaticamente y test_main.cpp los ejecuta.

#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace testfw {

struct Failure : std::runtime_error {
    explicit Failure(const std::string& message) : std::runtime_error(message) {}
};

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> body) {
        registry().push_back({std::move(name), std::move(body)});
    }
};

inline int& assertions() {
    static int total = 0;
    return total;
}

[[noreturn]] inline void reportFailure(const char* file, int line,
                                       const std::string& detail) {
    std::ostringstream oss;
    oss << file << ":" << line << ": " << detail;
    throw Failure(oss.str());
}

template <typename A, typename B>
void checkEqual(const A& actual, const B& expected, const char* exprA,
                const char* exprB, const char* file, int line) {
    ++assertions();
    if (!(actual == expected)) {
        std::ostringstream oss;
        oss << exprA << " == " << exprB << " (obtenido: " << actual
            << ", esperado: " << expected << ")";
        reportFailure(file, line, oss.str());
    }
}

template <typename A, typename B>
void checkNotEqual(const A& actual, const B& expected, const char* exprA,
                   const char* exprB, const char* file, int line) {
    ++assertions();
    if (actual == expected) {
        std::ostringstream oss;
        oss << exprA << " != " << exprB << " (ambos valen: " << actual << ")";
        reportFailure(file, line, oss.str());
    }
}

inline void checkNear(double actual, double expected, double tolerance,
                      const char* exprA, const char* exprB,
                      const char* file, int line) {
    ++assertions();
    if (!(std::fabs(actual - expected) <= tolerance)) {
        std::ostringstream oss;
        oss << exprA << " ~= " << exprB << " (obtenido: " << actual
            << ", esperado: " << expected << ", tolerancia: " << tolerance << ")";
        reportFailure(file, line, oss.str());
    }
}

template <typename Fn>
void checkThrows(Fn&& fn, const char* expression, const char* file, int line) {
    ++assertions();
    bool thrown = false;
    try {
        fn();
    } catch (...) {
        thrown = true;
    }
    if (!thrown) {
        reportFailure(file, line,
                      std::string("se esperaba una excepcion: ") + expression);
    }
}

} // namespace testfw

#define TEST_CASE(name)                                              \
    static void name();                                              \
    static ::testfw::Registrar registrar_##name(#name, name);        \
    static void name()

#define CHECK(condition)                                             \
    do {                                                             \
        ++::testfw::assertions();                                    \
        if (!(condition)) {                                          \
            ::testfw::reportFailure(__FILE__, __LINE__,              \
                std::string("condicion falsa: ") + #condition);      \
        }                                                            \
    } while (0)

#define CHECK_EQ(actual, expected)                                   \
    ::testfw::checkEqual((actual), (expected), #actual, #expected,   \
                         __FILE__, __LINE__)

#define CHECK_NE(actual, expected)                                   \
    ::testfw::checkNotEqual((actual), (expected), #actual, #expected, \
                            __FILE__, __LINE__)

#define CHECK_NEAR(actual, expected, tolerance)                      \
    ::testfw::checkNear((actual), (expected), (tolerance), #actual,  \
                        #expected, __FILE__, __LINE__)

#define CHECK_THROWS(expression)                                     \
    ::testfw::checkThrows([&]() { (void)(expression); },             \
                          #expression, __FILE__, __LINE__)
