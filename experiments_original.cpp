#include "single_include/bmssp.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <fstream>
#include <queue>
#include <limits>
#include <cmath>
#include <filesystem>

using T = long long;
using namespace std;
using namespace std::chrono;

// ============================================================
// CONFIGURACIÓN DE EXPERIMENTOS - ¡MODIFICA AQUÍ!
// ============================================================

struct Config {
    // Tipos de experimentos a ejecutar
    bool ejecutar_exp1 = true;      // Variar vértices
    bool ejecutar_exp2 = true;      // Variar densidad
    bool ejecutar_exp3 = true;      // Grafos personalizados
    bool ejecutar_exp4 = false;     // Grafos tipo hub
    
    // Parámetros Exp1: Variar vértices (m ≈ n * densidad)
    int exp1_n_inicio = 10000000;
    int exp1_n_fin = 1000000000;
    int exp1_paso = 2;              // Multiplicador (2 = n*2, 10 = n*10)
    int exp1_densidad = 10;         // m = n * densidad
    
    // Parámetros Exp2: Variar densidad (n fijo)
    int exp2_n_fijo = 5000;
    int exp2_densidad_inicio = 5;
    int exp2_densidad_fin = 50;
    int exp2_paso_densidad = 5;
    
    // Parámetros Exp3: Grafos personalizados (lista explícita)
    vector<pair<int, int>> exp3_grafos = {
        {100, 500},     // {n, m}
        {500, 2000},
        {1000, 5000},
        {5000, 25000},
        {10000, 50000}
    };
    
    // Parámetros Exp4: Grafos con hub
    int exp4_n = 5000;
    int exp4_m = 50000;             // Total de aristas
    int exp4_pct_hub = 20;          // % de aristas conectadas al hub
    
    // Parámetros generales
    int seed_base = 2025;           // Semilla base
    int peso_min = 1;               // Peso mínimo de arista
    int peso_max = 1000;            // Peso máximo de arista
    bool grafo_dirigido = false;    // false = no dirigido, true = dirigido
    bool guardar_grafos = true;     // Guardar archivos .gr
    bool verificar_correctitud = true;
    int max_vertices_mostrar = 20;  // Para debug
    
    // Salidas
    string archivo_resultados = "mis_experimentos.csv";
    string carpeta_grafos = "grafos_generados";
};

// ============================================================
// IMPLEMENTACIÓN DE DIJKSTRA
// ============================================================

vector<T> dijkstra(int n, const vector<vector<pair<int, T>>>& adj, int source) {
    vector<T> dist(n, numeric_limits<T>::max());
    priority_queue<pair<T, int>, vector<pair<T, int>>, greater<pair<T, int>>> pq;
    
    dist[source] = 0;
    pq.push({0, source});
    
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        
        if (d > dist[u]) continue;
        
        for (auto [v, w] : adj[u]) {
            if (dist[v] > d + w) {
                dist[v] = d + w;
                pq.push({dist[v], v});
            }
        }
    }
    return dist;
}

// ============================================================
// GENERADORES DE GRAFOS SINTÉTICOS
// ============================================================

// Generador 1: Grafo aleatorio estándar
vector<tuple<int, int, T>> generateRandomGraph(int n, int m, int seed, 
                                               int peso_min = 1, int peso_max = 1000) {
    mt19937 rng(seed);
    uniform_int_distribution<int> vertex_dist(0, n-1);
    uniform_int_distribution<T> weight_dist(peso_min, peso_max);
    
    vector<tuple<int, int, T>> edges;
    edges.reserve(m);
    
    // Asegurar conectividad básica (n-1 aristas)
    for (int i = 1; i < n; i++) {
        int u = uniform_int_distribution<int>(0, i-1)(rng);
        edges.push_back({u, i, weight_dist(rng)});
    }
    
    // Añadir aristas aleatorias adicionales
    int extra_edges = m - (n - 1);
    while (extra_edges > 0) {
        int u = vertex_dist(rng);
        int v = vertex_dist(rng);
        if (u != v) {
            edges.push_back({u, v, weight_dist(rng)});
            extra_edges--;
        }
    }
    
    return edges;
}

