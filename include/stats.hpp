#pragma once
// stats.hpp — funciones estadísticas básicas usadas en limpieza y
// en la evaluación de los modelos de pronóstico.

#include <vector>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace stats {

inline double mean(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    double s = 0.0;
    for (double x : v) s += x;
    return s / static_cast<double>(v.size());
}

inline double median(std::vector<double> v) {
    if (v.empty()) throw std::runtime_error("median() de vector vacío");
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n % 2 == 1) return v[n / 2];
    return (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

inline double stddev(const std::vector<double>& v) {
    if (v.size() < 2) return 0.0;
    double m = mean(v);
    double acc = 0.0;
    for (double x : v) acc += (x - m) * (x - m);
    return std::sqrt(acc / static_cast<double>(v.size() - 1));
}

struct Metrics {
    double mae;
    double rmse;
    double mape; // en porcentaje
};

inline Metrics evaluate(const std::vector<double>& yTrue, const std::vector<double>& yPred) {
    if (yTrue.size() != yPred.size() || yTrue.empty())
        throw std::runtime_error("evaluate(): tamaños incompatibles o vacíos");
    size_t n = yTrue.size();
    double sumAbs = 0.0, sumSq = 0.0, sumPct = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double err = yTrue[i] - yPred[i];
        sumAbs += std::fabs(err);
        sumSq  += err * err;
        sumPct += std::fabs(err / yTrue[i]);
    }
    Metrics m;
    m.mae  = sumAbs / static_cast<double>(n);
    m.rmse = std::sqrt(sumSq / static_cast<double>(n));
    m.mape = (sumPct / static_cast<double>(n)) * 100.0;
    return m;
}

} // namespace stats
