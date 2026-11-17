// forecasting.cpp — Parte 2: pronostico de tipo de cambio MXN/EUR.
// Lee data/fx_historico_eur.csv y escribe reports/metricas_fx.csv y
// reports/pronostico_6m.csv.
//
//  1. La serie objetivo es MXN/EUR.
//  2. El modelo de Machine Learning es una red neuronal (MLP).
//  3. Todo esta escrito en C++17 sin dependencias externas.
//
// El pronostico a futuro se genera reentrenando con TODA la serie historica
// disponible (2014-2024), mientras que las metricas de error se calculan con
// el tramo de entrenamiento (2014-2022) y el de prueba (2023-2024).

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "arima.hpp"
#include "csv.hpp"
#include "date_utils.hpp"
#include "fx_series.hpp"
#include "mlp.hpp"
#include "stats.hpp"

namespace fs = std::filesystem;

int main() {
    try {
        const fs::path inPath = "data/fx_historico_eur.csv";
        fs::create_directories("reports");

        const std::vector<fx::Obs> obs = fx::readHistory(inPath);
        std::cout << "Observaciones leidas: " << obs.size() << "\n";
        if (obs.size() <= 12) {
            std::cerr << "Error: la serie necesita mas de 12 observaciones\n";
            return 1;
        }

        std::vector<double> niveles;
        niveles.reserve(obs.size());
        for (const auto& o : obs) niveles.push_back(o.tipoCambio);

        int corteIdx = -1;
        for (std::size_t i = 0; i < obs.size(); ++i) {
            if (!fx::isTrainYear(obs[i].year)) {
                corteIdx = static_cast<int>(i);
                break;
            }
        }
        if (corteIdx <= 0) {
            std::cerr << "Error: no se encontro un tramo de prueba valido\n";
            return 1;
        }
        std::cout << "Indice de corte (2023-01): " << corteIdx << "\n";

        const std::vector<double> nivelesTrain(niveles.begin(), niveles.begin() + corteIdx);
        const std::vector<double> nivelesTest(niveles.begin() + corteIdx, niveles.end());
        std::cout << "ARIMA  -> train=" << nivelesTrain.size()
                  << "  test=" << nivelesTest.size() << "\n";

        // ---------------------------------------------------------------
        // ARIMA(1,1,1): metricas sobre el periodo de prueba (2023-2024)
        // ---------------------------------------------------------------
        const arima::Fit fitArimaTrain = arima::fit111(nivelesTrain);
        std::cout << "ARIMA(1,1,1) phi=" << fitArimaTrain.phi
                  << "  theta=" << fitArimaTrain.theta << "\n";

        const arima::Forecast fcTest =
            arima::forecast(fitArimaTrain, static_cast<int>(nivelesTest.size()));
        const stats::Metrics metArima = stats::evaluate(nivelesTest, fcTest.levels);
        std::cout << "[ARIMA(1,1,1)] MAE=" << metArima.mae << "  RMSE=" << metArima.rmse
                  << "  MAPE=" << metArima.mape << "%\n";

        // ---------------------------------------------------------------
        // MLP: metricas sobre el mismo periodo de prueba
        // ---------------------------------------------------------------
        const std::vector<fx::FeatureRow> features = fx::buildFeatures(obs);

        mlp::Mat xTrain, xTest;
        mlp::Vec yTrain, yTest;
        for (const auto& f : features) {
            if (fx::isTrainYear(f.year)) {
                xTrain.push_back(f.x);
                yTrain.push_back(f.y);
            } else {
                xTest.push_back(f.x);
                yTest.push_back(f.y);
            }
        }
        std::cout << "MLP    -> train=" << xTrain.size()
                  << "  test=" << xTest.size() << "\n";

        mlp::Scaler scalerXTrain;
        scalerXTrain.fit(xTrain);
        mlp::ScalerTarget scalerYTrain;
        scalerYTrain.fit(yTrain);

        const mlp::Mat xTrainN = scalerXTrain.transform(xTrain);
        mlp::Vec yTrainN(yTrain.size());
        for (std::size_t i = 0; i < yTrain.size(); ++i) {
            yTrainN[i] = scalerYTrain.transform(yTrain[i]);
        }

        // Hiperparametros elegidos por validacion, nunca mirando el test:
        // una red chica (2 neuronas ocultas) generaliza mejor con solo 96
        // observaciones de entrenamiento.
        const std::size_t nHidden = 2;
        const unsigned seedMlp = 42;
        const int epochs = 500;
        const double lr = 0.005;

        mlp::Mlp modeloTest(fx::kFeatureNames.size(), nHidden, seedMlp);
        modeloTest.train(xTrainN, yTrainN, epochs, lr);

        const mlp::Mat xTestN = scalerXTrain.transform(xTest);
        std::vector<double> predMlpTest(xTestN.size());
        for (std::size_t i = 0; i < xTestN.size(); ++i) {
            predMlpTest[i] = scalerYTrain.inverse(modeloTest.predict(xTestN[i]));
        }

        const stats::Metrics metMlp = stats::evaluate(yTest, predMlpTest);
        std::cout << "[MLP]          MAE=" << metMlp.mae << "  RMSE=" << metMlp.rmse
                  << "  MAPE=" << metMlp.mape << "%\n";

        // ---------------------------------------------------------------
        // Pronostico a 6 meses (enero-junio 2025), reentrenando con TODA
        // la serie historica disponible.
        // ---------------------------------------------------------------
        const int horizonte = 6;

        const arima::Fit fitArimaFull = arima::fit111(niveles);
        const arima::Forecast fcFuturo = arima::forecast(fitArimaFull, horizonte);

        mlp::Mat allX;
        mlp::Vec allY;
        for (const auto& f : features) {
            allX.push_back(f.x);
            allY.push_back(f.y);
        }

        mlp::Scaler scalerXFull;
        scalerXFull.fit(allX);
        mlp::ScalerTarget scalerYFull;
        scalerYFull.fit(allY);

        mlp::Mat xFullN;
        mlp::Vec yFullN;
        for (const auto& f : features) {
            xFullN.push_back(scalerXFull.transform(f.x));
            yFullN.push_back(scalerYFull.transform(f.y));
        }

        mlp::Mlp modeloFull(fx::kFeatureNames.size(), nHidden, seedMlp);
        modeloFull.train(xFullN, yFullN, epochs, lr);

        // Pronostico recursivo: cada prediccion nueva se agrega al historial
        // para poder calcular los lags del siguiente mes.
        std::vector<double> histTipoCambio = niveles;
        const double ultimaTasaBanxico = obs.back().tasaBanxico;
        const double ultimaTasaBce = obs.back().tasaBce;
        const double ultimaInflMx = obs.back().inflacionMx;
        const double ultimaInflEz = obs.back().inflacionEz;

        const dateutil::Date ultimaFecha{obs.back().year, obs.back().month, 1};
        std::vector<double> pronosticoMlp;
        std::vector<std::string> fechasFuturas;
        const double pi = 3.14159265358979323846;

        for (int h = 1; h <= horizonte; ++h) {
            const dateutil::Date fechaFutura = dateutil::addMonths(ultimaFecha, h);
            fechasFuturas.push_back(dateutil::toIsoString(fechaFutura));

            const std::size_t n = histTipoCambio.size();
            const mlp::Vec x = {
                histTipoCambio[n - 1], histTipoCambio[n - 2], histTipoCambio[n - 3],
                histTipoCambio[n - 6], histTipoCambio[n - 12],
                ultimaTasaBanxico - ultimaTasaBce,
                ultimaInflMx, ultimaInflEz,
                std::sin(2.0 * pi * fechaFutura.month / 12.0),
                std::cos(2.0 * pi * fechaFutura.month / 12.0),
                static_cast<double>(niveles.size() - 1 + static_cast<std::size_t>(h))
            };
            const double value =
                scalerYFull.inverse(modeloFull.predict(scalerXFull.transform(x)));
            pronosticoMlp.push_back(value);
            histTipoCambio.push_back(value);
        }

        std::cout << "\n=== Pronostico enero-junio 2025 ===\n";
        for (int h = 0; h < horizonte; ++h) {
            std::cout << fechasFuturas[static_cast<std::size_t>(h)]
                      << "  ARIMA=" << fx::formatFixed4(fcFuturo.levels[static_cast<std::size_t>(h)])
                      << "  MLP=" << fx::formatFixed4(pronosticoMlp[static_cast<std::size_t>(h)])
                      << "\n";
        }

        // ---------------------------------------------------------------
        // Guardar reportes
        // ---------------------------------------------------------------
        csvutil::Writer metricsWriter("reports/metricas_fx.csv");
        metricsWriter.writeRow({"", "MAE", "RMSE", "MAPE"});
        metricsWriter.writeRow({"ARIMA(1,1,1)", fx::formatFixed4(metArima.mae),
                                fx::formatFixed4(metArima.rmse),
                                fx::formatFixed4(metArima.mape)});
        metricsWriter.writeRow({"MLP (red neuronal)", fx::formatFixed4(metMlp.mae),
                                fx::formatFixed4(metMlp.rmse),
                                fx::formatFixed4(metMlp.mape)});
        std::cout << "\nMetricas guardadas en reports/metricas_fx.csv\n";

        csvutil::Writer forecastWriter("reports/pronostico_6m.csv");
        forecastWriter.writeRow({"fecha", "arima", "mlp"});
        for (int h = 0; h < horizonte; ++h) {
            forecastWriter.writeRow({
                fechasFuturas[static_cast<std::size_t>(h)],
                fx::formatFixed4(fcFuturo.levels[static_cast<std::size_t>(h)]),
                fx::formatFixed4(pronosticoMlp[static_cast<std::size_t>(h)])
            });
        }
        std::cout << "Pronostico guardado en reports/pronostico_6m.csv\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error en el pronostico: " << error.what() << "\n";
        return 1;
    }
}