// Generador 2: Grafo con hub (nodo central muy conectado)
vector<tuple<int, int, T>> generateHubGraph(int n, int m, int seed,
                                            int pct_hub = 20,
                                            int peso_min = 1, int peso_max = 1000) {
    mt19937 rng(seed);
    uniform_int_distribution<int> vertex_dist(1, n-1);
    uniform_int_distribution<T> weight_dist(peso_min, peso_max);
    uniform_int_distribution<int> hub_choice(0, 99);
    
    vector<tuple<int, int, T>> edges;
    edges.reserve(m);
    
    int hub = 0;  // Nodo central
    
    // Asegurar conectividad
    for (int i = 1; i < n; i++) {
        edges.push_back({hub, i, weight_dist(rng)});
    }
    
    // Añadir aristas adicionales
    int extra_edges = m - (n - 1);
    int hub_edges = 0;
    
    while (extra_edges > 0) {
        int u, v;
        if (hub_choice(rng) < pct_hub && hub_edges < m * 0.3) {
            // Conectar al hub
            u = hub;
            v = vertex_dist(rng);
            hub_edges++;
        } else {
            // Conectar aleatoriamente
            u = vertex_dist(rng);
            v = vertex_dist(rng);
        }
        
        if (u != v) {
            edges.push_back({u, v, weight_dist(rng)});
            extra_edges--;
        }
    }
    
    return edges;
}

// ============================================================
// FUNCIÓN PRINCIPAL DE EXPERIMENTO
// ============================================================

void runExperiment(int n, int m, int seed, ofstream& results, const Config& cfg) {
    cout << "  Grafo: n=" << n << ", m=" << m << " (seed=" << seed << ")" << endl;
    
    // Elegir tipo de grafo según configuración
    vector<tuple<int, int, T>> edges;
    // Por defecto usamos random, pero se puede pasar el tipo como parámetro
    edges = generateRandomGraph(n, m, seed, cfg.peso_min, cfg.peso_max);
    
    // Guardar grafo si está configurado
    if (cfg.guardar_grafos) {
        filesystem::create_directory(cfg.carpeta_grafos);
        string filename = cfg.carpeta_grafos + "/grafo_n" + to_string(n) + 
                         "_m" + to_string(m) + "_s" + to_string(seed) + ".gr";
        ofstream f(filename);
        f << "p sp " << n << " " << edges.size() << endl;
        for (auto [u, v, w] : edges) {
            f << "a " << u << " " << v << " " << w << endl;
            if (!cfg.grafo_dirigido && u != v) {
                f << "a " << v << " " << u << " " << w << endl;
            }
        }
        f.close();
    }
    
    // Construir lista de adyacencia para Dijkstra
    vector<vector<pair<int, T>>> adj(n);
    for (auto [u, v, w] : edges) {
        adj[u].push_back({v, w});
        if (!cfg.grafo_dirigido) {
            adj[v].push_back({u, w});
        }
    }
    
    int source = 0;
    
    // Configurar solver BMSSP
    spp::bmssp<T> solver(n);
    for (auto [u, v, w] : edges) {
        solver.addEdge(u, v, w);
        if (!cfg.grafo_dirigido) {
            solver.addEdge(v, u, w);
        }
    }
    
    // Ejecutar BMSSP
    cout << "   Ejecutando BMSSP..." << flush;
    auto start_bmssp = high_resolution_clock::now();
    solver.prepare_graph(false);
    auto [dist_bmssp, pred_bmssp] = solver.execute(source);
    auto end_bmssp = high_resolution_clock::now();
    auto time_bmssp = duration_cast<microseconds>(end_bmssp - start_bmssp);
    cout << "  " << time_bmssp.count() << " μs" << endl;
    
    // Ejecutar Dijkstra
    cout << "      Ejecutando Dijkstra..." << flush;
    auto start_dijkstra = high_resolution_clock::now();
    auto dist_dijkstra = dijkstra(n, adj, source);
    auto end_dijkstra = high_resolution_clock::now();
    auto time_dijkstra = duration_cast<microseconds>(end_dijkstra - start_dijkstra);
    cout << "  " << time_dijkstra.count() << " μs" << endl;
    
    // Verificar correctitud
    bool correct = true;
    if (cfg.verificar_correctitud) {
        for (int i = 0; i < n; i++) {
            if (dist_bmssp[i] != dist_dijkstra[i]) {
                correct = false;
                break;
            }
        }
        cout << "     Correctitud: " << (correct ? "OK" : "ERROR") << endl;
    }
    
    // Guardar resultados
    results << n << "," << m << ","
            << time_bmssp.count() << ","
            << time_dijkstra.count() << ","
            << (correct ? "OK" : "ERROR") << ","
            << cfg.grafo_dirigido << ","
            << seed << endl;
}

