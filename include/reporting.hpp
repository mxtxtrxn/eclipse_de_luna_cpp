#pragma once
// reporting.hpp — consolidacion de resultados del pipeline en un reporte
// ejecutivo y en un CSV de calidad de datos. Lee los reportes generados y
// vuelve a aplicar las reglas de limpieza para verificar sus cifras.

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "cleaning.hpp"
#include "csv.hpp"

namespace reporting {

struct ModelScore {
    std::string name;
    double mae = 0.0;
    double rmse = 0.0;
    double mape = 0.0;
};

struct ForecastPoint {
    std::string date;
    double arima = 0.0;
    double mlp = 0.0;
};

struct CleaningQuality {
    std::size_t rawRows = 0;
    std::size_t cleanRows = 0;
    std::size_t duplicates = 0;
    std::size_t imputedAmounts = 0;
    std::size_t imputedVpg = 0;
    bool uniqueIds = false;
    std::size_t salesWithoutAmount = 0;
    std::size_t amountsWithoutSale = 0;
    double minAmount = 0.0;
    double minVpg = 0.0;

    bool valid() const {
        return uniqueIds && salesWithoutAmount == 0 && amountsWithoutSale == 0 &&
               minAmount > 0.0 && minVpg > 0.0;
    }
};

inline double parseNumber(const std::string& value, const std::string& context) {
    if (value.empty()) {
        throw std::runtime_error("Valor numerico vacio en " + context);
    }
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        throw std::runtime_error("Valor numerico invalido en " + context + ": " + value);
    }
}

inline void requireShape(const csvutil::Table& table, std::size_t columns,
                         const std::string& context) {
    if (table.header.size() != columns) {
        throw std::runtime_error(context + ": se esperaban " +
                                 std::to_string(columns) + " columnas");
    }
    for (const auto& row : table.rows) {
        if (row.size() != columns) {
            throw std::runtime_error(context + ": fila con columnas inconsistentes");
        }
    }
}

inline std::vector<ModelScore> readModelScores(const std::filesystem::path& path) {
    const csvutil::Table table = csvutil::readCsv(path.string());
    requireShape(table, 4, "metricas_fx.csv");
    if (table.rows.empty()) {
        throw std::runtime_error("metricas_fx.csv no contiene modelos");
    }

    std::vector<ModelScore> scores;
    scores.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        ModelScore score;
        score.name = row[0];
        score.mae = parseNumber(row[1], "MAE");
        score.rmse = parseNumber(row[2], "RMSE");
        score.mape = parseNumber(row[3], "MAPE");
        scores.push_back(std::move(score));
    }
    return scores;
}

inline std::vector<ForecastPoint> readForecast(const std::filesystem::path& path) {
    const csvutil::Table table = csvutil::readCsv(path.string());
    requireShape(table, 3, "pronostico_6m.csv");
    if (table.rows.empty()) {
        throw std::runtime_error("pronostico_6m.csv no contiene pronosticos");
    }

    std::vector<ForecastPoint> points;
    points.reserve(table.rows.size());
    for (const auto& row : table.rows) {
        ForecastPoint point;
        point.date = row[0];
        point.arima = parseNumber(row[1], "ARIMA");
        point.mlp = parseNumber(row[2], "MLP");
        points.push_back(std::move(point));
    }
    return points;
}

inline CleaningQuality qualityFromReport(const cleaning::Report& report,
                                         std::size_t cleanRows) {
    CleaningQuality quality;
    quality.rawRows = report.filasLeidas;
    quality.cleanRows = cleanRows;
    quality.duplicates = report.duplicadosEliminados;
    quality.imputedAmounts = report.montosImputados;
    quality.imputedVpg = report.vpgsImputados;
    quality.uniqueIds = report.idsUnicos;
    quality.salesWithoutAmount = report.ventasSinMonto;
    quality.amountsWithoutSale = report.montosSinVenta;
    quality.minAmount = report.montoMinimo;
    quality.minVpg = report.vpgMinimo;
    return quality;
}

inline CleaningQuality analyzeCleaning(const std::filesystem::path& leadsPath) {
    const cleaning::Result result = cleaning::cleanFile(leadsPath);
    return qualityFromReport(result.reporte, result.filas.size());
}

inline std::string bestModel(const std::vector<ModelScore>& scores) {
    if (scores.empty()) {
        throw std::runtime_error("bestModel(): no hay modelos");
    }
    const auto best = std::min_element(
        scores.begin(), scores.end(),
        [](const ModelScore& a, const ModelScore& b) { return a.mae < b.mae; });
    return best->name;
}

inline bool allWithinThreshold(const std::vector<ModelScore>& scores,
                               double threshold) {
    if (scores.empty()) return false;
    for (const auto& score : scores) {
        if (score.mape > threshold) return false;
    }
    return true;
}

