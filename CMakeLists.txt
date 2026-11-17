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

set(ECLIPSE_PROGRAMS generar_datos limpieza forecasting)
foreach(programa IN LISTS ECLIPSE_PROGRAMS)
    add_executable(${programa} src/${programa}.cpp)
    target_link_libraries(${programa} PRIVATE eclipse_common)
    set_target_properties(${programa} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endforeach()

if(MINGW)
    foreach(objetivo IN ITEMS ${ECLIPSE_PROGRAMS})
        if(TARGET ${objetivo})
            target_link_options(${objetivo} PRIVATE -static)
        endif()
    endforeach()
endif()
