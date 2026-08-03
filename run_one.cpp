// run_one.cpp
// Ejecuta UNA instancia (un algoritmo, un grafo) en un proceso aislado,
// para poder medir memoria pico (getrusage) sin contaminacion entre
// Dijkstra y BMSSP ni entre repeticiones.
//
// Reutiliza EXACTAMENTE los generadores de grafos y la implementacion de
// Dijkstra del proyecto original (experiments.cpp) para que los grafos
// generados sean identicos (misma semilla -> mismo grafo).

#include "single_include/bmssp.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <fstream>
#include <queue>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <sys/resource.h>

using T = long long;
using namespace std;
using namespace std::chrono;

// ------------------------------------------------------------------
// Contador global de bytes de heap (Metrica 5: memoria por estructura)
// Se activa/desactiva alrededor de la fase de solucion (no del parseo
// ni la generacion del grafo), igual para Dijkstra y BMSSP.
// ------------------------------------------------------------------
static atomic<long long> g_bytes_allocated{0};
static atomic<bool> g_counting{false};

void* operator new(size_t size) {
    void* p = malloc(size);
    if (!p) throw bad_alloc();
    if (g_counting.load(memory_order_relaxed))
        g_bytes_allocated.fetch_add((long long)size, memory_order_relaxed);
    return p;
}
void operator delete(void* p) noexcept { free(p); }
void operator delete(void* p, size_t) noexcept { free(p); }
void* operator new[](size_t size) { return operator new(size); }
void operator delete[](void* p) noexcept { free(p); }
void operator delete[](void* p, size_t) noexcept { free(p); }

// ------------------------------------------------------------------
// Generadores de grafos (copiados literalmente de experiments.cpp)
// ------------------------------------------------------------------
vector<tuple<int, int, T>> generateRandomGraph(int n, int m, int seed,
                                                int peso_min = 1, int peso_max = 1000) {
    mt19937 rng(seed);
    uniform_int_distribution<int> vertex_dist(0, n - 1);
    uniform_int_distribution<T> weight_dist(peso_min, peso_max);
    vector<tuple<int, int, T>> edges;
    edges.reserve(m);
    for (int i = 1; i < n; i++) {
        int u = uniform_int_distribution<int>(0, i - 1)(rng);
        edges.push_back({u, i, weight_dist(rng)});
    }
    int extra_edges = m - (n - 1);
    while (extra_edges > 0) {
        int u = vertex_dist(rng);
        int v = vertex_dist(rng);
        if (u != v) { edges.push_back({u, v, weight_dist(rng)}); extra_edges--; }
    }
    return edges;
}

vector<tuple<int, int, T>> generateHubGraph(int n, int m, int seed, int pct_hub = 20,
                                             int peso_min = 1, int peso_max = 1000) {
    mt19937 rng(seed);
    uniform_int_distribution<int> vertex_dist(1, n - 1);
    uniform_int_distribution<T> weight_dist(peso_min, peso_max);
    uniform_int_distribution<int> hub_choice(0, 99);
    vector<tuple<int, int, T>> edges;
    edges.reserve(m);
    int hub = 0;
    for (int i = 1; i < n; i++) edges.push_back({hub, i, weight_dist(rng)});
    int extra_edges = m - (n - 1);
    int hub_edges = 0;
    while (extra_edges > 0) {
        int u, v;
        if (hub_choice(rng) < pct_hub && hub_edges < m * 0.3) { u = hub; v = vertex_dist(rng); hub_edges++; }
        else { u = vertex_dist(rng); v = vertex_dist(rng); }
        if (u != v) { edges.push_back({u, v, weight_dist(rng)}); extra_edges--; }
    }
    return edges;
}

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
            if (dist[v] > d + w) { dist[v] = d + w; pq.push({dist[v], v}); }
        }
    }
    return dist;
}

int main(int argc, char** argv) {
    // argv: algo n m seed peso_min peso_max graph_type prepare_transform pct_hub out_dist_file
    if (argc < 11) {
        cerr << "uso: run_one algo n m seed peso_min peso_max graph_type prepare_transform pct_hub out_dist_file\n";
        return 1;
    }
    string algo = argv[1];
    int n = atoi(argv[2]);
    int m = atoi(argv[3]);
    int seed = atoi(argv[4]);
    int peso_min = atoi(argv[5]);
    int peso_max = atoi(argv[6]);
    string graph_type = argv[7];
    int prepare_transform = atoi(argv[8]);
    int pct_hub = atoi(argv[9]);
    string out_dist_file = argv[10];

    vector<tuple<int, int, T>> edges;
    if (graph_type == "hub") edges = generateHubGraph(n, m, seed, pct_hub, peso_min, peso_max);
    else edges = generateRandomGraph(n, m, seed, peso_min, peso_max);

    int source = 0;
    vector<T> dist;
    long long time_us = 0;
    long long heap_bytes = 0;

    if (algo == "dijkstra") {
        vector<vector<pair<int, T>>> adj(n);
        for (auto [u, v, w] : edges) { adj[u].push_back({v, w}); adj[v].push_back({u, w}); }

        g_bytes_allocated.store(0);
        auto t0 = high_resolution_clock::now();
        g_counting.store(true);
        dist = dijkstra(n, adj, source);
        g_counting.store(false);
        auto t1 = high_resolution_clock::now();
        heap_bytes = g_bytes_allocated.load();
        time_us = duration_cast<microseconds>(t1 - t0).count();
    } else {
        spp::bmssp<T> solver(n);
        for (auto [u, v, w] : edges) { solver.addEdge(u, v, w); solver.addEdge(v, u, w); }

        g_bytes_allocated.store(0);
        auto t0 = high_resolution_clock::now();
        g_counting.store(true);
        solver.prepare_graph(prepare_transform != 0);
        auto res = solver.execute(source);
        g_counting.store(false);
        auto t1 = high_resolution_clock::now();
        heap_bytes = g_bytes_allocated.load();
        dist = res.first;
        time_us = duration_cast<microseconds>(t1 - t0).count();
    }

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    long peak_rss_kb = ru.ru_maxrss; // KB en Linux

    ofstream fout(out_dist_file);
    for (int i = 0; i < n; i++) fout << dist[i] << "\n";
    fout.close();

    // salida: algo,n,m,time_us,heap_bytes,peak_rss_kb
    cout << algo << "," << n << "," << m << "," << time_us << ","
         << heap_bytes << "," << peak_rss_kb << endl;
    return 0;
}