inline std::pair<double, double> forecastRange(const std::vector<ForecastPoint>& points,
                                               bool arima) {
    if (points.empty()) {
        throw std::runtime_error("forecastRange(): no hay pronosticos");
    }
    double minimum = arima ? points.front().arima : points.front().mlp;
    double maximum = minimum;
    for (const auto& point : points) {
        const double value = arima ? point.arima : point.mlp;
        minimum = std::min(minimum, value);
        maximum = std::max(maximum, value);
    }
    return {minimum, maximum};
}

inline std::string formatNumber(double value, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << value;
    return oss.str();
}

inline std::string status(bool condition) {
    return condition ? "OK" : "FALLO";
}

inline std::string buildSummaryMarkdown(const CleaningQuality& quality,
                                        const std::vector<ModelScore>& scores,
                                        const std::vector<ForecastPoint>& forecast,
                                        double mapeThreshold = 3.0) {
    std::ostringstream out;
    out << "# Resumen del Proyecto Eclipse de Luna\n\n";
    out << "## Calidad de datos\n\n";
    out << "- Filas crudas: " << quality.rawRows << "\n";
    out << "- Filas limpias: " << quality.cleanRows << "\n";
    out << "- Duplicados eliminados: " << quality.duplicates << "\n";
    out << "- Montos imputados: " << quality.imputedAmounts << "\n";
    out << "- VPG imputados: " << quality.imputedVpg << "\n";
    out << "- IDs unicos: " << status(quality.uniqueIds) << "\n";
    out << "- Ventas sin monto: " << quality.salesWithoutAmount << "\n";
    out << "- Montos sin venta: " << quality.amountsWithoutSale << "\n";
    out << "- Monto minimo: " << formatNumber(quality.minAmount, 2) << "\n";
    out << "- VPG minimo: " << formatNumber(quality.minVpg, 2) << "\n";
    out << "- Validacion general: " << status(quality.valid()) << "\n\n";

    out << "## Modelos\n\n";
    out << "| Modelo | MAE | RMSE | MAPE |\n";
    out << "|---|---|---|---|\n";
    for (const auto& score : scores) {
        out << "| " << score.name << " | " << formatNumber(score.mae, 4) << " | "
            << formatNumber(score.rmse, 4) << " | " << formatNumber(score.mape, 4)
            << "% |\n";
    }
    out << "\n";
    if (!scores.empty()) {
        out << "- Mejor modelo por MAE: " << bestModel(scores) << "\n";
    }
    out << "- Umbral MAPE (" << formatNumber(mapeThreshold, 2) << "%): "
        << status(allWithinThreshold(scores, mapeThreshold)) << "\n\n";

    out << "## Pronostico MXN/EUR\n\n";
    out << "| Fecha | ARIMA | MLP |\n";
    out << "|---|---|---|\n";
    for (const auto& point : forecast) {
        out << "| " << point.date << " | " << formatNumber(point.arima, 4) << " | "
            << formatNumber(point.mlp, 4) << " |\n";
    }
    out << "\n";
    if (!forecast.empty()) {
        const auto arimaRange = forecastRange(forecast, true);
        const auto mlpRange = forecastRange(forecast, false);
        out << "- Rango ARIMA: " << formatNumber(arimaRange.first, 4) << " a "
            << formatNumber(arimaRange.second, 4) << "\n";
        out << "- Rango MLP: " << formatNumber(mlpRange.first, 4) << " a "
            << formatNumber(mlpRange.second, 4) << "\n";
    }
    return out.str();
}

inline void writeQualityCsv(const std::filesystem::path& path,
                            const CleaningQuality& quality) {
    csvutil::Writer writer(path.string());
    writer.writeRow({"filas_crudas", "filas_limpias", "duplicados_eliminados",
                     "montos_imputados", "vpg_imputados", "ids_unicos",
                     "ventas_sin_monto", "montos_sin_venta", "monto_minimo",
                     "vpg_minimo"});
    writer.writeRow({
        std::to_string(quality.rawRows),
        std::to_string(quality.cleanRows),
        std::to_string(quality.duplicates),
        std::to_string(quality.imputedAmounts),
        std::to_string(quality.imputedVpg),
        status(quality.uniqueIds),
        std::to_string(quality.salesWithoutAmount),
        std::to_string(quality.amountsWithoutSale),
        formatNumber(quality.minAmount, 2),
        formatNumber(quality.minVpg, 2)
    });
}

inline void writeSummaryMarkdown(const std::filesystem::path& path,
                                 const CleaningQuality& quality,
                                 const std::vector<ModelScore>& scores,
                                 const std::vector<ForecastPoint>& forecast,
                                 double mapeThreshold = 3.0) {
    const std::string content =
        buildSummaryMarkdown(quality, scores, forecast, mapeThreshold);
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("No se pudo escribir: " + path.string());
    }
    out << content;
}

} // namespace reporting
