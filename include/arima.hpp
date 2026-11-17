#pragma once
// arima.hpp — ARIMA(1,1,1) implementado desde cero, sin librerias de
// series de tiempo. Se usa el metodo de dos pasos de Hannan-Rissanen
// (1982) para estimar phi (AR) y theta (MA) sin necesidad de montar
// un filtro de Kalman o maximizar una verosimilitud no lineal:
//
//   1. Se diferencia la serie una vez (d=1): w_t = y_t - y_{t-1}.
//   2. Se ajusta un AR(p) largo sobre w por OLS para obtener una
//      aproximacion de los residuales/innovaciones e_t.
//   3. Se regresiona w_t sobre w_{t-1} y e_{t-1} (tambien por OLS)
//      para obtener phi y theta. Sin intercepto, igual que el
//      ARIMA(1,1,1) por defecto de statsmodels (trend='n' cuando d>=1).
//
// Es una aproximacion estandar y bien documentada en la literatura de
// series de tiempo; no es tan exacta como maxima verosimilitud exacta,
// pero es consistente y perfectamente adecuada para una serie mensual
// de ~100 observaciones como esta.

#include <vector>
#include <stdexcept>
#include <cmath>
#include <algorithm>

#include "linalg.hpp"
#include "stats.hpp"

namespace arima {

struct Fit {
    double phi;
    double theta;
    double lastDiff;      // w_n: ultima diferencia observada
    double lastResidual;  // e_n: ultimo residual estimado (paso 2)
    double lastLevel;     // y_n: ultimo valor de la serie en niveles
    double sigma;         // desviacion estandar de los residuales (paso 2)
};

inline std::vector<double> diff(const std::vector<double>& y) {
    std::vector<double> w(y.size() - 1);
    for (size_t t = 1; t < y.size(); ++t) w[t - 1] = y[t] - y[t - 1];
    return w;
}

// Ajusta ARIMA(1,1,1) sobre la serie `y` (en niveles) y devuelve los
// parametros necesarios para pronosticar hacia adelante.
inline Fit fit111(const std::vector<double>& y) {
    if (y.size() < 20)
        throw std::runtime_error("arima::fit111 necesita al menos 20 observaciones");

    std::vector<double> w = diff(y);
    size_t nw = w.size();

    // Paso 1: AR largo para aproximar las innovaciones e_t.
    size_t pLong = std::min<size_t>(8, nw / 4);
    pLong = std::max<size_t>(pLong, 2);

    linalg::Mat X1;
    linalg::Vec y1;
    for (size_t t = pLong; t < nw; ++t) {
        linalg::Vec row;
        row.push_back(1.0); // intercepto (solo para esta regresion auxiliar)
        for (size_t lag = 1; lag <= pLong; ++lag) row.push_back(w[t - lag]);
        X1.push_back(row);
        y1.push_back(w[t]);
    }
    linalg::Vec beta1 = linalg::olsFit(X1, y1);

    // Residuales e_t para t = pLong..nw-1 (alineados con w).
    std::vector<double> e(nw, 0.0);
    for (size_t t = pLong; t < nw; ++t) {
        double pred = beta1[0];
        for (size_t lag = 1; lag <= pLong; ++lag) pred += beta1[lag] * w[t - lag];
        e[t] = w[t] - pred;
    }

    // Paso 2: regresion w_t ~ w_{t-1} + e_{t-1}, sin intercepto.
    linalg::Mat X2;
    linalg::Vec y2;
    for (size_t t = pLong + 1; t < nw; ++t) {
        linalg::Vec row = {w[t - 1], e[t - 1]};
        X2.push_back(row);
        y2.push_back(w[t]);
    }
    linalg::Vec beta2 = linalg::olsFit(X2, y2);
    double phi = beta2[0];
    double theta = beta2[1];

    // Residuales finales del paso 2, para el error estandar y para
    // obtener e_n (necesario como punto de partida del pronostico).
    std::vector<double> resid2;
    resid2.reserve(y2.size());
    double lastResidual = 0.0;
    for (size_t i = 0; i < y2.size(); ++i) {
        double pred = phi * X2[i][0] + theta * X2[i][1];
        double r = y2[i] - pred;
        resid2.push_back(r);
        lastResidual = r; // el ultimo de la lista queda al final del bucle
    }

    Fit fit;
    fit.phi = phi;
    fit.theta = theta;
    fit.lastDiff = w.back();
    fit.lastResidual = lastResidual;
    fit.lastLevel = y.back();
    fit.sigma = stats::stddev(resid2);
    return fit;
}

struct Forecast {
    std::vector<double> levels;
    std::vector<double> ciLower; // aproximacion simple: nivel - 1.96*sigma*sqrt(h)
    std::vector<double> ciUpper;
};

// Pronostica `horizonte` pasos hacia adelante a partir del ajuste `fit`.
// Para h=1 usa phi*w_n + theta*e_n (formula estandar ARMA(1,1)); para
// h>=2 asume que los errores futuros esperados son 0 (lo mismo que
// hace cualquier pronostico ARMA de punto), por lo que el termino MA
// ya no participa y solo queda la recursion AR: w_hat_{t+h} = phi*w_hat_{t+h-1}.
inline Forecast forecast(const Fit& fit, int horizonte) {
    Forecast out;
    out.levels.reserve(static_cast<size_t>(horizonte));
    out.ciLower.reserve(static_cast<size_t>(horizonte));
    out.ciUpper.reserve(static_cast<size_t>(horizonte));

    double wPrev = fit.lastDiff;
    double level = fit.lastLevel;

    for (int h = 1; h <= horizonte; ++h) {
        double wHat = (h == 1) ? (fit.phi * fit.lastDiff + fit.theta * fit.lastResidual)
                                : (fit.phi * wPrev);
        level += wHat;
        wPrev = wHat;

        double banda = 1.96 * fit.sigma * std::sqrt(static_cast<double>(h));
        out.levels.push_back(level);
        out.ciLower.push_back(level - banda);
        out.ciUpper.push_back(level + banda);
    }
    return out;
}

} // namespace arima
