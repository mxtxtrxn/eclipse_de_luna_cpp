# Proyecto Eclipse de Luna  [Proyecto en C++] (MXN/EUR)

Reescritura en C++17 de un proyecto de análisis en Python. El caso de negocio
combina dos tareas: limpieza de una tabla legada de leads y pronóstico del tipo
de cambio MXN/EUR para apoyar la planeación financiera.

El nombre del proyecto y todos los datos son ficticios. Las series se generan
con semillas fijas y no provienen de fuentes reales.

## Cambios respecto de la versión original

| | Versión original | Esta versión |
|---|---|---|
| Moneda pronosticada | MXN/USD | **MXN/EUR** |
| Modelo de Machine Learning | XGBoost | **Red neuronal (MLP) entrenada desde cero** |
| Lenguaje | Python + notebooks | **C++17, sin dependencias externas** |
| Modelo clásico | ARIMA(1,1,1) vía `statsmodels` | ARIMA(1,1,1) vía **Hannan-Rissanen** |
| Pruebas | No incluía | **Arnés propio con 119 casos** |
| Construcción | Scripts sueltos | **CMake + CTest y Makefile** |

No se usa ninguna librería externa: el parseo de CSV, las fechas, el álgebra
lineal, la estadística, ARIMA y la red neuronal están implementados sobre la
biblioteca estándar de C++. Compila con `g++` o `clang++` en cualquier máquina
sin instalar paquetes.

## Estructura

```text
.
├── CMakeLists.txt          Configuración de compilación y CTest
├── Makefile                Compilación alternativa con objetivos test/run
├── .gitignore              Exclusiones de compilación y temporales
├── include/                Librería interna (solo headers)
│   ├── arima.hpp           ARIMA(1,1,1) por Hannan-Rissanen
│   ├── cleaning.hpp        Reglas de limpieza y reporte de calidad
│   ├── csv.hpp             Lectura/escritura de CSV
│   ├── data_generator.hpp  Generación reproducible de datos sintéticos
│   ├── date_utils.hpp      Aritmética de fechas (calendario gregoriano)
│   ├── fx_series.hpp       Serie MXN/EUR, interpolación y variables
│   ├── linalg.hpp          OLS vía ecuaciones normales
│   ├── mlp.hpp             Red neuronal con Adam
│   ├── reporting.hpp       Reporte ejecutivo y métricas de calidad
│   ├── stats.hpp           Media, mediana, MAE/RMSE/MAPE
│   └── text_utils.hpp      Normalización de texto (acentos, minúsculas)
├── src/
│   ├── generar_datos.cpp   Genera data/leads_ventas.csv y fx_historico_eur.csv
│   ├── limpieza.cpp        Limpieza de datos legados
│   ├── forecasting.cpp     Pronóstico MXN/EUR (ARIMA + MLP)
│   └── reporte.cpp         Consolida el resumen ejecutivo y la calidad
├── tests/                  Arnés de pruebas y 119 casos
├── data/                   Datos sintéticos de entrada y salida
└── reports/                Métricas, pronóstico y reporte ejecutivo
```

## Requisitos

- Compilador con soporte de C++17 (GCC 7+, Clang 5+ o MSVC 2019+).
- CMake 3.16 o superior (recomendado) o `make` en entornos tipo Unix/MSYS2.
- No se requieren librerías externas.

## Compilación con CMake

```sh
cmake -S . -B build
cmake --build build --config Release
```

Los ejecutables quedan en `build/bin/`. En Windows con MinGW se enlazan de
forma estática para no depender de DLLs del compilador.

## Compilación con Make

```sh
make
make test
```

Los binarios se generan en `bin/`.

## Ejecución

El pipeline completo, en orden:

```sh
./bin/generar_datos
./bin/limpieza
./bin/forecasting
./bin/reporte
```

Con Make también se puede usar:

```sh
make run
```

Con CMake:

```sh
./build/bin/generar_datos
./build/bin/limpieza
./build/bin/forecasting
./build/bin/reporte
```

Los programas usan rutas relativas (`data/`, `reports/`), así que deben
ejecutarse desde la raíz del proyecto. `reporte` depende de los archivos que
producen `limpieza` y `forecasting`, por eso va al final del pipeline.

## Reporte ejecutivo

`reporte` vuelve a aplicar las reglas de limpieza sobre el CSV crudo para
verificar las cifras y consolida los resultados en dos archivos:

