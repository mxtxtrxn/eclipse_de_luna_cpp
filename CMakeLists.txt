cmake_minimum_required(VERSION 3.16)
project(eclipse_de_luna_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -fno-ident)
endif()

add_library(eclipse_common INTERFACE)
target_include_directories(eclipse_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(ECLIPSE_PROGRAMS generar_datos limpieza forecasting reporte)
foreach(programa IN LISTS ECLIPSE_PROGRAMS)
    add_executable(${programa} src/${programa}.cpp)
    target_link_libraries(${programa} PRIVATE eclipse_common)
    set_target_properties(${programa} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()

option(ECLIPSE_BUILD_TESTS "Compila el arnes de pruebas" ON)
if(ECLIPSE_BUILD_TESTS)
    enable_testing()

    add_executable(eclipse_tests
        tests/test_main.cpp
        tests/test_csv.cpp
        tests/test_date_utils.cpp
        tests/test_text_utils.cpp
        tests/test_stats.cpp
        tests/test_linalg.cpp
        tests/test_arima.cpp
        tests/test_mlp.cpp
        tests/test_cleaning.cpp
        tests/test_fx_series.cpp
        tests/test_data_generator.cpp
        tests/test_reporting.cpp
        tests/test_integration.cpp
    )
    target_include_directories(eclipse_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(eclipse_tests PRIVATE eclipse_common)
    set_target_properties(eclipse_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_test(NAME eclipse_tests COMMAND eclipse_tests)
    set_tests_properties(eclipse_tests PROPERTIES
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(MINGW)
    foreach(objetivo IN ITEMS ${ECLIPSE_PROGRAMS} eclipse_tests)
        if(TARGET ${objetivo})
            target_link_options(${objetivo} PRIVATE -static)
        endif()
    endforeach()
endif()
