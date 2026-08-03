#!/usr/bin/env python3
"""
Orquestador de experimentos: corre run_one (Dijkstra y BMSSP) para las
17 instancias ORIGINALES del articulo (Exp1: n=100..6400, Exp2: m=25000..250000
con n=5000 fijo), con REPS repeticiones cada una, en procesos aislados para
medir memoria pico de forma limpia.

Usa exactamente la misma formula de semilla que experiments.cpp original:
  Exp1: seed = 2025 + n
  Exp2: seed = 2025 + d   (d = densidad = m/n)
"""
import subprocess, csv, statistics, sys, os

REPS = 10
BIN = "./run_one"
OUTDIR = "dist_tmp"
os.makedirs(OUTDIR, exist_ok=True)

# Mismas 17 instancias reportadas en experiment_results.csv original
exp1 = [(n, n * 10, 2025 + n) for n in [100, 200, 400, 800, 1600, 3200, 6400]]
exp2 = [(5000, 5000 * d, 2025 + d) for d in [5, 10, 15, 20, 25, 30, 35, 40, 45, 50]]
instances = [("exp1", n, m, seed) for n, m, seed in exp1] + \
            [("exp2", n, m, seed) for n, m, seed in exp2]

def run_once(algo, n, m, seed, tag):
    out_file = f"{OUTDIR}/{algo}_{tag}_{n}_{m}.txt"
    r = subprocess.run([BIN, algo, str(n), str(m), str(seed), "1", "1000",
                         "random", "0", "20", out_file],
                        capture_output=True, text=True, timeout=300)
    if r.returncode != 0:
        print("ERROR:", r.stderr, file=sys.stderr)
        sys.exit(1)
    parts = r.stdout.strip().split(",")
    return {
        "algo": parts[0], "n": int(parts[1]), "m": int(parts[2]),
        "time_us": int(parts[3]), "heap_bytes": int(parts[4]),
        "peak_rss_kb": int(parts[5]), "dist_file": out_file
    }

rows = []
for exp_name, n, m, seed in instances:
    print(f"[{exp_name}] n={n} m={m} seed={seed}", flush=True)

    dij_runs, bms_runs = [], []
    dij_dist_file, bms_dist_file = None, None

    for rep in range(REPS):
        d = run_once("dijkstra", n, m, seed, f"r{rep}")
        b = run_once("bmssp", n, m, seed, f"r{rep}")
        dij_runs.append(d)
        bms_runs.append(b)
        if rep == 0:
            dij_dist_file, bms_dist_file = d["dist_file"], b["dist_file"]

    # Correctitud: comparar distancias exactas (misma semilla -> mismo grafo)
    with open(dij_dist_file) as f1, open(bms_dist_file) as f2:
        correcto = f1.read() == f2.read()

    def stats(runs, key):
        vals = [r[key] for r in runs]
        mean = statistics.mean(vals)
        std = statistics.stdev(vals) if len(vals) > 1 else 0.0
        return mean, std, min(vals), max(vals)

    dij_t_mean, dij_t_std, dij_t_min, dij_t_max = stats(dij_runs, "time_us")
    bms_t_mean, bms_t_std, bms_t_min, bms_t_max = stats(bms_runs, "time_us")
    dij_heap_mean, _, _, _ = stats(dij_runs, "heap_bytes")
    bms_heap_mean, _, _, _ = stats(bms_runs, "heap_bytes")
    dij_rss_mean, dij_rss_std, _, dij_rss_max = stats(dij_runs, "peak_rss_kb")
    bms_rss_mean, bms_rss_std, _, bms_rss_max = stats(bms_runs, "peak_rss_kb")

    rows.append({
        "experimento": exp_name, "vertices": n, "aristas": m, "seed": seed, "repeticiones": REPS,
        "dijkstra_time_us_mean": round(dij_t_mean, 2), "dijkstra_time_us_std": round(dij_t_std, 2),
        "dijkstra_time_us_min": dij_t_min, "dijkstra_time_us_max": dij_t_max,
        "bmssp_time_us_mean": round(bms_t_mean, 2), "bmssp_time_us_std": round(bms_t_std, 2),
        "bmssp_time_us_min": bms_t_min, "bmssp_time_us_max": bms_t_max,
        "speedup_bmssp_vs_dijkstra": round(bms_t_mean / dij_t_mean, 4),
        "dijkstra_heap_bytes_mean": round(dij_heap_mean, 1),
        "bmssp_heap_bytes_mean": round(bms_heap_mean, 1),
        "dijkstra_peak_rss_kb_mean": round(dij_rss_mean, 2), "dijkstra_peak_rss_kb_max": dij_rss_max,
        "bmssp_peak_rss_kb_mean": round(bms_rss_mean, 2), "bmssp_peak_rss_kb_max": bms_rss_max,
        "correcto": "OK" if correcto else "ERROR",
    })
    print(f"   dijkstra={dij_t_mean:.1f}us  bmssp={bms_t_mean:.1f}us  "
          f"speedup={bms_t_mean/dij_t_mean:.2f}x  correcto={'OK' if correcto else 'ERROR'}", flush=True)

fieldnames = list(rows[0].keys())
with open("results_7metricas.csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=fieldnames)
    w.writeheader()
    w.writerows(rows)

print("\nListo -> results_7metricas.csv")
