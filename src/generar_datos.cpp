// generar_datos.cpp
// Genera datos sinteticos para el Proyecto Eclipse de Luna (version EUR).
// Salida: data/leads_ventas.csv y data/fx_historico_eur.csv
//
// La generacion vive en include/data_generator.hpp. Este programa solo
// orquesta la escritura con las semillas fijas que hacen reproducible el
// dataset.

#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "data_generator.hpp"

int main() {
    try {
        std::filesystem::create_directories("data");

        const std::size_t leads = datagen::generateLeadsCsv("data/leads_ventas.csv", 42);
        std::cout << "leads_ventas.csv generado: " << leads
                  << " registros en data/leads_ventas.csv\n";

        const std::size_t fxRows = datagen::generateFxCsv("data/fx_historico_eur.csv", 7);
        std::cout << "fx_historico_eur.csv generado: " << fxRows
                  << " registros en data/fx_historico_eur.csv\n";

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error al generar los datos: " << error.what() << "\n";
        return 1;
    }
}
