#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "arima.hpp"
#include "csv.hpp"
#include "fx_series.hpp"
#include "mlp.hpp"
#include "stats.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

namespace {

struct SerieDividida {
    std::vector<fx::Obs> obs;
    std::vector<double> niveles;
    std::vector<double> train;
    std::vector<double> test;
};

SerieDividida cargarSerie() {
    const testutil::fs::path root = testutil::projectRoot();
    SerieDividida serie;
    serie.obs = fx::readHistory(root / "data" / "fx_historico_eur.csv");
    for (const auto& item : serie.obs) serie.niveles.push_back(item.tipoCambio);

    std::size_t corte = 0;
    while (corte < serie.obs.size() && fx::isTrainYear(serie.obs[corte].year)) ++corte;
    serie.train.assign(serie.niveles.begin(), serie.niveles.begin() + static_cast<long>(corte));
    serie.test.assign(serie.niveles.begin() + static_cast<long>(corte), serie.niveles.end());
    return serie;
}

struct PrediccionMlp {
    std::vector<double> test;
    std::vector<double> futuro;
};

PrediccionMlp ejecutarMlp(const SerieDividida& serie) {
    const std::vector<fx::FeatureRow> features = fx::buildFeatures(serie.obs);

    mlp::Mat xTrain, xTest;
    mlp::Vec yTrain, yTest;
    for (const auto& fila : features) {
        if (fx::isTrainYear(fila.year)) {
            xTrain.push_back(fila.x);
            yTrain.push_back(fila.y);
        } else {
            xTest.push_back(fila.x);
            yTest.push_back(fila.y);
        }
    }

    mlp::Scaler scalerX;
    scalerX.fit(xTrain);
    mlp::ScalerTarget scalerY;
    scalerY.fit(yTrain);

    const mlp::Mat xTrainN = scalerX.transform(xTrain);
    mlp::Vec yTrainN(yTrain.size());
    for (std::size_t i = 0; i < yTrain.size(); ++i) {
        yTrainN[i] = scalerY.transform(yTrain[i]);
    }

    mlp::Mlp modelo(fx::kFeatureNames.size(), 2, 42);
    modelo.train(xTrainN, yTrainN, 500, 0.005);

    PrediccionMlp out;
    for (const auto& row : xTest) {
        out.test.push_back(scalerY.inverse(modelo.predict(scalerX.transform(row))));
    }

    mlp::Mat allX;
    mlp::Vec allY;
    for (const auto& fila : features) {
        allX.push_back(fila.x);
        allY.push_back(fila.y);
    }

    mlp::Scaler scalerFullX;
    scalerFullX.fit(allX);
    mlp::ScalerTarget scalerFullY;
    scalerFullY.fit(allY);

    mlp::Mat xFullN;
    mlp::Vec yFullN;
    for (const auto& fila : features) {
        xFullN.push_back(scalerFullX.transform(fila.x));
        yFullN.push_back(scalerFullY.transform(fila.y));
    }

    mlp::Mlp modeloFull(fx::kFeatureNames.size(), 2, 42);
    modeloFull.train(xFullN, yFullN, 500, 0.005);

    std::vector<double> historia = serie.niveles;
    const double ultimaTasaBanxico = serie.obs.back().tasaBanxico;
    const double ultimaTasaBce = serie.obs.back().tasaBce;
    const double ultimaInflMx = serie.obs.back().inflacionMx;
    const double ultimaInflEz = serie.obs.back().inflacionEz;
    const double pi = 3.14159265358979323846;

    for (int h = 1; h <= 6; ++h) {
        const std::size_t n = historia.size();
        const int month = serie.obs.back().month + h;
        const int mesNormalizado = ((month - 1) % 12) + 1;
        const mlp::Vec x = {
            historia[n - 1], historia[n - 2], historia[n - 3],
            historia[n - 6], historia[n - 12],
            ultimaTasaBanxico - ultimaTasaBce,
            ultimaInflMx, ultimaInflEz,
            std::sin(2.0 * pi * mesNormalizado / 12.0),
            std::cos(2.0 * pi * mesNormalizado / 12.0),
            static_cast<double>(serie.niveles.size() - 1 + static_cast<std::size_t>(h))
        };
        const double value = scalerFullY.inverse(modeloFull.predict(scalerFullX.transform(x)));
        out.futuro.push_back(value);
        historia.push_back(value);
    }

    return out;
}

} // namespace

