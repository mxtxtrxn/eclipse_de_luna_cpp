#pragma once
// data_generator.hpp — generacion reproducible de los datos sinteticos.
// Se extrajo del programa principal para poder comprobar la reproducibilidad
// desde las pruebas sin ejecutar el binario completo.

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "csv.hpp"
#include "date_utils.hpp"

namespace datagen {

using dateutil::Date;

namespace detail {

struct LeadRow {
    long leadId = 0;
    std::string fuente;
    Date fechaCreacion{};
    bool tieneVenta = false;
    Date fechaVenta{};
    std::optional<double> monto;
    std::optional<double> vpg;
    std::string sala;
    std::optional<std::string> comentario;
};

struct FxRow {
    Date fecha{};
    double tipoCambio = 0.0;
    double tasaBanxico = 0.0;
    double tasaBce = 0.0;
    double inflacionMx = 0.0;
    double inflacionEz = 0.0;
    bool tipoCambioNulo = false;
};

inline std::string fieldOrEmpty(const std::optional<std::string>& value) {
    return value.has_value() ? *value : std::string();
}

inline std::string numOrEmpty(const std::optional<double>& value) {
    if (!value.has_value()) return std::string();
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << *value;
    return oss.str();
}

inline std::string fixed4(double value) {
    std::ostringstream oss;
    oss.precision(4);
    oss << std::fixed << value;
    return oss.str();
}

} // namespace detail

// Genera data/leads_ventas.csv (tabla legada con datos sucios).
// Devuelve la cantidad de filas escritas, incluidos los duplicados.
inline std::size_t generateLeadsCsv(const std::filesystem::path& outputPath,
                                    unsigned seed) {
    const std::vector<std::string> fuentesRaw = {
        "Proveedor A", "proveedor a", "PROVEEDOR A", "ProveedorA",
        "Portal Web", "portal web", "PORTAL WEB",
        "Social", "social", "Facebook", "FB",
        "Calle", "calle", "Calle TMS",
        "RCI", "rci", "OTA", "ota",
        "Hotel", "hotel", "", "", "N/A"
    };

    const std::vector<std::string> salas = {
        "Cancún", "cancún", "CANCÚN", "Playa del Carmen", "PDC",
        "Puerto Vallarta", "PV", "Los Cabos", "Cabo", "cabo"
    };

    const std::vector<std::string> comentariosOk = {
        "Cliente interesado, pidió información por email.",
        "No contesta llamadas, dejar seguimiento.",
        "Solicitó cotización para 2 adultos y 2 niños.",
        "Muy interesado, agendar presentación.",
        "Pidió hablar con supervisor.",
        "Ya había hecho tour antes.",
        "Prefiere destino de playa.",
        "Objeción de precio, volver a llamar.",
        "Cliente molesto por llamadas repetidas.",
        "Solicitó cancelar su registro."
    };

    const int total = 5000;
    const Date baseFecha{2023, 1, 1};

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> fuenteDist(0, static_cast<int>(fuentesRaw.size()) - 1);
    std::uniform_int_distribution<int> salaDist(0, static_cast<int>(salas.size()) - 1);
    std::uniform_int_distribution<int> comentarioDist(0, static_cast<int>(comentariosOk.size()) - 1);
    std::uniform_int_distribution<int> diasCreacionDist(0, 730);
    std::uniform_int_distribution<int> diasVentaDist(1, 90);
    std::uniform_real_distribution<double> montoDist(15000.0, 250000.0);
    std::uniform_real_distribution<double> vpgDist(500.0, 8000.0);
    std::uniform_real_distribution<double> unif01(0.0, 1.0);

    std::vector<detail::LeadRow> data;
    data.reserve(static_cast<std::size_t>(total));

    for (int i = 0; i < total; ++i) {
        detail::LeadRow row;
        row.leadId = 100000 + i;
        row.fuente = fuentesRaw[static_cast<std::size_t>(fuenteDist(rng))];
        row.fechaCreacion = dateutil::addDays(baseFecha, diasCreacionDist(rng));

        if (unif01(rng) < 0.60) {
            row.tieneVenta = true;
            row.fechaVenta = dateutil::addDays(row.fechaCreacion, diasVentaDist(rng));
            row.monto = std::round(montoDist(rng) * 100.0) / 100.0;
            row.vpg = std::round(vpgDist(rng) * 100.0) / 100.0;
        }

        row.sala = salas[static_cast<std::size_t>(salaDist(rng))];
        if (unif01(rng) < 0.70) {
            row.comentario = comentariosOk[static_cast<std::size_t>(comentarioDist(rng))];
        }

        data.push_back(row);
    }

    // Nulos adicionales en monto/vpg (10% cada uno, sobre filas con venta).
    // Se nulifica antes de duplicar para que cada par de duplicados quede
    // verdaderamente identico.
    for (auto& row : data) {
        if (row.monto.has_value() && unif01(rng) < 0.10) row.monto.reset();
        if (row.vpg.has_value() && unif01(rng) < 0.10) row.vpg.reset();
    }

    // Duplicados intencionales (5% de los registros originales).
    const int nDup = static_cast<int>(total * 0.05);
    std::vector<int> idx(static_cast<std::size_t>(total));
    for (int i = 0; i < total; ++i) idx[static_cast<std::size_t>(i)] = i;
    std::shuffle(idx.begin(), idx.end(), rng);
    for (int k = 0; k < nDup; ++k) {
        data.push_back(data[static_cast<std::size_t>(idx[static_cast<std::size_t>(k)])]);
    }

    csvutil::Writer writer(outputPath.string());
    writer.writeRow({"lead_id", "fuente", "fecha_creacion", "fecha_venta",
                     "monto", "vpg", "sala", "comentarios"});
    for (const auto& row : data) {
        writer.writeRow({
            std::to_string(row.leadId),
            row.fuente,
            dateutil::toIsoString(row.fechaCreacion),
            row.tieneVenta ? dateutil::toIsoString(row.fechaVenta) : std::string(),
            detail::numOrEmpty(row.monto),
            detail::numOrEmpty(row.vpg),
            row.sala,
            detail::fieldOrEmpty(row.comentario)
        });
    }

    return data.size();
}

// Genera la serie mensual MXN/EUR 2014-2024 con 2% de nulos intencionales.
inline std::size_t generateFxCsv(const std::filesystem::path& outputPath,
                                 unsigned seed) {
    const Date inicio{2014, 1, 1};
    const int nMeses = 132;

    std::mt19937 rng(seed);
    std::normal_distribution<double> ruidoFx(0.0, 0.35);
    std::normal_distribution<double> ruidoBanxico(0.0, 0.30);
    std::normal_distribution<double> ruidoBce(0.0, 0.15);
    std::normal_distribution<double> ruidoInflMx(0.0, 0.20);
    std::normal_distribution<double> ruidoInflEz(0.0, 0.30);

    const double pi = 3.14159265358979323846;
    const int idx2022 = (2022 - 2014) * 12;
    const int idxPico2022 = idx2022 + 7;

    std::vector<detail::FxRow> filas;
    filas.reserve(static_cast<std::size_t>(nMeses));

    for (int i = 0; i < nMeses; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(nMeses - 1);

        const double tendencia = 16.5 + t * (21.5 - 16.5);
        const double estacionalidad = 0.45 * std::sin(2.0 * pi * static_cast<double>(i) / 12.0 * 1.6);
        const double tipoCambio = tendencia + estacionalidad + ruidoFx(rng);

        const double tasaBanxico = 4.0 + 2.0 * std::sin(4.0 * pi * t) + ruidoBanxico(rng);
        const double tasaBce = 0.3 + 3.0 / (1.0 + std::exp(-0.15 * (i - idx2022))) + ruidoBce(rng);
        const double inflacionMx = 3.5 + 1.5 * std::sin(6.0 * pi * t) + ruidoInflMx(rng);
        const double gauss = std::exp(-0.5 * std::pow((i - idxPico2022) / 12.0, 2.0));
        const double inflacionEz = 1.8 + 6.0 * gauss + ruidoInflEz(rng);

        detail::FxRow fila;
        fila.fecha = dateutil::addMonths(inicio, i);
        fila.tipoCambio = tipoCambio;
        fila.tasaBanxico = tasaBanxico;
        fila.tasaBce = tasaBce;
        fila.inflacionMx = inflacionMx;
        fila.inflacionEz = inflacionEz;
        filas.push_back(fila);
    }

    const int nNulos = std::max(1, static_cast<int>(nMeses * 0.02));
    std::vector<int> idx(static_cast<std::size_t>(nMeses));
    for (int i = 0; i < nMeses; ++i) idx[static_cast<std::size_t>(i)] = i;
    std::shuffle(idx.begin(), idx.end(), rng);
    for (int k = 0; k < nNulos; ++k) {
        filas[static_cast<std::size_t>(idx[static_cast<std::size_t>(k)])].tipoCambioNulo = true;
    }

    csvutil::Writer writer(outputPath.string());
    writer.writeRow({"fecha", "tipo_cambio", "tasa_banxico", "tasa_bce",
                     "inflacion_mx", "inflacion_ez"});
    for (const auto& fila : filas) {
        writer.writeRow({
            dateutil::toIsoString(fila.fecha),
            fila.tipoCambioNulo ? std::string() : detail::fixed4(fila.tipoCambio),
            detail::fixed4(fila.tasaBanxico),
            detail::fixed4(fila.tasaBce),
            detail::fixed4(fila.inflacionMx),
            detail::fixed4(fila.inflacionEz)
        });
    }

    return filas.size();
}

} // namespace datagen
