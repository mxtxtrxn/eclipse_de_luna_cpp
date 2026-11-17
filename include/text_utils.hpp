#pragma once
// text_utils.hpp — normalización de texto equivalente a la función
// `normalizar_texto` del notebook original de Python: minúsculas,
// sin espacios extra, sin acentos. El dataset está en UTF-8, así que
// las vocales acentuadas vienen como 2 bytes (ej. 'á' = 0xC3 0xA1).

#include <string>
#include <algorithm>
#include <cctype>

namespace textutil {

inline std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::string toLowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Reemplaza vocales acentuadas UTF-8 (á,é,í,ó,ú,ü,ñ y mayúsculas) por
// su versión sin acento. Deja cualquier otro byte sin tocar.
inline std::string stripAccentsUtf8(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c0 = static_cast<unsigned char>(s[i]);
        if (c0 == 0xC3 && i + 1 < s.size()) {
            unsigned char c1 = static_cast<unsigned char>(s[i + 1]);
            char replacement = 0;
            switch (c1) {
                case 0xA1: replacement = 'a'; break; // á
                case 0x81: replacement = 'A'; break; // Á
                case 0xA9: replacement = 'e'; break; // é
                case 0x89: replacement = 'E'; break; // É
                case 0xAD: replacement = 'i'; break; // í
                case 0x8D: replacement = 'I'; break; // Í
                case 0xB3: replacement = 'o'; break; // ó
                case 0x93: replacement = 'O'; break; // Ó
                case 0xBA: replacement = 'u'; break; // ú
                case 0x9A: replacement = 'U'; break; // Ú
                case 0xBC: replacement = 'u'; break; // ü
                case 0x9C: replacement = 'U'; break; // Ü
                case 0xB1: replacement = 'n'; break; // ñ
                case 0x91: replacement = 'N'; break; // Ñ
                default: replacement = 0; break;
            }
            if (replacement != 0) {
                out += replacement;
                i += 2;
                continue;
            }
        }
        out += s[i];
        ++i;
    }
    return out;
}

// Equivalente a normalizar_texto(): strip + sin acentos + lower.
inline std::string normalize(const std::string& raw) {
    std::string s = trim(raw);
    s = stripAccentsUtf8(s);
    s = toLowerAscii(s);
    return s;
}

} // namespace textutil
