# BMSSP vs. Dijkstra

Análisis experimental del rendimiento del algoritmo **BMSSP** (*Bounded Multi-Source Shortest Path*, Duan et al., STOC 2025) frente al algoritmo clásico de **Dijkstra**, para el problema del camino más corto de fuente única (SSSP) en grafos con pesos no negativos.

Artículo científico asociado (formato IEEE): ver `main.pdf` / `main.tex` en este mismo repositorio o carpeta de entrega.

## Contenido del repositorio

```
.
├── main.cpp                  # Demo mínima de uso de la librería BMSSP
├── experiments.cpp           # Generación de grafos, Dijkstra, orquestación de experimentos
├── CMakeLists.txt            # Sistema de construcción (CMake >= 3.28, C++20)
├── experiment_results.csv    # Resultados crudos de los 17 experimentos reportados en el paper
├── single_include/bmssp.hpp  # Librería header-only de referencia del algoritmo BMSSP
├── EXPERIMENTACION3/bmssp-python/   # Implementación de referencia en Python (material de estudio)
├── bmssp-main/bmssp-main/           # Implementación de referencia en Go (material de estudio)
└── Experimentacion1/optimal-mst/    # Proyecto de referencia sobre MST (material de estudio)
```

## Requisitos / Dependencias

- CMake >= 3.28
- Compilador con soporte C++20 (probado con GCC / g++)
- Sistema operativo: Windows, Linux o macOS (probado en Windows 11)
- No requiere bibliotecas externas adicionales (BMSSP se integra como header-only)

## Instalación y compilación

```bash
git clone https://github.com/215326-hub/BMSSP-vs-DIJKSTRA.git
cd BMSSP-vs-DIJKSTRA
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

## Ejecución

Demo mínima (grafo de 5 vértices):
```bash
./main
```

Suite completa de experimentos (genera `experiment_results.csv`):
```bash
./experiments
```

Los parámetros de los experimentos (rango de vértices, densidades, semillas) se configuran en la `struct Config` al inicio de `experiments.cpp`.

## Autores

- Franklin Gilberto Mamani Condori — 230253@unsaac.edu.pe
- Jhon Eber Huayhua Huamani — 215326@unsaac.edu.pe
- Joham Esau Quispe Huillca — 211358@unsaac.edu.pe

Universidad Nacional de San Antonio Abad del Cusco (UNSAAC)

## Licencia

MIT License, 2025.
