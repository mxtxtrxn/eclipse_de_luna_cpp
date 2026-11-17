#include <cmath>
#include <vector>

#include "arima.hpp"
#include "test_framework.hpp"

namespace {

std::vector<double> serieSenoidal(std::size_t n) {
    std::vector<double> y(n);
    for (std::size_t i = 0; i < n; ++i) {
        y[i] = 10.0 + 0.01 * static_cast<double>(i) +
               std::sin(static_cast<double>(i) / 3.0);
    }
    return y;
}

} // namespace

TEST_CASE(arima_diff_calcula_diferencias) {
    const std::vector<double> w = arima::diff({1.0, 3.0, 6.0});
    CHECK_EQ(w.size(), std::size_t{2});
    CHECK_NEAR(w[0], 2.0, 1e-12);
    CHECK_NEAR(w[1], 3.0, 1e-12);
}

TEST_CASE(arima_fit_necesita_veinte_observaciones) {
    CHECK_THROWS(arima::fit111(std::vector<double>(19, 1.0)));
}

TEST_CASE(arima_fit_serie_constante_es_finita) {
    const std::vector<double> y(30, 5.0);
    const arima::Fit fit = arima::fit111(y);
    CHECK(std::isfinite(fit.phi));
    CHECK(std::isfinite(fit.theta));
    CHECK(std::isfinite(fit.sigma));
    CHECK(fit.sigma >= 0.0);
    CHECK_NEAR(fit.lastLevel, 5.0, 1e-12);
}

TEST_CASE(arima_fit_serie_senoidal_es_finita) {
    const arima::Fit fit = arima::fit111(serieSenoidal(80));
    CHECK(std::isfinite(fit.phi));
    CHECK(std::isfinite(fit.theta));
    CHECK(std::isfinite(fit.sigma));
    CHECK(fit.sigma > 0.0);
}

TEST_CASE(arima_forecast_primer_paso_usa_phi_y_theta) {
    arima::Fit fit;
    fit.phi = 0.5;
    fit.theta = -0.25;
    fit.lastDiff = 0.4;
    fit.lastResidual = 0.1;
    fit.lastLevel = 10.0;
    fit.sigma = 0.2;

    const arima::Forecast forecast = arima::forecast(fit, 3);
    CHECK_EQ(forecast.levels.size(), std::size_t{3});
    CHECK_NEAR(forecast.levels[0], 10.0 + 0.5 * 0.4 - 0.25 * 0.1, 1e-12);
    CHECK_NEAR(forecast.ciLower[0], forecast.levels[0] - 1.96 * 0.2, 1e-12);
    CHECK_NEAR(forecast.ciUpper[0], forecast.levels[0] + 1.96 * 0.2, 1e-12);
}

TEST_CASE(arima_forecast_banda_crece_con_el_horizonte) {
    arima::Fit fit;
    fit.phi = 0.3;
    fit.theta = 0.1;
    fit.lastDiff = 0.2;
    fit.lastResidual = 0.0;
    fit.lastLevel = 20.0;
    fit.sigma = 0.5;

    const arima::Forecast forecast = arima::forecast(fit, 6);
    const double ancho1 = forecast.ciUpper[0] - forecast.ciLower[0];
    const double ancho6 = forecast.ciUpper[5] - forecast.ciLower[5];
    CHECK(ancho6 > ancho1);
}

TEST_CASE(arima_forecast_horizonte_cero_vacio) {
    const arima::Fit fit{};
    const arima::Forecast forecast = arima::forecast(fit, 0);
    CHECK(forecast.levels.empty());
    CHECK(forecast.ciLower.empty());
    CHECK(forecast.ciUpper.empty());
}

TEST_CASE(arima_fit_es_determinista) {
    const std::vector<double> y = serieSenoidal(60);
    const arima::Fit a = arima::fit111(y);
    const arima::Fit b = arima::fit111(y);
    CHECK_NEAR(a.phi, b.phi, 0.0);
    CHECK_NEAR(a.theta, b.theta, 0.0);
    CHECK_NEAR(a.sigma, b.sigma, 0.0);
}

TEST_CASE(arima_forecast_sobre_fit_real_es_finito) {
    const arima::Fit fit = arima::fit111(serieSenoidal(70));
    const arima::Forecast forecast = arima::forecast(fit, 6);
    for (double value : forecast.levels) CHECK(std::isfinite(value));
    for (double value : forecast.ciLower) CHECK(std::isfinite(value));
    for (double value : forecast.ciUpper) CHECK(std::isfinite(value));
}
