# Resumen del Proyecto Eclipse de Luna

## Calidad de datos

- Filas crudas: 5250
- Filas limpias: 5000
- Duplicados eliminados: 250
- Montos imputados: 329
- VPG imputados: 302
- IDs unicos: OK
- Ventas sin monto: 0
- Montos sin venta: 0
- Monto minimo: 15057.09
- VPG minimo: 501.78
- Validacion general: OK

## Modelos

| Modelo | MAE | RMSE | MAPE |
|---|---|---|---|
| ARIMA(1,1,1) | 0.3976 | 0.5119 | 1.8666% |
| MLP (red neuronal) | 0.5215 | 0.6514 | 2.4452% |

- Mejor modelo por MAE: ARIMA(1,1,1)
- Umbral MAPE (3.00%): OK

## Pronostico MXN/EUR

| Fecha | ARIMA | MLP |
|---|---|---|
| 2025-01-01 | 21.6433 | 21.1475 |
| 2025-02-01 | 21.6011 | 21.1365 |
| 2025-03-01 | 21.5923 | 21.1835 |
| 2025-04-01 | 21.5905 | 21.1428 |
| 2025-05-01 | 21.5901 | 21.1520 |
| 2025-06-01 | 21.5900 | 21.1685 |

- Rango ARIMA: 21.5900 a 21.6433
- Rango MLP: 21.1365 a 21.1835