TEST_CASE(integracion_reportes_tienen_la_estructura_esperada) {
    const testutil::fs::path root = testutil::projectRoot();
    const csvutil::Table metrics =
        csvutil::readCsv((root / "reports" / "metricas_fx.csv").string());
    const csvutil::Table forecast =
        csvutil::readCsv((root / "reports" / "pronostico_6m.csv").string());

    CHECK_EQ(metrics.header.size(), std::size_t{4});
    CHECK_EQ(metrics.rows.size(), std::size_t{2});
    CHECK_EQ(metrics.rows[0][0], std::string("ARIMA(1,1,1)"));
    CHECK_EQ(metrics.rows[1][0], std::string("MLP (red neuronal)"));

    CHECK_EQ(forecast.header.size(), std::size_t{3});
    CHECK_EQ(forecast.rows.size(), std::size_t{6});
    CHECK_EQ(forecast.rows.front()[0], std::string("2025-01-01"));
    CHECK_EQ(forecast.rows.back()[0], std::string("2025-06-01"));
}

TEST_CASE(integracion_metricas_arima_coinciden_con_el_reporte) {
    const testutil::fs::path root = testutil::projectRoot();
    const SerieDividida serie = cargarSerie();
    const csvutil::Table metrics =
        csvutil::readCsv((root / "reports" / "metricas_fx.csv").string());

    const arima::Fit fit = arima::fit111(serie.train);
    const arima::Forecast forecast = arima::forecast(fit, static_cast<int>(serie.test.size()));
    const stats::Metrics result = stats::evaluate(serie.test, forecast.levels);

    CHECK_NEAR(result.mae, std::stod(metrics.rows[0][1]), 1e-3);
    CHECK_NEAR(result.rmse, std::stod(metrics.rows[0][2]), 1e-3);
    CHECK_NEAR(result.mape, std::stod(metrics.rows[0][3]), 1e-3);
}

TEST_CASE(integracion_metricas_mlp_coinciden_con_el_reporte) {
    const testutil::fs::path root = testutil::projectRoot();
    const SerieDividida serie = cargarSerie();
    const csvutil::Table metrics =
        csvutil::readCsv((root / "reports" / "metricas_fx.csv").string());

    const std::vector<fx::FeatureRow> features = fx::buildFeatures(serie.obs);
    mlp::Mat xTrain, xTest;
    mlp::Vec yTrain, yTest;
    for (const auto& fila : features) {
        if (fx::isTrainYear(fila.year)) {
            xTrain.push_back(fila.x);
            yTrain.push_back(fila.y);
        } else {
            xTest.push_back(fila.x);
            yTest.push_back(fila.y);
        }
    }

    mlp::Scaler scalerX;
    scalerX.fit(xTrain);
    mlp::ScalerTarget scalerY;
    scalerY.fit(yTrain);
    const mlp::Mat xTrainN = scalerX.transform(xTrain);
    mlp::Vec yTrainN(yTrain.size());
    for (std::size_t i = 0; i < yTrain.size(); ++i) {
        yTrainN[i] = scalerY.transform(yTrain[i]);
    }

    mlp::Mlp modelo(fx::kFeatureNames.size(), 2, 42);
    modelo.train(xTrainN, yTrainN, 500, 0.005);

    std::vector<double> predicciones;
    for (const auto& row : xTest) {
        predicciones.push_back(scalerY.inverse(modelo.predict(scalerX.transform(row))));
    }

    const stats::Metrics result = stats::evaluate(yTest, predicciones);
    CHECK_NEAR(result.mae, std::stod(metrics.rows[1][1]), 1e-3);
    CHECK_NEAR(result.rmse, std::stod(metrics.rows[1][2]), 1e-3);
    CHECK_NEAR(result.mape, std::stod(metrics.rows[1][3]), 1e-3);
}

TEST_CASE(integracion_forecast_arima_coincide_con_el_reporte) {
    const testutil::fs::path root = testutil::projectRoot();
    const SerieDividida serie = cargarSerie();
    const csvutil::Table forecastTable =
        csvutil::readCsv((root / "reports" / "pronostico_6m.csv").string());

    const arima::Fit fit = arima::fit111(serie.niveles);
    const arima::Forecast forecast = arima::forecast(fit, 6);
    CHECK_EQ(forecast.levels.size(), std::size_t{6});
    for (std::size_t i = 0; i < forecast.levels.size(); ++i) {
        CHECK_NEAR(forecast.levels[i], std::stod(forecastTable.rows[i][1]), 1e-3);
    }
}

TEST_CASE(integracion_forecast_mlp_coincide_con_el_reporte) {
    const testutil::fs::path root = testutil::projectRoot();
    const SerieDividida serie = cargarSerie();
    const csvutil::Table forecastTable =
        csvutil::readCsv((root / "reports" / "pronostico_6m.csv").string());

    const PrediccionMlp mlpResult = ejecutarMlp(serie);
    CHECK_EQ(mlpResult.futuro.size(), std::size_t{6});
    for (std::size_t i = 0; i < mlpResult.futuro.size(); ++i) {
        CHECK_NEAR(mlpResult.futuro[i], std::stod(forecastTable.rows[i][2]), 1e-3);
    }
}
