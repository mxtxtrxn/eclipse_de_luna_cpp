CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -fno-ident
CPPFLAGS ?= -Iinclude

BIN_DIR := bin
APPS := $(BIN_DIR)/generar_datos $(BIN_DIR)/limpieza $(BIN_DIR)/forecasting
HEADERS := $(wildcard include/*.hpp)

.PHONY: all run clean

all: $(APPS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/%: src/%.cpp $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

run: all
	./$(BIN_DIR)/generar_datos
	./$(BIN_DIR)/limpieza
	./$(BIN_DIR)/forecasting

clean:
	rm -rf $(BIN_DIR)
