#include <cmath>
#include <vector>

#include "mlp.hpp"
#include "test_framework.hpp"

namespace {

mlp::Mat matrizSimple() {
    return {{1.0, 10.0}, {3.0, 20.0}, {5.0, 30.0}};
}

double mse(const mlp::Mlp& model, const mlp::Mat& x, const mlp::Vec& y) {
    double total = 0.0;
    for (std::size_t i = 0; i < x.size(); ++i) {
        const double error = model.predict(x[i]) - y[i];
        total += error * error;
    }
    return total / static_cast<double>(x.size());
}

} // namespace

TEST_CASE(mlp_scaler_centra_y_escala) {
    const mlp::Mat x = matrizSimple();
    mlp::Scaler scaler;
    scaler.fit(x);

    const mlp::Mat z = scaler.transform(x);
    const double media0 = (z[0][0] + z[1][0] + z[2][0]) / 3.0;
    const double media1 = (z[0][1] + z[1][1] + z[2][1]) / 3.0;
    CHECK_NEAR(media0, 0.0, 1e-12);
    CHECK_NEAR(media1, 0.0, 1e-12);

    const double var0 = (z[0][0] * z[0][0] + z[1][0] * z[1][0] + z[2][0] * z[2][0]) / 3.0;
    CHECK_NEAR(var0, 1.0, 1e-12);
}

TEST_CASE(mlp_scaler_columna_constante_no_divide_entre_cero) {
    const mlp::Mat x = {{1.0, 5.0}, {2.0, 5.0}, {3.0, 5.0}};
    mlp::Scaler scaler;
    scaler.fit(x);
    const mlp::Mat z = scaler.transform(x);
    CHECK_NEAR(z[0][1], 0.0, 1e-12);
    CHECK_NEAR(z[2][1], 0.0, 1e-12);
}

TEST_CASE(mlp_scaler_target_ida_y_vuelta) {
    mlp::ScalerTarget scaler;
    scaler.fit({10.0, 20.0, 30.0});
    for (double value : {10.0, 20.0, 30.0}) {
        CHECK_NEAR(scaler.inverse(scaler.transform(value)), value, 1e-12);
    }
}

TEST_CASE(mlp_scaler_target_constante_es_estable) {
    mlp::ScalerTarget scaler;
    scaler.fit({7.0, 7.0, 7.0});
    CHECK_NEAR(scaler.transform(7.0), 0.0, 1e-12);
}

TEST_CASE(mlp_misma_semilla_es_determinista) {
    const mlp::Mat x = {{0.0}, {0.5}, {1.0}, {1.5}};
    const mlp::Vec y = {1.0, 2.0, 3.0, 4.0};

    mlp::Mlp a(1, 3, 7);
    mlp::Mlp b(1, 3, 7);
    a.train(x, y, 40, 0.01);
    b.train(x, y, 40, 0.01);

    for (const auto& row : x) {
        CHECK_NEAR(a.predict(row), b.predict(row), 0.0);
    }
}

TEST_CASE(mlp_semillas_distintas_dan_predicciones_distintas) {
    const mlp::Vec row = {0.25, -0.5};
    mlp::Mlp a(2, 3, 1);
    mlp::Mlp b(2, 3, 2);
    CHECK(std::fabs(a.predict(row) - b.predict(row)) > 1e-9);
}

TEST_CASE(mlp_entrenamiento_reduce_el_error) {
    mlp::Mat x;
    mlp::Vec y;
    for (int i = 0; i < 40; ++i) {
        const double value = -1.0 + 2.0 * static_cast<double>(i) / 39.0;
        x.push_back({value});
        y.push_back(2.0 * value + 1.0);
    }

    mlp::Mlp model(1, 4, 123);
    const double antes = mse(model, x, y);
    model.train(x, y, 800, 0.02);
    const double despues = mse(model, x, y);

    CHECK(despues < antes);
    CHECK(despues < antes * 0.5);
}

TEST_CASE(mlp_entrenamiento_con_dimensiones_invalidas_lanza) {
    mlp::Mlp model(2, 2, 1);
    CHECK_THROWS(model.train({{1.0}}, {1.0}, 1));
}

TEST_CASE(mlp_prediccion_es_finita) {
    mlp::Mlp model(2, 2, 3);
    const double value = model.predict({1.0, -1.0});
    CHECK(std::isfinite(value));
}
