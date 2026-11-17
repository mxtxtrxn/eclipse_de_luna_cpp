#include <cstdint>
#include <string>

#include "date_utils.hpp"
#include "test_framework.hpp"

TEST_CASE(date_epoca_1970_es_cero) {
    CHECK_EQ(dateutil::daysFromCivil(1970, 1, 1), std::int64_t{0});
}

TEST_CASE(date_2000_01_01_es_10957) {
    CHECK_EQ(dateutil::daysFromCivil(2000, 1, 1), std::int64_t{10957});
}

TEST_CASE(date_ida_y_vuelta_incluye_bisiesto) {
    const dateutil::Date original{2024, 2, 29};
    const dateutil::Date vuelta = dateutil::civilFromDays(
        dateutil::daysFromCivil(2024, 2, 29));
    CHECK_EQ(vuelta.year, 2024);
    CHECK_EQ(vuelta.month, 2);
    CHECK_EQ(vuelta.day, 29);
    CHECK_EQ(dateutil::toIsoString(original), std::string("2024-02-29"));
}

TEST_CASE(date_add_days_cruza_fin_de_anio) {
    const dateutil::Date resultado = dateutil::addDays({2023, 12, 31}, 1);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2024-01-01"));
}

TEST_CASE(date_add_days_entra_al_29_de_febrero) {
    const dateutil::Date resultado = dateutil::addDays({2024, 2, 28}, 1);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2024-02-29"));
}

TEST_CASE(date_add_days_anio_no_bisiesto) {
    const dateutil::Date resultado = dateutil::addDays({2023, 2, 28}, 1);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2023-03-01"));
}

TEST_CASE(date_add_days_negativo) {
    const dateutil::Date resultado = dateutil::addDays({2024, 1, 1}, -1);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2023-12-31"));
}

TEST_CASE(date_add_months_cruza_anio) {
    const dateutil::Date resultado = dateutil::addMonths({2024, 11, 1}, 3);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2025-02-01"));
}

TEST_CASE(date_add_months_doce_meses) {
    const dateutil::Date resultado = dateutil::addMonths({2024, 1, 15}, 12);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2025-01-15"));
}

TEST_CASE(date_add_months_negativo) {
    const dateutil::Date resultado = dateutil::addMonths({2024, 1, 1}, -1);
    CHECK_EQ(dateutil::toIsoString(resultado), std::string("2023-12-01"));
}

TEST_CASE(date_to_iso_rellena_con_ceros) {
    const dateutil::Date fecha{7, 1, 2};
    CHECK_EQ(dateutil::toIsoString(fecha), std::string("0007-01-02"));
}
