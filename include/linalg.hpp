#pragma once
// linalg.hpp — álgebra lineal mínima para regresión OLS (usada en
// la estimación ARIMA por Hannan-Rissanen). Nada de esto depende
// de librerías externas: son matrices como vector<vector<double>>.

#include <vector>
#include <stdexcept>
#include <cmath>

namespace linalg {

using Vec = std::vector<double>;
using Mat = std::vector<std::vector<double>>;

inline Mat transpose(const Mat& a) {
    if (a.empty()) return {};
    size_t rows = a.size(), cols = a[0].size();
    Mat t(cols, Vec(rows));
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            t[j][i] = a[i][j];
    return t;
}

inline Mat matmul(const Mat& a, const Mat& b) {
    size_t n = a.size(), k = a[0].size(), m = b[0].size();
    if (b.size() != k) throw std::runtime_error("Dimensiones incompatibles en matmul");
    Mat c(n, Vec(m, 0.0));
    for (size_t i = 0; i < n; ++i)
        for (size_t l = 0; l < k; ++l) {
            double aij = a[i][l];
            if (aij == 0.0) continue;
            for (size_t j = 0; j < m; ++j)
                c[i][j] += aij * b[l][j];
        }
    return c;
}

inline Vec matvec(const Mat& a, const Vec& x) {
    size_t n = a.size(), k = a[0].size();
    if (x.size() != k) throw std::runtime_error("Dimensiones incompatibles en matvec");
    Vec y(n, 0.0);
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < k; ++j)
            y[i] += a[i][j] * x[j];
    return y;
}

// Resuelve A·x = b por eliminación gaussiana con pivoteo parcial.
inline Vec solve(Mat a, Vec b) {
    size_t n = a.size();
    for (size_t col = 0; col < n; ++col) {
        size_t pivot = col;
        double best = std::fabs(a[col][col]);
        for (size_t r = col + 1; r < n; ++r) {
            if (std::fabs(a[r][col]) > best) { best = std::fabs(a[r][col]); pivot = r; }
        }
        if (best < 1e-12) continue; // matriz (casi) singular: seguimos lo mejor posible
        std::swap(a[col], a[pivot]);
        std::swap(b[col], b[pivot]);

        double diag = a[col][col];
        if (std::fabs(diag) < 1e-12) continue;
        for (size_t r = 0; r < n; ++r) {
            if (r == col) continue;
            double factor = a[r][col] / diag;
            if (factor == 0.0) continue;
            for (size_t c = col; c < n; ++c) a[r][c] -= factor * a[col][c];
            b[r] -= factor * b[col];
        }
    }
    Vec x(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        double diag = a[i][i];
        x[i] = (std::fabs(diag) < 1e-12) ? 0.0 : b[i] / diag;
    }
    return x;
}

// Regresión OLS: minimiza ||X*beta - y||^2 resolviendo las ecuaciones
// normales (X^T X) beta = X^T y. X incluye ya la columna de 1's si se
// quiere intercepto.
inline Vec olsFit(const Mat& X, const Vec& y) {
    Mat Xt = transpose(X);
    Mat XtX = matmul(Xt, X);
    Vec Xty = matvec(Xt, y);
    return solve(XtX, Xty);
}

} // namespace linalg
