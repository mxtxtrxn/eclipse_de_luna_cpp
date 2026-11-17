#include <cmath>
#include <vector>

#include "linalg.hpp"
#include "test_framework.hpp"

TEST_CASE(linalg_transpose) {
    const linalg::Mat a = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
    const linalg::Mat t = linalg::transpose(a);
    CHECK_EQ(t.size(), std::size_t{3});
    CHECK_EQ(t[0].size(), std::size_t{2});
    CHECK_NEAR(t[0][0], 1.0, 1e-12);
    CHECK_NEAR(t[1][0], 2.0, 1e-12);
    CHECK_NEAR(t[2][1], 6.0, 1e-12);
}

TEST_CASE(linalg_transpose_vacia) {
    CHECK(linalg::transpose({}).empty());
}

TEST_CASE(linalg_matmul) {
    const linalg::Mat a = {{1.0, 2.0}, {3.0, 4.0}};
    const linalg::Mat b = {{5.0, 6.0}, {7.0, 8.0}};
    const linalg::Mat c = linalg::matmul(a, b);
    CHECK_NEAR(c[0][0], 19.0, 1e-12);
    CHECK_NEAR(c[0][1], 22.0, 1e-12);
    CHECK_NEAR(c[1][0], 43.0, 1e-12);
    CHECK_NEAR(c[1][1], 50.0, 1e-12);
}

TEST_CASE(linalg_matvec) {
    const linalg::Mat a = {{1.0, 2.0}, {3.0, 4.0}};
    const linalg::Vec x = {5.0, 6.0};
    const linalg::Vec y = linalg::matvec(a, x);
    CHECK_NEAR(y[0], 17.0, 1e-12);
    CHECK_NEAR(y[1], 39.0, 1e-12);
}

TEST_CASE(linalg_matmul_dimensiones_incompatibles_lanza) {
    const linalg::Mat a = {{1.0, 2.0}};
    const linalg::Mat b = {{1.0, 2.0}};
    CHECK_THROWS(linalg::matmul(a, b));
}

TEST_CASE(linalg_matvec_dimensiones_incompatibles_lanza) {
    const linalg::Mat a = {{1.0, 2.0}};
    CHECK_THROWS(linalg::matvec(a, {1.0, 2.0, 3.0}));
}

TEST_CASE(linalg_solve_diagonal) {
    const linalg::Vec x = linalg::solve({{2.0, 0.0}, {0.0, 4.0}}, {6.0, 8.0});
    CHECK_NEAR(x[0], 3.0, 1e-12);
    CHECK_NEAR(x[1], 2.0, 1e-12);
}

TEST_CASE(linalg_solve_sistema_2x2) {
    const linalg::Vec x = linalg::solve({{2.0, 1.0}, {1.0, 3.0}}, {5.0, 10.0});
    CHECK_NEAR(x[0], 1.0, 1e-12);
    CHECK_NEAR(x[1], 3.0, 1e-12);
}

TEST_CASE(linalg_solve_singular_no_lanza) {
    const linalg::Vec x = linalg::solve({{1.0, 2.0}, {2.0, 4.0}}, {1.0, 2.0});
    CHECK_EQ(x.size(), std::size_t{2});
    CHECK(std::isfinite(x[0]));
    CHECK(std::isfinite(x[1]));
}

TEST_CASE(linalg_ols_recupera_recta) {
    const linalg::Mat x = {{1.0, 0.0}, {1.0, 1.0}, {1.0, 2.0}, {1.0, 3.0}};
    const linalg::Vec y = {1.0, 3.0, 5.0, 7.0};
    const linalg::Vec beta = linalg::olsFit(x, y);
    CHECK_NEAR(beta[0], 1.0, 1e-12);
    CHECK_NEAR(beta[1], 2.0, 1e-12);
}

TEST_CASE(linalg_ols_sin_intercepto) {
    const linalg::Mat x = {{1.0}, {2.0}, {3.0}};
    const linalg::Vec y = {2.0, 4.0, 6.0};
    const linalg::Vec beta = linalg::olsFit(x, y);
    CHECK_NEAR(beta[0], 2.0, 1e-12);
}
