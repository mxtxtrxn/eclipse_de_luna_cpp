#pragma once
// cleaning.hpp — reglas de limpieza del dataset legado, extraidas a una
// unidad reutilizable para poder probarlas sin ejecutar el programa.

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "csv.hpp"
#include "stats.hpp"
#include "text_utils.hpp"

namespace cleaning {

struct CleanedRow {
    std::string leadId;
    std::string fuente;
    std::string fechaCreacion;
    std::string fechaVenta;
    std::optional<double> monto;
    std::optional<double> vpg;
    std::string sala;
    std::string comentarios;

    bool tieneVenta() const { return !fechaVenta.empty(); }
};

struct Report {
    std::size_t filasLeidas = 0;
    std::size_t duplicadosEliminados = 0;
    std::size_t montosImputados = 0;
    std::size_t vpgsImputados = 0;
    bool idsUnicos = true;
    std::size_t ventasSinMonto = 0;
    std::size_t montosSinVenta = 0;
    double montoMinimo = 0.0;
    double vpgMinimo = 0.0;

    bool valido() const {
        return idsUnicos && ventasSinMonto == 0 && montosSinVenta == 0 &&
               montoMinimo > 0.0 && vpgMinimo > 0.0;
    }
};

struct Result {
    std::vector<CleanedRow> filas;
    Report reporte;
};

inline const std::unordered_map<std::string, std::string>& mapaFuente() {
    static const std::unordered_map<std::string, std::string> mapa = {
        {"proveedor a", "Proveedor A"},
        {"proveedora", "Proveedor A"},
        {"portal web", "Portal Web"},
        {"social", "Social"},
        {"facebook", "Social"},
        {"fb", "Social"},
        {"calle", "Calle"},
        {"calle tms", "Calle"},
        {"rci", "RCI"},
        {"ota", "OTA"},
        {"hotel", "Hotel"},
    };
    return mapa;
}

inline const std::unordered_map<std::string, std::string>& mapaSala() {
    static const std::unordered_map<std::string, std::string> mapa = {
        {"cancun", "Cancún"},
        {"playa del carmen", "Playa del Carmen"},
        {"pdc", "Playa del Carmen"},
        {"puerto vallarta", "Puerto Vallarta"},
        {"pv", "Puerto Vallarta"},
        {"los cabos", "Los Cabos"},
        {"cabo", "Los Cabos"},
    };
    return mapa;
}

inline std::string mapCategoria(
    const std::string& raw,
    const std::unordered_map<std::string, std::string>& mapa) {
    const std::string normalizado = textutil::normalize(raw);
    const auto it = mapa.find(normalizado);
    return (it != mapa.end()) ? it->second : std::string("Desconocido");
}

inline std::string canonicalFuente(const std::string& raw) {
    return mapCategoria(raw, mapaFuente());
}

inline std::string canonicalSala(const std::string& raw) {
    return mapCategoria(raw, mapaSala());
}

inline std::optional<double> parseOptionalDouble(const std::string& value) {
    if (value.empty()) return std::nullopt;
    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

inline std::string formatNumber(double value) {
    std::ostringstream oss;
    oss.precision(2);
    oss << std::fixed << value;
    return oss.str();
}

inline std::string formatOptional(const std::optional<double>& value) {
    return value.has_value() ? formatNumber(*value) : std::string();
}

inline void requireColumns(const csvutil::Table& raw,
                           const std::vector<std::string>& columnas) {
    for (const auto& columna : columnas) {
        if (raw.colIndex(columna) < 0) {
            throw std::runtime_error("Falta la columna obligatoria: " + columna);
        }
    }
}

inline Result cleanTable(const csvutil::Table& raw) {
    requireColumns(raw, {"lead_id", "fuente", "fecha_creacion", "fecha_venta",
                         "monto", "vpg", "sala", "comentarios"});

    const int cId = raw.colIndex("lead_id");
    const int cFuente = raw.colIndex("fuente");
    const int cFechaCreacion = raw.colIndex("fecha_creacion");
    const int cFechaVenta = raw.colIndex("fecha_venta");
    const int cMonto = raw.colIndex("monto");
    const int cVpg = raw.colIndex("vpg");
    const int cSala = raw.colIndex("sala");
    const int cComentarios = raw.colIndex("comentarios");

    Result result;
    result.reporte.filasLeidas = raw.rows.size();
    result.filas.reserve(raw.rows.size());

    for (const auto& fila : raw.rows) {
        if (fila.size() != raw.header.size()) {
            throw std::runtime_error("Fila con cantidad de columnas inesperada");
        }

        CleanedRow row;
        row.leadId = fila[static_cast<std::size_t>(cId)];
        row.fuente = canonicalFuente(fila[static_cast<std::size_t>(cFuente)]);
        row.fechaCreacion = fila[static_cast<std::size_t>(cFechaCreacion)];
        row.fechaVenta = fila[static_cast<std::size_t>(cFechaVenta)];
        row.monto = parseOptionalDouble(fila[static_cast<std::size_t>(cMonto)]);
        row.vpg = parseOptionalDouble(fila[static_cast<std::size_t>(cVpg)]);
        row.sala = canonicalSala(fila[static_cast<std::size_t>(cSala)]);

        const std::string comentario = fila[static_cast<std::size_t>(cComentarios)];
        row.comentarios = comentario.empty() ? std::string("Sin comentarios") : comentario;

        result.filas.push_back(std::move(row));
    }

    std::unordered_map<std::string, std::vector<double>> montosPorSala;
    std::unordered_map<std::string, std::vector<double>> vpgsPorSala;
    for (const auto& row : result.filas) {
        if (!row.tieneVenta()) continue;
        if (row.monto.has_value()) montosPorSala[row.sala].push_back(*row.monto);
        if (row.vpg.has_value()) vpgsPorSala[row.sala].push_back(*row.vpg);
    }

    std::unordered_map<std::string, double> medianaMonto;
    std::unordered_map<std::string, double> medianaVpg;
    for (auto& [sala, valores] : montosPorSala) medianaMonto[sala] = stats::median(valores);
    for (auto& [sala, valores] : vpgsPorSala) medianaVpg[sala] = stats::median(valores);

    for (auto& row : result.filas) {
        if (row.tieneVenta() && !row.monto.has_value()) {
            const auto it = medianaMonto.find(row.sala);
            if (it != medianaMonto.end()) {
                row.monto = it->second;
                ++result.reporte.montosImputados;
            }
        }
        if (row.tieneVenta() && !row.vpg.has_value()) {
            const auto it = medianaVpg.find(row.sala);
            if (it != medianaVpg.end()) {
                row.vpg = it->second;
                ++result.reporte.vpgsImputados;
            }
        }
    }

    std::unordered_set<std::string> vistos;
    std::vector<CleanedRow> sinDuplicados;
    sinDuplicados.reserve(result.filas.size());
    for (const auto& row : result.filas) {
        const std::string clave = row.leadId + '\x1f' + row.fuente + '\x1f' +
                                  row.fechaCreacion + '\x1f' + row.fechaVenta + '\x1f' +
                                  formatOptional(row.monto) + '\x1f' +
                                  formatOptional(row.vpg) + '\x1f' + row.sala + '\x1f' +
                                  row.comentarios;
        if (vistos.insert(clave).second) {
            sinDuplicados.push_back(row);
        }
    }
    result.reporte.duplicadosEliminados = result.filas.size() - sinDuplicados.size();
    result.filas = std::move(sinDuplicados);

    std::unordered_set<std::string> idsVistos;
    bool hayMonto = false;
    bool hayVpg = false;
    for (const auto& row : result.filas) {
        if (!idsVistos.insert(row.leadId).second) result.reporte.idsUnicos = false;
        if (row.tieneVenta() && !row.monto.has_value()) ++result.reporte.ventasSinMonto;
        if (!row.tieneVenta() && row.monto.has_value()) ++result.reporte.montosSinVenta;
        if (row.monto.has_value()) {
            result.reporte.montoMinimo = hayMonto
                ? std::min(result.reporte.montoMinimo, *row.monto)
                : *row.monto;
            hayMonto = true;
        }
        if (row.vpg.has_value()) {
            result.reporte.vpgMinimo = hayVpg
                ? std::min(result.reporte.vpgMinimo, *row.vpg)
                : *row.vpg;
            hayVpg = true;
        }
    }

    return result;
}

inline Result cleanFile(const std::filesystem::path& inputPath) {
    return cleanTable(csvutil::readCsv(inputPath.string()));
}

inline void writeCleanedCsv(const std::filesystem::path& outputPath,
                            const std::vector<CleanedRow>& filas) {
    csvutil::Writer writer(outputPath.string());
    writer.writeRow({"lead_id", "fuente", "fecha_creacion", "fecha_venta",
                     "monto", "vpg", "sala", "comentarios"});
    for (const auto& row : filas) {
        writer.writeRow({row.leadId, row.fuente, row.fechaCreacion, row.fechaVenta,
                         formatOptional(row.monto), formatOptional(row.vpg), row.sala,
                         row.comentarios});
    }
}

} // namespace cleaning