- `reports/resumen.md`: calidad de datos, tabla comparativa de modelos, mejor
  modelo por MAE, cumplimiento del umbral de MAPE y pronóstico MXN/EUR con sus
  rangos.
- `reports/calidad_limpieza.csv`: filas crudas y limpias, duplicados, valores
  imputados, unicidad de `lead_id`, coherencia venta/monto y mínimos.

El programa termina con código distinto de cero si la limpieza no es válida o
si algún modelo supera el 3% de MAPE, de modo que sirve como verificación del
pipeline.

## Pruebas

Con CMake y CTest:

```sh
ctest --test-dir build --output-on-failure
```

Con Make:

```sh
make test
```

El arnés no requiere dependencias externas. Cubre:

- Parseo y escritura de CSV, incluidos campos entre comillas y finales CRLF.
- Aritmética de fechas, años bisiestos y cambios de mes.
- Normalización de texto con acentos, mayúsculas y espacios.
- Media, mediana, desviación estándar y métricas MAE/RMSE/MAPE.
- Álgebra lineal y regresión OLS.
- Ajuste y pronóstico ARIMA(1,1,1).
- Entrenamiento, determinismo y estabilidad del MLP.
- Reglas de limpieza: normalización, imputación por mediana, duplicados y
  validaciones.
- Interpolación de la serie MXN/EUR y construcción de variables.
- Reproducibilidad byte a byte de los datos generados.
- Coincidencia de las métricas y del pronóstico con los reportes guardados.
- Lectura de reportes, selección del mejor modelo, umbral de MAPE y escritura
  del resumen ejecutivo y del CSV de calidad.

Resultado esperado: **119/119 casos aprobados** (472 aserciones).

## Estado del proyecto

- **1.604 líneas efectivas de producción** en `include/` y `src/`; 1.275 en
  `tests/` (3.013 líneas no vacías en total).
- **119 casos de prueba** con 472 aserciones y 0 advertencias al compilar con
  `-Wall -Wextra -Wpedantic`.
- Cuatro programas: `generar_datos`, `limpieza`, `forecasting` y `reporte`.
- Sin dependencias externas; compila con CMake 3.16+ o Make.
- Salidas reproducibles byte a byte con las semillas 42 (leads) y 7 (serie FX).
- El repositorio no versiona `build/`, `bin/`, binarios, paquetes ni archivos
  temporales; esas exclusiones están en `.gitignore`.

## Parte 1: Limpieza de datos

El programa `limpieza` aplica las mismas reglas que el notebook original:

1. Normaliza `fuente` y `sala` (minúsculas, sin acentos, mapeo a categorías
   canónicas; lo que no mapea queda como `"Desconocido"`).
2. Rellena `comentarios` vacíos con `"Sin comentarios"`.
3. Imputa `monto` y `vpg` faltantes **solo** en registros con venta, usando la
   mediana por sala calculada únicamente sobre ventas válidas.
4. Elimina duplicados exactos, conservando la primera aparición.
5. Valida unicidad de `lead_id`, montos y VPG positivos, y coherencia entre
   venta y monto.

Resultado: de 5,250 filas sucias a 5,000 filas limpias (250 duplicados
intencionales, 5% del total).

## Parte 2: Pronóstico MXN/EUR

### Por qué EUR es distinto de USD

El tipo de cambio MXN/USD depende sobre todo de la política de Banxico y la
Fed. MXN/EUR depende de **dos** bancos centrales con ciclos distintos (Banxico
y el BCE), así que el dataset sintético modela explícitamente:

- `tasa_banxico` y `tasa_bce` por separado. El BCE tuvo tasas cercanas a cero
  entre 2014 y 2021 y subió con fuerza en 2022-2023; el generador lo representa
  con una función logística centrada en 2022.
- `inflacion_mx` e `inflacion_ez` por separado. La eurozona tuvo un pico
  inflacionario en 2022; el generador lo representa con una gaussiana centrada
  en agosto de 2022.

El modelo usa el **diferencial de tasas** (`tasa_banxico - tasa_bce`) como
variable, que es la variable que la teoría de paridad de tasas de interés
señala como relevante.

Los datos siguen siendo **sintéticos y reproducibles** (semillas fijas). No son
series reales de Banxico ni del BCE.

### ARIMA(1,1,1): Hannan-Rissanen

