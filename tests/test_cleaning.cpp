#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "cleaning.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

namespace {

csvutil::Table tablaPequena() {
    csvutil::Table table;
    table.header = {"lead_id", "fuente", "fecha_creacion", "fecha_venta",
                    "monto", "vpg", "sala", "comentarios"};
    table.rows = {
        {"1", "FB", "2024-01-01", "2024-01-10", "100", "10", "cancun", ""},
        {"2", "facebook", "2024-01-02", "2024-01-11", "200", "20", "CANCUN", "ok"},
        {"3", "desconocido", "2024-01-03", "2024-01-12", "", "", "cancun", "x"},
        {"4", "N/A", "2024-01-04", "", "", "", "cabo", "hola"},
        {"4", "N/A", "2024-01-04", "", "", "", "cabo", "hola"},
        {"5", "social", "2024-01-05", "", "", "", "cancun", "sin venta"}
    };
    return table;
}

} // namespace

TEST_CASE(cleaning_canonical_fuente) {
    CHECK_EQ(cleaning::canonicalFuente("FB"), std::string("Social"));
    CHECK_EQ(cleaning::canonicalFuente("facebook"), std::string("Social"));
    CHECK_EQ(cleaning::canonicalFuente("PROVEEDOR A"), std::string("Proveedor A"));
    CHECK_EQ(cleaning::canonicalFuente("ProveedorA"), std::string("Proveedor A"));
    CHECK_EQ(cleaning::canonicalFuente("Portal Web"), std::string("Portal Web"));
    CHECK_EQ(cleaning::canonicalFuente("Calle TMS"), std::string("Calle"));
    CHECK_EQ(cleaning::canonicalFuente("rci"), std::string("RCI"));
    CHECK_EQ(cleaning::canonicalFuente("ota"), std::string("OTA"));
}

TEST_CASE(cleaning_canonical_fuente_desconocida) {
    CHECK_EQ(cleaning::canonicalFuente("N/A"), std::string("Desconocido"));
    CHECK_EQ(cleaning::canonicalFuente(""), std::string("Desconocido"));
    CHECK_EQ(cleaning::canonicalFuente("   "), std::string("Desconocido"));
}

TEST_CASE(cleaning_canonical_sala) {
    CHECK_EQ(cleaning::canonicalSala("CANCÚN"), std::string("Cancún"));
    CHECK_EQ(cleaning::canonicalSala("pdc"), std::string("Playa del Carmen"));
    CHECK_EQ(cleaning::canonicalSala("PV"), std::string("Puerto Vallarta"));
    CHECK_EQ(cleaning::canonicalSala("cabo"), std::string("Los Cabos"));
}

TEST_CASE(cleaning_canonical_sala_desconocida) {
    CHECK_EQ(cleaning::canonicalSala("Madrid"), std::string("Desconocido"));
}

TEST_CASE(cleaning_parse_optional_double) {
    CHECK(!cleaning::parseOptionalDouble("").has_value());
    CHECK(!cleaning::parseOptionalDouble("abc").has_value());
    CHECK_NEAR(*cleaning::parseOptionalDouble("12.5"), 12.5, 1e-12);
    CHECK_NEAR(*cleaning::parseOptionalDouble("0"), 0.0, 1e-12);
}

TEST_CASE(cleaning_format_number_y_optional) {
    CHECK_EQ(cleaning::formatNumber(150.0), std::string("150.00"));
    CHECK_EQ(cleaning::formatNumber(12.5), std::string("12.50"));
    CHECK_EQ(cleaning::formatOptional(std::nullopt), std::string(""));
    CHECK_EQ(cleaning::formatOptional(std::optional<double>(7.0)), std::string("7.00"));
}

TEST_CASE(cleaning_tabla_pequena_normaliza_e_imputa) {
    const cleaning::Result result = cleaning::cleanTable(tablaPequena());

    CHECK_EQ(result.reporte.filasLeidas, std::size_t{6});
    CHECK_EQ(result.reporte.duplicadosEliminados, std::size_t{1});
    CHECK_EQ(result.reporte.montosImputados, std::size_t{1});
    CHECK_EQ(result.reporte.vpgsImputados, std::size_t{1});
    CHECK(result.reporte.idsUnicos);
    CHECK_EQ(result.reporte.ventasSinMonto, std::size_t{0});
    CHECK_EQ(result.reporte.montosSinVenta, std::size_t{0});
    CHECK_NEAR(result.reporte.montoMinimo, 100.0, 1e-12);
    CHECK_NEAR(result.reporte.vpgMinimo, 10.0, 1e-12);
    CHECK(result.reporte.valido());
    CHECK_EQ(result.filas.size(), std::size_t{5});
}

TEST_CASE(cleaning_tabla_pequena_valores_esperados) {
    const cleaning::Result result = cleaning::cleanTable(tablaPequena());

    CHECK_EQ(result.filas[0].fuente, std::string("Social"));
    CHECK_EQ(result.filas[0].sala, std::string("Cancún"));
    CHECK_EQ(result.filas[0].comentarios, std::string("Sin comentarios"));

    CHECK_EQ(result.filas[2].sala, std::string("Cancún"));
    CHECK_NEAR(*result.filas[2].monto, 150.0, 1e-12);
    CHECK_NEAR(*result.filas[2].vpg, 15.0, 1e-12);

    CHECK_EQ(result.filas[3].fuente, std::string("Desconocido"));
    CHECK_EQ(result.filas[3].sala, std::string("Los Cabos"));
    CHECK(!result.filas[3].monto.has_value());
    CHECK(!result.filas[3].vpg.has_value());
}

TEST_CASE(cleaning_tabla_sin_columna_obligatoria_lanza) {
    csvutil::Table table;
    table.header = {"lead_id", "fuente"};
    table.rows = {{"1", "FB"}};
    CHECK_THROWS(cleaning::cleanTable(table));
}

TEST_CASE(cleaning_fila_con_columnas_inesperadas_lanza) {
    csvutil::Table table = tablaPequena();
    table.rows[0].pop_back();
    CHECK_THROWS(cleaning::cleanTable(table));
}

TEST_CASE(cleaning_archivo_real_produce_el_csv_limpio_guardado) {
    const testutil::fs::path root = testutil::projectRoot();
    const cleaning::Result result =
        cleaning::cleanFile(root / "data" / "leads_ventas.csv");

    CHECK_EQ(result.reporte.filasLeidas, std::size_t{5250});
    CHECK_EQ(result.reporte.duplicadosEliminados, std::size_t{250});
    CHECK_EQ(result.filas.size(), std::size_t{5000});
    CHECK(result.reporte.idsUnicos);
    CHECK_EQ(result.reporte.ventasSinMonto, std::size_t{0});
    CHECK_EQ(result.reporte.montosSinVenta, std::size_t{0});
    CHECK(result.reporte.montoMinimo > 0.0);
    CHECK(result.reporte.vpgMinimo > 0.0);
    CHECK(result.reporte.valido());

    const testutil::fs::path out =
        testutil::tempDir("cleaning_integracion") / "leads_ventas_clean.csv";
    cleaning::writeCleanedCsv(out, result.filas);
    CHECK_EQ(testutil::readText(out),
             testutil::readText(root / "data" / "leads_ventas_clean.csv"));
}
