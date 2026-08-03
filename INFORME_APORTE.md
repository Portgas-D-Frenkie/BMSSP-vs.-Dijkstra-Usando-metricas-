# Aporte experimental — 7 métricas (BMSSP vs. Dijkstra)

Este paquete contiene una re-experimentación completa, desde cero, del proyecto
BMSSP vs. Dijkstra, instrumentando **las 7 métricas** solicitadas. A diferencia
del CSV original del artículo (1 ejecución por instancia, Windows/MSVC), estos
resultados usan **10 repeticiones por instancia** en un entorno Linux/GCC -O3
controlado, con **procesos aislados por algoritmo** para medir memoria de forma
limpia (sin contaminación cruzada entre Dijkstra y BMSSP).

## Metodología

- Mismos 17 grafos que el artículo original: Experimento 1 (n=100..6400,
  densidad≈10) y Experimento 2 (n=5000 fijo, m=25 000..250 000).
- **Misma fórmula de semilla** que `experiments.cpp` original
  (`seed = 2025 + n` en Exp1, `seed = 2025 + densidad` en Exp2), por lo que los
  grafos generados son bit-a-bit idénticos a los del artículo.
- Cada instancia se corrió **10 veces** por algoritmo, en un **proceso nuevo por
  corrida** (`run_one`), para que `getrusage()` mida memoria pico sin arrastrar
  memoria de ejecuciones anteriores.
- La librería BMSSP usada es la oficial (`lcs147/bmssp`, autores del paper
  arXiv:2511.03007), sin modificar. `prepare_graph(false)` en todos los casos
  (igual que el artículo original).

## Las 7 métricas: cómo se midieron

| # | Métrica | Método de medición | Columna en `results_7metricas.csv` |
|---|---|---|---|
| 1 | Tiempo de ejecución | `std::chrono::high_resolution_clock`, media de 10 corridas | `*_time_us_mean` |
| 2 | Correctitud | Comparación byte-a-byte del vector de distancias completo (no solo checksum) | `correcto` |
| 3 | Speedup relativo | `bmssp_time_us_mean / dijkstra_time_us_mean` | `speedup_bmssp_vs_dijkstra` |
| 4 | Memoria pico | `getrusage(RUSAGE_SELF).ru_maxrss` (KB), proceso aislado | `*_peak_rss_kb_mean` |
| 5 | Memoria por estructura | Bytes de heap asignados durante la fase de solución (override de `operator new`/`delete`, activado solo durante `dijkstra()` / `solver.execute()`) | `*_heap_bytes_mean` |
| 6 | Tasa de crecimiento empírica | Regresión ley de potencia $t = c \cdot n^p$ sobre Exp1 (log-log) | `growth_analysis.txt` |
| 7 | Punto de cruce | Extrapolación del modelo de potencia (intersección de ambas curvas) | `growth_analysis.txt` |

## Resultados principales

### 1-3. Tiempo, correctitud y speedup
- **Correctitud: 100% OK** en las 17 instancias (distancias idénticas, vector completo).
- Dijkstra fue más rápido en las 17 instancias. El speedup de BMSSP/Dijkstra
  **converge hacia ~3.5-4.1x** en las instancias más grandes (n=6400: 3.47x;
  m=250 000: 4.13x) — mucho más estable que los datos originales de 1 sola
  corrida (que iban de 3.1x a 10.2x). Este rango converge con lo reportado por
  Castro et al. (arXiv:2511.03007): "Dijkstra 3 to 4 times faster in all tested
  scenarios."

### 4. Memoria pico (RSS)
- Para grafos pequeños (Exp1 completo, n≤6400) el RSS pico se mantiene plano en
  ~12 900 KB para ambos algoritmos — está dominado por el **piso base del
  proceso** (runtime de C++, no por las estructuras del algoritmo). En este
  rango, la memoria pico **no es una métrica informativa**.