`statsmodels` ajusta ARIMA por máxima verosimilitud con filtro de Kalman.
Reimplementar eso desde cero no se justifica para una serie mensual de ~130
observaciones, así que se usa el método de dos pasos de **Hannan-Rissanen**
(1982): un AR largo auxiliar para aproximar las innovaciones y una regresión
OLS de `w_t` sobre `w_{t-1}` y `e_{t-1}` para obtener φ y θ. El intervalo de
confianza usa una aproximación simple (`±1.96·σ·√h`), documentada en el código.

### MLP en vez de XGBoost

La red es feedforward, con una capa oculta y activación tanh, entrenada con
backpropagation y Adam. Las 11 variables de entrada, estandarizadas, son:
`lag_1,2,3,6,12` del tipo de cambio, el diferencial de tasas, la inflación de
México, la inflación de la eurozona, el mes codificado como `(seno, coseno)` y
una tendencia lineal.

El tamaño de la red (2 neuronas ocultas, 500 épocas) se eligió por validación:
se apartó 2022 dentro del tramo de entrenamiento y se promediaron 5
inicializaciones por configuración. Con solo 96 observaciones de entrenamiento,
una red pequeña generaliza mejor que una grande.

### Resultados (test 2023-2024)

| Modelo | MAE | RMSE | MAPE |
|---|---|---|---|
| ARIMA(1,1,1) | 0.3976 | 0.5119 | 1.87% |
| MLP (red neuronal) | 0.5215 | 0.6514 | 2.45% |

ARIMA queda ligeramente mejor. Es un resultado honesto: con una serie mensual
corta, dominada por su nivel reciente y ruido, un modelo estadístico simple es
difícil de superar. Ambos modelos quedan por debajo del umbral de 3% de MAPE
usado como referencia de buen resultado.

### Pronóstico enero-junio 2025

| Fecha | ARIMA | MLP |
|---|---|---|
| 2025-01 | 21.6433 | 21.1475 |
| 2025-02 | 21.6011 | 21.1365 |
| 2025-03 | 21.5923 | 21.1835 |
| 2025-04 | 21.5905 | 21.1428 |
| 2025-05 | 21.5901 | 21.1520 |
| 2025-06 | 21.5900 | 21.1685 |

### Corrección sobre el proyecto original

En el notebook original, el pronóstico final a 6 meses usaba el mismo
`arima_fit` entrenado solo hasta 2022-12 (el usado para medir el error en test)
y le pedía 6 pasos más; el archivo resultante quedaba etiquetado como
enero-junio 2025 aunque correspondía a 2023. Aquí se corrigió: las métricas se
calculan con el modelo entrenado solo hasta 2022, y el pronóstico de producción
reentrena con **toda** la historia disponible (2014-2024) antes de proyectar.

## Formato de datos

`data/leads_ventas.csv` (entrada sucia):

```text
lead_id,fuente,fecha_creacion,fecha_venta,monto,vpg,sala,comentarios
```

`data/leads_ventas_clean.csv` (salida limpia): mismas columnas, valores
normalizados e imputados.

`data/fx_historico_eur.csv`:

```text
fecha,tipo_cambio,tasa_banxico,tasa_bce,inflacion_mx,inflacion_ez
```

`reports/metricas_fx.csv`:

```text
,MAE,RMSE,MAPE
```

`reports/pronostico_6m.csv`:

```text
fecha,arima,mlp
```

`reports/calidad_limpieza.csv`:

```text
filas_crudas,filas_limpias,duplicados_eliminados,montos_imputados,vpg_imputados,ids_unicos,ventas_sin_monto,montos_sin_venta,monto_minimo,vpg_minimo
```

`reports/resumen.md` es un documento Markdown con las mismas cifras y las
tablas de modelos y pronóstico.

Los montos se guardan con dos decimales y las series con cuatro. Los campos con
comas o comillas se escapan siguiendo las reglas habituales de CSV.

## Notas de diseño

- C++17 portable: no usa `conio.h`, `system("cls")` ni APIs exclusivas de
  Windows.
- Semillas fijas (42 para leads, 7 para la serie FX) para que los datos sean
  reproducibles byte a byte.
- El parseo de CSV soporta comillas, comas internas y finales de línea CRLF.
- Las fechas usan el algoritmo de días civiles de Howard Hinnant, adecuado para
  C++17.
- No hay datos personales, credenciales ni secretos: los comentarios de los
  leads son textos genéricos y no incluyen nombres, correos ni teléfonos.
- Los reportes se regeneran al ejecutar el pipeline y las pruebas verifican que
  coincidan con los archivos versionados.
