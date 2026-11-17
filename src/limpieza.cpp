// limpieza.cpp — Parte 1: limpieza de datos legados.
// Lee data/leads_ventas.csv (sucio) y escribe data/leads_ventas_clean.csv.
// Las reglas viven en include/cleaning.hpp; este programa solo las aplica
// y muestra el resumen de calidad.

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "cleaning.hpp"

int main() {
    try {
        const std::filesystem::path inPath = "data/leads_ventas.csv";
        const std::filesystem::path outPath = "data/leads_ventas_clean.csv";

        const cleaning::Result result = cleaning::cleanFile(inPath);
        const cleaning::Report& report = result.reporte;

        std::cout << "Filas leidas (crudo): " << report.filasLeidas << "\n";
        std::cout << "Montos imputados por mediana de sala: " << report.montosImputados << "\n";
        std::cout << "VPG imputados por mediana de sala:    " << report.vpgsImputados << "\n";
        std::cout << "Filas antes de quitar duplicados: " << report.filasLeidas << "\n";
        std::cout << "Filas despues de quitar duplicados: " << result.filas.size() << "\n";

        std::cout << "\n=== Validaciones post-limpieza ===\n";
        std::cout << "lead_id unico: " << (report.idsUnicos ? "OK" : "FALLO") << "\n";
        std::cout << "monto minimo: " << report.montoMinimo << " (debe ser > 0)\n";
        std::cout << "vpg minimo:   " << report.vpgMinimo << " (debe ser > 0)\n";
        std::cout << "Registros con venta pero sin monto: " << report.ventasSinMonto << "\n";
        std::cout << "Registros sin venta pero con monto: " << report.montosSinVenta
                  << " (debe ser 0)\n";

        cleaning::writeCleanedCsv(outPath, result.filas);
        std::cout << "\nGuardado en " << outPath.string() << "\n";

        return report.valido() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Error en la limpieza: " << error.what() << "\n";
        return 1;
    }
}