- Recién se diferencia a partir de m≥125 000 (Exp2): en m=250 000, BMSSP usa
  20 790 KB vs. 19 330 KB de Dijkstra (**+7.5%**).

### 5. Memoria por estructura de datos (heap asignado en la fase de solución)
- Aquí sí hay una diferencia dramática y consistente: BMSSP asigna, en promedio,
  **~23 veces más bytes de heap** que Dijkstra durante la resolución (Exp2).
  En Exp1, n=6400: Dijkstra asigna 575 KB vs. BMSSP 12 324 KB (**21.4x**).
- **Hallazgo interesante:** la memoria *pico* (métrica 4) crece mucho menos que
  la memoria *asignada total* (métrica 5). Esto sugiere que la sobrecarga de
  BMSSP no es un footprint sostenido más grande, sino un **volumen alto de
  asignaciones/liberaciones transitorias** (estructuras recursivas de la
  partición por niveles, cola de lotes *batchPQ*, mapas de pivotes), consistente
  con el diseño recursivo multinivel del algoritmo.

### 6. Tasa de crecimiento empírica
```
Dijkstra: t ~ 0.1816 * n^1.1208   (R² = 0.9970)
BMSSP:    t ~ 5.4833 * n^0.8623   (R² = 0.9730)
```
Con 10 repeticiones el ajuste es mucho más confiable que el intento anterior
(R² negativo con 1 sola corrida). El exponente de Dijkstra (≈1.12) es coherente
con $O(n\log n)$; el de BMSSP (≈0.86) sugiere que, en este rango de n, el costo
fijo de BMSSP crece más lentamente que Dijkstra en términos relativos — aunque
sigue siendo más lento en términos absolutos por su constante inicial mucho
mayor (c=5.48 vs. c=0.18).

### 7. Punto de cruce
Extrapolando el modelo de potencia: **n ≈ 5.3 × 10⁵** (530 000 vértices).

⚠️ **Advertencia metodológica honesta:** este número es una extrapolación desde
un rango de solo 100 a 6400 vértices — casi dos órdenes de magnitud por debajo
del punto proyectado. Aunque el ajuste tiene buen R² (>0.97) *dentro* del rango
medido, extrapolar leyes de potencia fuera del rango observado es
estadísticamente frágil (como ya se discutió: un modelo distinto puede mover
esta cifra en órdenes de magnitud). Además, difiere en varios órdenes de
magnitud del estimado por Castro et al. (n≈10⁶⁷), que se basa en análisis
asintótico riguroso de las constantes del algoritmo, no en extrapolación
empírica de datos. **Se recomienda citar ambos números en el artículo,
explicando la diferencia de metodología**, no presentar el propio como
definitivo.

## Archivos incluidos

- `run_one.cpp` — programa C++ instrumentado (una instancia, un algoritmo, un proceso)
- `run_all.py` — orquestador (17 instancias x 2 algoritmos x 10 repeticiones)
- `analyze.py`, `analyze_exp2.py` — regresión, punto de cruce, figuras
- `results_7metricas.csv` — resultados crudos completos (17 filas x 20 columnas)
- `growth_analysis.txt` — resultado de la regresión (métricas 6 y 7)
- `fig_tiempo_exp1.png`, `fig_speedup_exp1.png`, `fig_memoria_pico_exp1.png`,
  `fig_memoria_pico_exp2.png`, `fig_memoria_estructura_exp1.png` — figuras listas
- `snippet_latex.tex` — sección lista para pegar en `main.tex`
- `single_include/bmssp.hpp` — librería oficial usada (sin modificar)

## Cómo reproducir

```bash
g++ -std=c++20 -O3 -I. run_one.cpp -o run_one
python3 run_all.py        # genera results_7metricas.csv (tarda ~1-2 min)
python3 analyze.py        # genera figuras y growth_analysis.txt (Exp1)
python3 analyze_exp2.py   # figura y hallazgos de memoria (Exp2)
```
