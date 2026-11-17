#include <vector>

#include "stats.hpp"
#include "test_framework.hpp"

TEST_CASE(stats_mean_basica) {
    CHECK_NEAR(stats::mean({1.0, 2.0, 3.0, 4.0}), 2.5, 1e-12);
}

TEST_CASE(stats_mean_vacia_es_cero) {
    CHECK_NEAR(stats::mean({}), 0.0, 1e-12);
}

TEST_CASE(stats_median_impar) {
    CHECK_NEAR(stats::median({5.0, 1.0, 3.0}), 3.0, 1e-12);
}

TEST_CASE(stats_median_par) {
    CHECK_NEAR(stats::median({4.0, 1.0, 3.0, 2.0}), 2.5, 1e-12);
}

TEST_CASE(stats_median_vacia_lanza) {
    CHECK_THROWS(stats::median({}));
}

TEST_CASE(stats_stddev_muestral) {
    CHECK_NEAR(stats::stddev({2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0}),
               2.138089935299395, 1e-9);
}

TEST_CASE(stats_stddev_con_menos_de_dos_es_cero) {
    CHECK_NEAR(stats::stddev({}), 0.0, 1e-12);
    CHECK_NEAR(stats::stddev({3.0}), 0.0, 1e-12);
}

TEST_CASE(stats_evaluate_mae_rmse_mape) {
    const stats::Metrics metrics = stats::evaluate({1.0, 2.0, 4.0}, {1.0, 2.0, 2.0});
    CHECK_NEAR(metrics.mae, 2.0 / 3.0, 1e-12);
    CHECK_NEAR(metrics.rmse, std::sqrt(4.0 / 3.0), 1e-12);
    CHECK_NEAR(metrics.mape, 100.0 / 6.0, 1e-12);
}

TEST_CASE(stats_evaluate_perfecto_es_cero) {
    const stats::Metrics metrics = stats::evaluate({1.0, 2.0}, {1.0, 2.0});
    CHECK_NEAR(metrics.mae, 0.0, 1e-12);
    CHECK_NEAR(metrics.rmse, 0.0, 1e-12);
    CHECK_NEAR(metrics.mape, 0.0, 1e-12);
}

TEST_CASE(stats_evaluate_tamanos_distintos_lanza) {
    CHECK_THROWS(stats::evaluate({1.0, 2.0}, {1.0}));
}

TEST_CASE(stats_evaluate_vacio_lanza) {
    CHECK_THROWS(stats::evaluate({}, {}));
}
