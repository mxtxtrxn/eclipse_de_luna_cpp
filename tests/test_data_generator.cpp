#include <cstddef>
#include <string>
#include <unordered_set>

#include "csv.hpp"
#include "data_generator.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

TEST_CASE(datagen_leads_genera_5250_filas) {
    const testutil::fs::path dir = testutil::tempDir("datagen_leads");
    const testutil::fs::path path = dir / "leads.csv";
    const std::size_t rows = datagen::generateLeadsCsv(path, 42);
    CHECK_EQ(rows, std::size_t{5250});

    const csvutil::Table table = csvutil::readCsv(path.string());
    CHECK_EQ(table.header.size(), std::size_t{8});
    CHECK_EQ(table.rows.size(), std::size_t{5250});
}

TEST_CASE(datagen_leads_conserva_5000_ids_unicos) {
    const testutil::fs::path dir = testutil::tempDir("datagen_ids");
    const testutil::fs::path path = dir / "leads.csv";
    datagen::generateLeadsCsv(path, 42);

    const csvutil::Table table = csvutil::readCsv(path.string());
    std::unordered_set<std::string> ids;
    for (const auto& row : table.rows) ids.insert(row[0]);
    CHECK_EQ(ids.size(), std::size_t{5000});
}

TEST_CASE(datagen_leads_es_reproducible) {
    const testutil::fs::path dir = testutil::tempDir("datagen_leads_repro");
    const testutil::fs::path first = dir / "a.csv";
    const testutil::fs::path second = dir / "b.csv";
    datagen::generateLeadsCsv(first, 42);
    datagen::generateLeadsCsv(second, 42);
    CHECK_EQ(testutil::readText(first), testutil::readText(second));
}

TEST_CASE(datagen_leads_cambia_con_la_semilla) {
    const testutil::fs::path dir = testutil::tempDir("datagen_leads_seed");
    const testutil::fs::path first = dir / "a.csv";
    const testutil::fs::path second = dir / "b.csv";
    datagen::generateLeadsCsv(first, 42);
    datagen::generateLeadsCsv(second, 43);
    CHECK_NE(testutil::readText(first), testutil::readText(second));
}

TEST_CASE(datagen_fx_genera_132_meses_con_dos_nulos) {
    const testutil::fs::path dir = testutil::tempDir("datagen_fx");
    const testutil::fs::path path = dir / "fx.csv";
    const std::size_t rows = datagen::generateFxCsv(path, 7);
    CHECK_EQ(rows, std::size_t{132});

    const csvutil::Table table = csvutil::readCsv(path.string());
    CHECK_EQ(table.rows.size(), std::size_t{132});
    std::size_t nulos = 0;
    for (const auto& row : table.rows) {
        if (row[1].empty()) ++nulos;
    }
    CHECK_EQ(nulos, std::size_t{2});
}

TEST_CASE(datagen_fx_es_reproducible) {
    const testutil::fs::path dir = testutil::tempDir("datagen_fx_repro");
    const testutil::fs::path first = dir / "a.csv";
    const testutil::fs::path second = dir / "b.csv";
    datagen::generateFxCsv(first, 7);
    datagen::generateFxCsv(second, 7);
    CHECK_EQ(testutil::readText(first), testutil::readText(second));
}

TEST_CASE(datagen_coincide_con_los_datos_guardados) {
    const testutil::fs::path root = testutil::projectRoot();
    const testutil::fs::path dir = testutil::tempDir("datagen_repo");

    const testutil::fs::path leads = dir / "leads.csv";
    const testutil::fs::path fxPath = dir / "fx.csv";
    datagen::generateLeadsCsv(leads, 42);
    datagen::generateFxCsv(fxPath, 7);

    CHECK_EQ(testutil::readText(leads),
             testutil::readText(root / "data" / "leads_ventas.csv"));
    CHECK_EQ(testutil::readText(fxPath),
             testutil::readText(root / "data" / "fx_historico_eur.csv"));
}

TEST_CASE(datagen_leads_no_contiene_correos) {
    const testutil::fs::path dir = testutil::tempDir("datagen_pii");
    const testutil::fs::path path = dir / "leads.csv";
    datagen::generateLeadsCsv(path, 42);
    CHECK(testutil::readText(path).find('@') == std::string::npos);
}
