CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -fno-ident
CPPFLAGS ?= -Iinclude

BIN_DIR := bin
APPS := $(BIN_DIR)/generar_datos $(BIN_DIR)/limpieza $(BIN_DIR)/forecasting
TEST_SRCS := $(wildcard tests/test_*.cpp)
TEST_BIN := $(BIN_DIR)/eclipse_tests
HEADERS := $(wildcard include/*.hpp)
TEST_HEADERS := $(wildcard tests/*.hpp)

.PHONY: all run test clean

all: $(APPS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/%: src/%.cpp $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

$(TEST_BIN): $(TEST_SRCS) $(TEST_HEADERS) $(HEADERS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -Itests -o $@ $(TEST_SRCS)

run: all
	./$(BIN_DIR)/generar_datos
	./$(BIN_DIR)/limpieza
	./$(BIN_DIR)/forecasting

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BIN_DIR)
