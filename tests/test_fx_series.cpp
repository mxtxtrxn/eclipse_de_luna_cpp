#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "fx_series.hpp"
#include "test_framework.hpp"
#include "test_utils.hpp"

namespace {

testutil::fs::path escribirCsv(const std::string& name, const std::string& content) {
    const testutil::fs::path dir = testutil::tempDir(name);
    const testutil::fs::path path = dir / "serie.csv";
    testutil::writeText(path, content);
    return path;
}

std::vector<fx::Obs> serieSimple(std::size_t n) {
    std::vector<fx::Obs> obs;
    for (std::size_t i = 0; i < n; ++i) {
        fx::Obs item;
        item.year = 2020;
        item.month = static_cast<int>(i % 12) + 1;
        item.tipoCambio = static_cast<double>(i) + 1.0;
        item.tasaBanxico = 5.0;
        item.tasaBce = 2.0;
        item.inflacionMx = 3.0;
        item.inflacionEz = 4.0;
        obs.push_back(item);
    }
    return obs;
}

} // namespace

TEST_CASE(fx_read_history_archivo_real) {
    const testutil::fs::path path =
        testutil::projectRoot() / "data" / "fx_historico_eur.csv";
    const std::vector<fx::Obs> obs = fx::readHistory(path);

    CHECK_EQ(obs.size(), std::size_t{132});
    CHECK_EQ(obs.front().year, 2014);
    CHECK_EQ(obs.front().month, 1);
    CHECK_EQ(obs.back().year, 2024);
    CHECK_EQ(obs.back().month, 12);
    for (const auto& item : obs) CHECK(item.tipoCambio > 0.0);
}

TEST_CASE(fx_interpola_hueco_interior) {
    const testutil::fs::path path = escribirCsv("fx_interior",
        "fecha,tipo_cambio,tasa_banxico,tasa_bce,inflacion_mx,inflacion_ez\n"
        "2020-01-01,10,1,1,1,1\n"
        "2020-02-01,,1,1,1,1\n"
        "2020-03-01,20,1,1,1,1\n");

    const std::vector<fx::Obs> obs = fx::readHistory(path);
    CHECK_NEAR(obs[1].tipoCambio, 15.0, 1e-12);
}

TEST_CASE(fx_rellena_huecos_en_los_bordes) {
    const testutil::fs::path path = escribirCsv("fx_bordes",
        "fecha,tipo_cambio,tasa_banxico,tasa_bce,inflacion_mx,inflacion_ez\n"
        "2020-01-01,,1,1,1,1\n"
        "2020-02-01,10,1,1,1,1\n"
        "2020-03-01,20,1,1,1,1\n"
        "2020-04-01,,1,1,1,1\n"
        "2020-05-01,,1,1,1,1\n");

    const std::vector<fx::Obs> obs = fx::readHistory(path);
    CHECK_NEAR(obs[0].tipoCambio, 10.0, 1e-12);
    CHECK_NEAR(obs[3].tipoCambio, 20.0, 1e-12);
    CHECK_NEAR(obs[4].tipoCambio, 20.0, 1e-12);
}

TEST_CASE(fx_build_features_cantidad_y_lags) {
    const std::vector<fx::FeatureRow> features = fx::buildFeatures(serieSimple(13));
    CHECK_EQ(features.size(), std::size_t{1});
    CHECK_EQ(features[0].t, 12);
    CHECK_EQ(features[0].x.size(), std::size_t{11});
    CHECK_NEAR(features[0].x[0], 12.0, 1e-12);
    CHECK_NEAR(features[0].x[1], 11.0, 1e-12);
    CHECK_NEAR(features[0].x[2], 10.0, 1e-12);
    CHECK_NEAR(features[0].x[3], 7.0, 1e-12);
    CHECK_NEAR(features[0].x[4], 1.0, 1e-12);
}

TEST_CASE(fx_build_features_variables_explicativas) {
    const std::vector<fx::FeatureRow> features = fx::buildFeatures(serieSimple(13));
    CHECK_NEAR(features[0].x[5], 3.0, 1e-12);
    CHECK_NEAR(features[0].x[6], 3.0, 1e-12);
    CHECK_NEAR(features[0].x[7], 4.0, 1e-12);
    CHECK_NEAR(features[0].x[8], 0.5, 1e-12);
    CHECK_NEAR(features[0].x[9], std::sqrt(3.0) / 2.0, 1e-12);
    CHECK_NEAR(features[0].x[10], 12.0, 1e-12);
    CHECK_NEAR(features[0].y, 13.0, 1e-12);
}

TEST_CASE(fx_build_features_serie_corta_vacia) {
    CHECK(fx::buildFeatures(serieSimple(12)).empty());
    CHECK(fx::buildFeatures(serieSimple(5)).empty());
}

TEST_CASE(fx_is_train_year) {
    CHECK(fx::isTrainYear(2014));
    CHECK(fx::isTrainYear(2022));
    CHECK(!fx::isTrainYear(2023));
    CHECK(!fx::isTrainYear(2024));
}

TEST_CASE(fx_format_fixed4) {
    CHECK_EQ(fx::formatFixed4(1.25), std::string("1.2500"));
    CHECK_EQ(fx::formatFixed4(21.64329), std::string("21.6433"));
}

TEST_CASE(fx_read_history_sin_columnas_lanza) {
    const testutil::fs::path path = escribirCsv("fx_incompleto",
        "fecha,tipo_cambio\n2020-01-01,10\n");
    CHECK_THROWS(fx::readHistory(path));
}
