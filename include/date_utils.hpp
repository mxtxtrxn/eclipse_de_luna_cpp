#pragma once
// date_utils.hpp — aritmética de fechas civiles (calendario gregoriano)
// usando el algoritmo de Howard Hinnant (días desde 1970-01-01),
// de dominio público: https://howardhinnant.github.io/date_algorithms.html
// Se usa en vez de std::chrono::year_month_day porque esa API de
// calendario llegó hasta C++20 y este proyecto se compila en C++17.

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>

namespace dateutil {

struct Date {
    int year;
    int month; // 1-12
    int day;   // 1-31
};

inline int64_t daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int64_t>(doe) - 719468;
}

inline Date civilFromDays(int64_t z) {
    z += 719468;
    const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const int64_t y = static_cast<int64_t>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m = mp + (mp < 10 ? 3 : -9);
    Date out;
    out.year  = static_cast<int>(y + (m <= 2));
    out.month = static_cast<int>(m);
    out.day   = static_cast<int>(d);
    return out;
}

inline Date addDays(const Date& base, int64_t days) {
    int64_t z = daysFromCivil(base.year, static_cast<unsigned>(base.month),
                               static_cast<unsigned>(base.day));
    return civilFromDays(z + days);
}

inline Date addMonths(const Date& base, int months) {
    int totalMonth0 = (base.year * 12 + (base.month - 1)) + months;
    int y = totalMonth0 / 12;
    int m = totalMonth0 % 12;
    if (m < 0) { m += 12; y -= 1; }
    Date out;
    out.year = y;
    out.month = m + 1;
    out.day = base.day;
    return out;
}

inline std::string toIsoString(const Date& d) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << d.year << "-"
        << std::setfill('0') << std::setw(2) << d.month << "-"
        << std::setfill('0') << std::setw(2) << d.day;
    return oss.str();
}

} // namespace dateutil
