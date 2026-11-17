#pragma once
// fx_series.hpp — lectura, interpolacion y construccion de variables de la
// serie mensual MXN/EUR. Se extrajo del programa principal para poder
// probar cada transformacion por separado.

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "csv.hpp"

namespace fx {

struct Obs {
    int year = 0;
    int month = 0;
    double tipoCambio = 0.0;
    double tasaBanxico = 0.0;
    double tasaBce = 0.0;
    double inflacionMx = 0.0;
    double inflacionEz = 0.0;
};

struct FeatureRow {
    int t = 0;
    int year = 0;
    int month = 0;
    std::vector<double> x;
    double y = 0.0;
};

inline const std::vector<std::string> kFeatureNames = {
    "lag_1", "lag_2", "lag_3", "lag_6", "lag_12",
    "diferencial_tasas", "inflacion_mx", "inflacion_ez",
    "mes_sin", "mes_cos", "tendencia"
};

inline std::vector<Obs> readHistory(const std::filesystem::path& path) {
    const csvutil::Table table = csvutil::readCsv(path.string());
    std::vector<Obs> obs;
    obs.reserve(table.rows.size());

    const int cFecha = table.colIndex("fecha");
    const int cTc = table.colIndex("tipo_cambio");
    const int cBanxico = table.colIndex("tasa_banxico");
    const int cBce = table.colIndex("tasa_bce");
    const int cInflMx = table.colIndex("inflacion_mx");
    const int cInflEz = table.colIndex("inflacion_ez");

    if (cFecha < 0 || cTc < 0 || cBanxico < 0 || cBce < 0 || cInflMx < 0 || cInflEz < 0) {
        throw std::runtime_error("fx::readHistory: faltan columnas obligatorias");
    }

    std::vector<bool> nulo;
    nulo.reserve(table.rows.size());

    for (const auto& row : table.rows) {
        Obs item;
        const std::string fecha = row[static_cast<std::size_t>(cFecha)];
        item.year = std::stoi(fecha.substr(0, 4));
        item.month = std::stoi(fecha.substr(5, 2));

        const std::string tc = row[static_cast<std::size_t>(cTc)];
        const bool esNulo = tc.empty();
        item.tipoCambio = esNulo ? 0.0 : std::stod(tc);
        item.tasaBanxico = std::stod(row[static_cast<std::size_t>(cBanxico)]);
        item.tasaBce = std::stod(row[static_cast<std::size_t>(cBce)]);
        item.inflacionMx = std::stod(row[static_cast<std::size_t>(cInflMx)]);
        item.inflacionEz = std::stod(row[static_cast<std::size_t>(cInflEz)]);

        obs.push_back(item);
        nulo.push_back(esNulo);
    }

    const std::size_t n = obs.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!nulo[i]) continue;

        std::size_t left = i;
        while (left > 0 && nulo[left - 1]) --left;
        const bool hayIzq = (left > 0);

        std::size_t right = i;
        while (right + 1 < n && nulo[right + 1]) ++right;
        const bool hayDer = (right + 1 < n);

        if (hayIzq && hayDer) {
            const double v0 = obs[left - 1].tipoCambio;
            const double v1 = obs[right + 1].tipoCambio;
            const double span = static_cast<double>(right + 1 - (left - 1));
            const double pos = static_cast<double>(i - (left - 1));
            obs[i].tipoCambio = v0 + (v1 - v0) * (pos / span);
        } else if (hayIzq) {
            obs[i].tipoCambio = obs[left - 1].tipoCambio;
        } else if (hayDer) {
            obs[i].tipoCambio = obs[right + 1].tipoCambio;
        }
    }

    return obs;
}

inline std::vector<FeatureRow> buildFeatures(const std::vector<Obs>& obs) {
    std::vector<FeatureRow> filas;
    if (obs.size() <= 12) return filas;
    filas.reserve(obs.size() - 12);

    const double pi = 3.14159265358979323846;
    for (std::size_t t = 12; t < obs.size(); ++t) {
        FeatureRow fila;
        fila.t = static_cast<int>(t);
        fila.year = obs[t].year;
        fila.month = obs[t].month;
        fila.x = {
            obs[t - 1].tipoCambio,
            obs[t - 2].tipoCambio,
            obs[t - 3].tipoCambio,
            obs[t - 6].tipoCambio,
            obs[t - 12].tipoCambio,
            obs[t].tasaBanxico - obs[t].tasaBce,
            obs[t].inflacionMx,
            obs[t].inflacionEz,
            std::sin(2.0 * pi * obs[t].month / 12.0),
            std::cos(2.0 * pi * obs[t].month / 12.0),
            static_cast<double>(t)
        };
        fila.y = obs[t].tipoCambio;
        filas.push_back(std::move(fila));
    }
    return filas;
}

inline bool isTrainYear(int year) {
    return year < 2023;
}

inline std::string formatFixed4(double value) {
    std::ostringstream oss;
    oss.precision(4);
    oss << std::fixed << value;
    return oss.str();
}

} // namespace fx
