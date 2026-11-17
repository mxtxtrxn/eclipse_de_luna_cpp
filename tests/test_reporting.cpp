#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "csv.hpp"
#include "reporting.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

namespace {

std::vector<reporting::ModelScore> modelosDePrueba() {
    return {
        {"ARIMA(1,1,1)", 0.39, 0.51, 1.86},
        {"MLP (red neuronal)", 0.52, 0.65, 2.44}
    };
}

std::vector<reporting::ForecastPoint> pronosticoDePrueba() {
    return {
        {"2025-01-01", 21.6433, 21.1475},
        {"2025-02-01", 21.6011, 21.1365},
        {"2025-03-01", 21.5923, 21.1835}
    };
}

} // namespace

TEST_CASE(reporting_lee_metricas_reales) {
    const testutil::fs::path path =
        testutil::projectRoot() / "reports" / "metricas_fx.csv";
    const std::vector<reporting::ModelScore> scores = reporting::readModelScores(path);

    CHECK_EQ(scores.size(), std::size_t{2});
    CHECK_EQ(scores[0].name, std::string("ARIMA(1,1,1)"));
    CHECK_EQ(scores[1].name, std::string("MLP (red neuronal)"));
    CHECK_NEAR(scores[0].mae, 0.3976, 1e-4);
    CHECK_NEAR(scores[1].mape, 2.4452, 1e-4);
}

TEST_CASE(reporting_lee_pronostico_real) {
    const testutil::fs::path path =
        testutil::projectRoot() / "reports" / "pronostico_6m.csv";
    const std::vector<reporting::ForecastPoint> points = reporting::readForecast(path);

    CHECK_EQ(points.size(), std::size_t{6});
    CHECK_EQ(points.front().date, std::string("2025-01-01"));
    CHECK_EQ(points.back().date, std::string("2025-06-01"));
    CHECK_NEAR(points.front().arima, 21.6433, 1e-4);
    CHECK_NEAR(points.back().mlp, 21.1685, 1e-4);
}

TEST_CASE(reporting_analiza_la_limpieza_real) {
    const testutil::fs::path path =
        testutil::projectRoot() / "data" / "leads_ventas.csv";
    const reporting::CleaningQuality quality = reporting::analyzeCleaning(path);

    CHECK_EQ(quality.rawRows, std::size_t{5250});
    CHECK_EQ(quality.cleanRows, std::size_t{5000});
    CHECK_EQ(quality.duplicates, std::size_t{250});
    CHECK(quality.uniqueIds);
    CHECK_EQ(quality.salesWithoutAmount, std::size_t{0});
    CHECK_EQ(quality.amountsWithoutSale, std::size_t{0});
    CHECK(quality.valid());
}

TEST_CASE(reporting_mejor_modelo_por_mae) {
    CHECK_EQ(reporting::bestModel(modelosDePrueba()), std::string("ARIMA(1,1,1)"));
}

TEST_CASE(reporting_mejor_modelo_sin_datos_lanza) {
    CHECK_THROWS(reporting::bestModel({}));
}

TEST_CASE(reporting_umbral_mape) {
    CHECK(reporting::allWithinThreshold(modelosDePrueba(), 3.0));
    CHECK(!reporting::allWithinThreshold(modelosDePrueba(), 2.0));
    CHECK(!reporting::allWithinThreshold({}, 3.0));
}

TEST_CASE(reporting_rango_de_pronostico) {
    const auto arima = reporting::forecastRange(pronosticoDePrueba(), true);
    const auto mlp = reporting::forecastRange(pronosticoDePrueba(), false);
    CHECK_NEAR(arima.first, 21.5923, 1e-12);
    CHECK_NEAR(arima.second, 21.6433, 1e-12);
    CHECK_NEAR(mlp.first, 21.1365, 1e-12);
    CHECK_NEAR(mlp.second, 21.1835, 1e-12);
}

TEST_CASE(reporting_rango_sin_datos_lanza) {
    CHECK_THROWS(reporting::forecastRange({}, true));
}

TEST_CASE(reporting_resumen_contiene_las_secciones) {
    reporting::CleaningQuality quality;
    quality.rawRows = 5250;
    quality.cleanRows = 5000;
    quality.duplicates = 250;
    quality.uniqueIds = true;
    quality.minAmount = 15057.1;
    quality.minVpg = 501.78;

    const std::string summary = reporting::buildSummaryMarkdown(
        quality, modelosDePrueba(), pronosticoDePrueba());

    CHECK(summary.find("# Resumen del Proyecto Eclipse de Luna") != std::string::npos);
    CHECK(summary.find("## Calidad de datos") != std::string::npos);
    CHECK(summary.find("## Modelos") != std::string::npos);
    CHECK(summary.find("## Pronostico MXN/EUR") != std::string::npos);
    CHECK(summary.find("- Filas limpias: 5000") != std::string::npos);
    CHECK(summary.find("Mejor modelo por MAE: ARIMA(1,1,1)") != std::string::npos);
    CHECK(summary.find("| 2025-03-01 |") != std::string::npos);
    CHECK(summary.find("Umbral MAPE (3.00%): OK") != std::string::npos);
}

TEST_CASE(reporting_escribe_calidad_csv) {
    const testutil::fs::path dir = testutil::tempDir("reporting_calidad");
    const testutil::fs::path path = dir / "calidad.csv";

    reporting::CleaningQuality quality;
    quality.rawRows = 10;
    quality.cleanRows = 8;
    quality.duplicates = 2;
    quality.uniqueIds = true;
    quality.minAmount = 100.0;
    quality.minVpg = 10.0;

    reporting::writeQualityCsv(path, quality);
    const csvutil::Table table = csvutil::readCsv(path.string());

    CHECK_EQ(table.header.size(), std::size_t{10});
    CHECK_EQ(table.rows.size(), std::size_t{1});
    CHECK_EQ(table.rows[0][0], std::string("10"));
    CHECK_EQ(table.rows[0][1], std::string("8"));
    CHECK_EQ(table.rows[0][5], std::string("OK"));
    CHECK_EQ(table.rows[0][8], std::string("100.00"));
}

TEST_CASE(reporting_escribe_resumen_markdown) {
    const testutil::fs::path dir = testutil::tempDir("reporting_resumen");
    const testutil::fs::path path = dir / "resumen.md";

    reporting::CleaningQuality quality;
    quality.rawRows = 1;
    quality.cleanRows = 1;
    quality.uniqueIds = true;
    quality.minAmount = 1.0;
    quality.minVpg = 1.0;

    reporting::writeSummaryMarkdown(path, quality, modelosDePrueba(),
                                    pronosticoDePrueba());
    const std::string content = testutil::readText(path);
    CHECK(content.find("# Resumen del Proyecto Eclipse de Luna") != std::string::npos);
    CHECK(content.find("ARIMA(1,1,1)") != std::string::npos);
}

TEST_CASE(reporting_metricas_con_columnas_invalidas_lanza) {
    const testutil::fs::path dir = testutil::tempDir("reporting_invalido");
    const testutil::fs::path path = dir / "metricas.csv";
    testutil::writeText(path, "modelo,MAE\nARIMA,0.4\n");
    CHECK_THROWS(reporting::readModelScores(path));
}

TEST_CASE(reporting_metricas_con_numero_invalido_lanza) {
    const testutil::fs::path dir = testutil::tempDir("reporting_numero");
    const testutil::fs::path path = dir / "metricas.csv";
    testutil::writeText(path, ",MAE,RMSE,MAPE\nARIMA,abc,0.5,1.0\n");
    CHECK_THROWS(reporting::readModelScores(path));
}
