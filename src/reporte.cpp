// reporte.cpp — consolida los resultados del pipeline en:
//   reports/calidad_limpieza.csv
//   reports/resumen.md
//
// Debe ejecutarse despues de limpieza y forecasting. Vuelve a aplicar las
// reglas de limpieza sobre el CSV crudo para verificar las cifras.

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "reporting.hpp"

int main() {
    try {
        namespace fs = std::filesystem;
        fs::create_directories("reports");

        const reporting::CleaningQuality quality =
            reporting::analyzeCleaning("data/leads_ventas.csv");
        const std::vector<reporting::ModelScore> scores =
            reporting::readModelScores("reports/metricas_fx.csv");
        const std::vector<reporting::ForecastPoint> forecast =
            reporting::readForecast("reports/pronostico_6m.csv");

        reporting::writeQualityCsv("reports/calidad_limpieza.csv", quality);
        reporting::writeSummaryMarkdown("reports/resumen.md", quality, scores, forecast);

        std::cout << "Filas limpias: " << quality.cleanRows << "\n";
        std::cout << "Duplicados eliminados: " << quality.duplicates << "\n";
        std::cout << "Validacion de limpieza: " << (quality.valid() ? "OK" : "FALLO")
                  << "\n";
        std::cout << "Mejor modelo por MAE: " << reporting::bestModel(scores) << "\n";
        std::cout << "Umbral MAPE (3.00%): "
                  << (reporting::allWithinThreshold(scores, 3.0) ? "OK" : "FALLO")
                  << "\n";
        std::cout << "\nReportes guardados en reports/resumen.md y "
                     "reports/calidad_limpieza.csv\n";

        return (quality.valid() && reporting::allWithinThreshold(scores, 3.0)) ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error al consolidar el reporte: " << error.what() << "\n";
        return 1;
    }
}
