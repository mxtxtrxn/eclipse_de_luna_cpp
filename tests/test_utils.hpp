#pragma once
// test_utils.hpp — utilidades compartidas por las pruebas: localizar la raiz
// del proyecto, crear carpetas temporales y leer/escribir archivos de texto.

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace testutil {

namespace fs = std::filesystem;

inline fs::path projectRoot() {
    fs::path current = fs::current_path();
    for (int i = 0; i < 6; ++i) {
        if (fs::exists(current / "data" / "leads_ventas.csv")) return current;
        if (!current.has_parent_path()) break;
        current = current.parent_path();
    }
    throw std::runtime_error("No se encontro la raiz del proyecto");
}

inline fs::path tempDir(const std::string& name) {
    const fs::path dir = fs::temp_directory_path() / "eclipse_de_luna_tests" / name;
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

inline void writeText(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        throw std::runtime_error("No se pudo escribir: " + path.string());
    }
    out << text;
}

inline std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        throw std::runtime_error("No se pudo leer: " + path.string());
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}

} // namespace testutil
