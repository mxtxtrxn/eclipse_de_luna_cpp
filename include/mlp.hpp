#pragma once
// mlp.hpp — perceptron multicapa (1 capa oculta) entrenado desde cero
// con backpropagation + Adam. Es el modelo de Machine Learning que
// reemplaza a XGBoost: en vez de un ensamble de arboles por boosting,
// aqui se usa una red neuronal (otra familia de modelos por completo),
// util para mostrar la dinamica no lineal de las variables de entrada.
//
// Sin librerias externas: los pesos son vector<vector<double>> y el
// entrenamiento es full-batch gradient descent con Adam.

#include <vector>
#include <random>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace mlp {

using Vec = std::vector<double>;
using Mat = std::vector<std::vector<double>>;

inline double tanhAct(double x) { return std::tanh(x); }
inline double tanhDeriv(double y) { return 1.0 - y * y; } // y = tanh(x)

struct Scaler {
    Vec mean, std;

    void fit(const Mat& X) {
        size_t n = X.size(), d = X[0].size();
        mean.assign(d, 0.0);
        std.assign(d, 0.0);
        for (const auto& row : X)
            for (size_t j = 0; j < d; ++j) mean[j] += row[j];
        for (size_t j = 0; j < d; ++j) mean[j] /= static_cast<double>(n);

        for (const auto& row : X)
            for (size_t j = 0; j < d; ++j) std[j] += (row[j] - mean[j]) * (row[j] - mean[j]);
        for (size_t j = 0; j < d; ++j) {
            std[j] = std::sqrt(std[j] / static_cast<double>(n));
            if (std[j] < 1e-9) std[j] = 1.0; // evita division por cero en columnas constantes
        }
    }

    Vec transform(const Vec& row) const {
        Vec out(row.size());
        for (size_t j = 0; j < row.size(); ++j) out[j] = (row[j] - mean[j]) / std[j];
        return out;
    }

    Mat transform(const Mat& X) const {
        Mat out;
        out.reserve(X.size());
        for (const auto& row : X) out.push_back(transform(row));
        return out;
    }
};

struct ScalerTarget {
    double mean = 0.0, std = 1.0;

    void fit(const Vec& y) {
        double n = static_cast<double>(y.size());
        mean = std::accumulate(y.begin(), y.end(), 0.0) / n;
        double acc = 0.0;
        for (double v : y) acc += (v - mean) * (v - mean);
        std = std::sqrt(acc / n);
        if (std < 1e-9) std = 1.0;
    }

    double transform(double v) const { return (v - mean) / std; }
    double inverse(double v) const { return v * std + mean; }
};

class Mlp {
public:
    Mlp(size_t nInput, size_t nHidden, unsigned seed)
        : nInput_(nInput), nHidden_(nHidden) {
        std::mt19937 rng(seed);
        // Inicializacion estilo Xavier/Glorot, adecuada para tanh.
        double limit1 = std::sqrt(6.0 / static_cast<double>(nInput_ + nHidden_));
        double limit2 = std::sqrt(6.0 / static_cast<double>(nHidden_ + 1));
        std::uniform_real_distribution<double> dist1(-limit1, limit1);
        std::uniform_real_distribution<double> dist2(-limit2, limit2);

        W1_.assign(nHidden_, Vec(nInput_));
        for (auto& row : W1_)
            for (double& w : row) w = dist1(rng);
        b1_.assign(nHidden_, 0.0);

        W2_.assign(nHidden_, 0.0);
        for (double& w : W2_) w = dist2(rng);
        b2_ = 0.0;
    }

    double predict(const Vec& x) const {
        Vec h = hidden(x);
        double y = b2_;
        for (size_t j = 0; j < nHidden_; ++j) y += W2_[j] * h[j];
        return y;
    }

    // Entrena con descenso de gradiente full-batch + Adam durante
    // `epochs` iteraciones. Devuelve el historial de la perdida (MSE)
    // cada `logEvery` epochs (solo para diagnostico, no es obligatorio
    // revisarlo).
    void train(const Mat& X, const Vec& y, int epochs, double lr = 0.01) {
        size_t n = X.size();
        if (n == 0 || X[0].size() != nInput_) throw std::runtime_error("Mlp::train: dimensiones invalidas");

        // Momentos de Adam para cada parametro.
        Mat mW1(nHidden_, Vec(nInput_, 0.0)), vW1(nHidden_, Vec(nInput_, 0.0));
        Vec mb1(nHidden_, 0.0), vb1(nHidden_, 0.0);
        Vec mW2(nHidden_, 0.0), vW2(nHidden_, 0.0);
        double mb2 = 0.0, vb2 = 0.0;

        const double beta1 = 0.9, beta2 = 0.999, eps = 1e-8;
        int t = 0;

        for (int epoch = 0; epoch < epochs; ++epoch) {
            Mat gW1(nHidden_, Vec(nInput_, 0.0));
            Vec gb1(nHidden_, 0.0);
            Vec gW2(nHidden_, 0.0);
            double gb2 = 0.0;

            for (size_t i = 0; i < n; ++i) {
                const Vec& x = X[i];
                Vec h = hidden(x);
                double yPred = b2_;
                for (size_t j = 0; j < nHidden_; ++j) yPred += W2_[j] * h[j];

                double err = yPred - y[i]; // d(MSE)/d(yPred) = 2*err/n, se absorbe el 2 en lr

                for (size_t j = 0; j < nHidden_; ++j) gW2[j] += err * h[j];
                gb2 += err;

                for (size_t j = 0; j < nHidden_; ++j) {
                    double dh = err * W2_[j] * tanhDeriv(h[j]);
                    for (size_t k = 0; k < nInput_; ++k) gW1[j][k] += dh * x[k];
                    gb1[j] += dh;
                }
            }

            double invN = 1.0 / static_cast<double>(n);
            ++t;
            double lrT = lr * std::sqrt(1.0 - std::pow(beta2, t)) / (1.0 - std::pow(beta1, t));

            auto adamStep = [&](double& param, double grad, double& m, double& v) {
                grad *= invN;
                m = beta1 * m + (1 - beta1) * grad;
                v = beta2 * v + (1 - beta2) * grad * grad;
                param -= lrT * m / (std::sqrt(v) + eps);
            };

            for (size_t j = 0; j < nHidden_; ++j) {
                for (size_t k = 0; k < nInput_; ++k) adamStep(W1_[j][k], gW1[j][k], mW1[j][k], vW1[j][k]);
                adamStep(b1_[j], gb1[j], mb1[j], vb1[j]);
                adamStep(W2_[j], gW2[j], mW2[j], vW2[j]);
            }
            adamStep(b2_, gb2, mb2, vb2);
        }
    }

private:
    Vec hidden(const Vec& x) const {
        Vec h(nHidden_);
        for (size_t j = 0; j < nHidden_; ++j) {
            double z = b1_[j];
            for (size_t k = 0; k < nInput_; ++k) z += W1_[j][k] * x[k];
            h[j] = tanhAct(z);
        }
        return h;
    }

    size_t nInput_, nHidden_;
    Mat W1_;   // [nHidden x nInput]
    Vec b1_;   // [nHidden]
    Vec W2_;   // [nHidden]
    double b2_;
};

} // namespace mlp
