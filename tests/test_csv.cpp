#include <cstddef>
#include <string>

#include "csv.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

TEST_CASE(csv_parse_campos_simples) {
    const csvutil::Row row = csvutil::parseLine("a,b,c");
    CHECK_EQ(row.size(), std::size_t{3});
    CHECK_EQ(row[0], std::string("a"));
    CHECK_EQ(row[1], std::string("b"));
    CHECK_EQ(row[2], std::string("c"));
}

TEST_CASE(csv_parse_campo_con_coma_entre_comillas) {
    const csvutil::Row row = csvutil::parseLine("x,\"a,b\",y");
    CHECK_EQ(row.size(), std::size_t{3});
    CHECK_EQ(row[1], std::string("a,b"));
}

TEST_CASE(csv_parse_comillas_escapadas) {
    const csvutil::Row row = csvutil::parseLine("\"di \"\"hola\"\"\",2");
    CHECK_EQ(row.size(), std::size_t{2});
    CHECK_EQ(row[0], std::string("di \"hola\""));
    CHECK_EQ(row[1], std::string("2"));
}

TEST_CASE(csv_parse_campos_vacios) {
    const csvutil::Row row = csvutil::parseLine("a,,c");
    CHECK_EQ(row.size(), std::size_t{3});
    CHECK(row[1].empty());
}

TEST_CASE(csv_parse_coma_final) {
    const csvutil::Row row = csvutil::parseLine("a,b,");
    CHECK_EQ(row.size(), std::size_t{3});
    CHECK(row[2].empty());
}

TEST_CASE(csv_strip_cr_quita_finales) {
    CHECK_EQ(csvutil::stripCR("abc\r\n"), std::string("abc"));
    CHECK_EQ(csvutil::stripCR("abc"), std::string("abc"));
    CHECK_EQ(csvutil::stripCR("\n\n"), std::string(""));
}

TEST_CASE(csv_colindex_encuentra_y_falla) {
    csvutil::Table table;
    table.header = {"lead_id", "monto", "sala"};
    CHECK_EQ(table.colIndex("monto"), 1);
    CHECK_EQ(table.colIndex("sala"), 2);
    CHECK_EQ(table.colIndex("inexistente"), -1);
}

TEST_CASE(csv_at_lee_columna_y_lanza_si_falta) {
    csvutil::Table table;
    table.header = {"a", "b"};
    table.rows = {{"1", "2"}};
    CHECK_EQ(table.at(0, "b"), std::string("2"));
    CHECK_THROWS(table.at(0, "z"));
}

TEST_CASE(csv_read_file_con_crlf_y_comillas) {
    const testutil::fs::path dir = testutil::tempDir("csv_read");
    const testutil::fs::path path = dir / "datos.csv";
    testutil::writeText(path,
        "a,b,c\r\n"
        "1,\"x,y\",3\r\n"
        "\r\n"
        "4,5,6\r\n");

    const csvutil::Table table = csvutil::readCsv(path.string());
    CHECK_EQ(table.header.size(), std::size_t{3});
    CHECK_EQ(table.rows.size(), std::size_t{2});
    CHECK_EQ(table.rows[0][1], std::string("x,y"));
    CHECK_EQ(table.rows[1][2], std::string("6"));
}

TEST_CASE(csv_escape_agrega_comillas_solo_cuando_hace_falta) {
    CHECK_EQ(csvutil::escapeField("abc"), std::string("abc"));
    CHECK_EQ(csvutil::escapeField("a,b"), std::string("\"a,b\""));
    CHECK_EQ(csvutil::escapeField("a\"b"), std::string("\"a\"\"b\""));
}

TEST_CASE(csv_writer_y_reader_ida_y_vuelta) {
    const testutil::fs::path dir = testutil::tempDir("csv_roundtrip");
    const testutil::fs::path path = dir / "salida.csv";
    {
        csvutil::Writer writer(path.string());
        writer.writeRow({"lead_id", "comentario"});
        writer.writeRow({"1", "hola, mundo"});
        writer.writeRow({"2", "dijo \"ok\""});
    }

    const csvutil::Table table = csvutil::readCsv(path.string());
    CHECK_EQ(table.rows.size(), std::size_t{2});
    CHECK_EQ(table.rows[0][1], std::string("hola, mundo"));
    CHECK_EQ(table.rows[1][1], std::string("dijo \"ok\""));
}

TEST_CASE(csv_read_archivo_inexistente_lanza) {
    const testutil::fs::path dir = testutil::tempDir("csv_missing");
    CHECK_THROWS(csvutil::readCsv((dir / "no_existe.csv").string()));
}
