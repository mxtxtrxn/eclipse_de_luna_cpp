#pragma once
// csv.hpp — lectura y escritura de CSV sin dependencias externas.
// Soporta campos entre comillas (con comas internas) y distintos
// finales de línea (\r\n o \n), que es lo que traen los CSV originales.

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace csvutil {

using Row = std::vector<std::string>;

inline Row parseLine(const std::string& line) {
    Row fields;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';
                    ++i;
                } else {
                    inQuotes = false;
                }
            } else {
                current += c;
            }
        } else {
            if (c == '"') {
                inQuotes = true;
            } else if (c == ',') {
                fields.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
    }
    fields.push_back(current);
    return fields;
}

inline std::string stripCR(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

struct Table {
    Row header;
    std::vector<Row> rows;

    int colIndex(const std::string& name) const {
        for (size_t i = 0; i < header.size(); ++i)
            if (header[i] == name) return static_cast<int>(i);
        return -1;
    }

    const std::string& at(size_t row, const std::string& colName) const {
        int idx = colIndex(colName);
        if (idx < 0) throw std::runtime_error("Columna no encontrada: " + colName);
        return rows[row][static_cast<size_t>(idx)];
    }
};

inline Table readCsv(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("No se pudo abrir: " + path);

    Table table;
    std::string line;
    bool first = true;
    while (std::getline(file, line)) {
        line = stripCR(line);
        if (line.empty()) continue;
        Row fields = parseLine(line);
        if (first) {
            table.header = fields;
            first = false;
        } else {
            table.rows.push_back(fields);
        }
    }
    return table;
}

inline std::string escapeField(const std::string& field) {
    bool needsQuotes = field.find(',') != std::string::npos ||
                        field.find('"') != std::string::npos ||
                        field.find('\n') != std::string::npos;
    if (!needsQuotes) return field;
    std::string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

class Writer {
public:
    explicit Writer(const std::string& path) : out_(path, std::ios::binary) {
        if (!out_.is_open()) throw std::runtime_error("No se pudo escribir: " + path);
    }

    void writeRow(const std::vector<std::string>& fields) {
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) out_ << ',';
            out_ << escapeField(fields[i]);
        }
        out_ << '\n';
    }

private:
    std::ofstream out_;
};

} // namespace csvutil