// ============================================================
// MAIN - EJECUCIÓN DE EXPERIMENTOS
// ============================================================

int main() {
    Config cfg;
    
    cout << "================================================" << endl;
    cout << "  EXPERIMENTOS BMSSP vs DIJKSTRA" << endl;
    cout << "  Artículo: Castro, Clementino & de Freitas (2025)" << endl;
    cout << "================================================" << endl;
    cout << endl;
    
    // Crear archivo de resultados
    ofstream results(cfg.archivo_resultados);
    results << "vertices,aristas,bmssp_us,dijkstra_us,correcto,dirigido,seed" << endl;
    
    int seed = cfg.seed_base;
    
    // ============================================================
    // EXPERIMENTO 1: Variar número de vértices
    // ============================================================
    if (cfg.ejecutar_exp1) {
        cout << " EXPERIMENTO 1: Variando vértices (m ≈ n * " << cfg.exp1_densidad << ")" << endl;
        cout << "------------------------------------------------" << endl;
        
        for (int n = cfg.exp1_n_inicio; n <= cfg.exp1_n_fin; n *= cfg.exp1_paso) {
            int m = n * cfg.exp1_densidad;
            runExperiment(n, m, seed + n, results, cfg);
        }
        cout << endl;
    }
    
    // ============================================================
    // EXPERIMENTO 2: Variar densidad
    // ============================================================
    if (cfg.ejecutar_exp2) {
        cout << " EXPERIMENTO 2: Variando densidad (n=" << cfg.exp2_n_fijo << ")" << endl;
        cout << "------------------------------------------------" << endl;
        
        for (int d = cfg.exp2_densidad_inicio; d <= cfg.exp2_densidad_fin; d += cfg.exp2_paso_densidad) {
            int m = cfg.exp2_n_fijo * d;
            runExperiment(cfg.exp2_n_fijo, m, seed + d, results, cfg);
        }
        cout << endl;
    }
    
    // ============================================================
    // EXPERIMENTO 3: Grafos personalizados
    // ============================================================
    if (cfg.ejecutar_exp3) {
        cout << " EXPERIMENTO 3: Grafos personalizados" << endl;
        cout << "------------------------------------------------" << endl;
        
        for (auto [n, m] : cfg.exp3_grafos) {
            runExperiment(n, m, seed + n + m, results, cfg);
        }
        cout << endl;
    }
    
    // ============================================================
    // EXPERIMENTO 4: Grafos con hub
    // ============================================================
    if (cfg.ejecutar_exp4) {
        cout << " EXPERIMENTO 4: Grafos con hub (n=" << cfg.exp4_n 
             << ", m=" << cfg.exp4_m << ", " << cfg.exp4_pct_hub << "% al hub)" << endl;
        cout << "------------------------------------------------" << endl;
        
        // Aquí necesitarías una versión de runExperiment que acepte generador personalizado
        // Por ahora usamos el generador random normal
        runExperiment(cfg.exp4_n, cfg.exp4_m, seed + 9999, results, cfg);
        cout << endl;
    }
    
    results.close();
    
    cout << "================================================" << endl;
    cout << " EXPERIMENTOS COMPLETADOS" << endl;
    cout << "   Resultados guardados en: " << cfg.archivo_resultados << endl;
    if (cfg.guardar_grafos) {
        cout << "   Grafos guardados en: " << cfg.carpeta_grafos << "/" << endl;
    }
    cout << "================================================" << endl;
    
    return 0;
}
