#include <string>

#include "test_framework.hpp"
#include "text_utils.hpp"

TEST_CASE(text_trim_quita_espacios_extremos) {
    CHECK_EQ(textutil::trim("  hola  "), std::string("hola"));
    CHECK_EQ(textutil::trim("\t\nhola\r\n"), std::string("hola"));
}

TEST_CASE(text_trim_de_cadena_vacia) {
    CHECK_EQ(textutil::trim(""), std::string(""));
    CHECK_EQ(textutil::trim("   \t\n"), std::string(""));
}

TEST_CASE(text_lower_ascii) {
    CHECK_EQ(textutil::toLowerAscii("ABC Def"), std::string("abc def"));
}

TEST_CASE(text_strip_accents_vocales) {
    CHECK_EQ(textutil::stripAccentsUtf8("café"), std::string("cafe"));
    CHECK_EQ(textutil::stripAccentsUtf8("México"), std::string("Mexico"));
    CHECK_EQ(textutil::stripAccentsUtf8("pirámide"), std::string("piramide"));
    CHECK_EQ(textutil::stripAccentsUtf8("canción"), std::string("cancion"));
}

TEST_CASE(text_strip_accents_ene_y_dieresis) {
    CHECK_EQ(textutil::stripAccentsUtf8("Ñandú"), std::string("Nandu"));
    CHECK_EQ(textutil::stripAccentsUtf8("über"), std::string("uber"));
}

TEST_CASE(text_strip_accents_mayusculas) {
    CHECK_EQ(textutil::stripAccentsUtf8("ÁÉÍÓÚ"), std::string("AEIOU"));
}

TEST_CASE(text_strip_accents_conserva_otros_bytes) {
    CHECK_EQ(textutil::stripAccentsUtf8("café €"), std::string("cafe €"));
}

TEST_CASE(text_normalize_combina_trim_lower_y_acentos) {
    CHECK_EQ(textutil::normalize("  Playa del CARMEN  "), std::string("playa del carmen"));
    CHECK_EQ(textutil::normalize("Cancún"), std::string("cancun"));
    CHECK_EQ(textutil::normalize("CANCÚN"), std::string("cancun"));
}

TEST_CASE(text_normalize_conserva_digitos_y_guiones) {
    CHECK_EQ(textutil::normalize("  N/A-2024 "), std::string("n/a-2024"));
}

TEST_CASE(text_normalize_de_cadena_vacia) {
    CHECK_EQ(textutil::normalize(""), std::string(""));
}
