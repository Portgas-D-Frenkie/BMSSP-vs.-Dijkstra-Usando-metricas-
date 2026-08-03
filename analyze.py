import csv, numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

rows = list(csv.DictReader(open('results_7metricas.csv')))
exp1 = [r for r in rows if r['experimento']=='exp1']
exp2 = [r for r in rows if r['experimento']=='exp2']

n1 = np.array([float(r['vertices']) for r in exp1])
t_dij1 = np.array([float(r['dijkstra_time_us_mean']) for r in exp1])
t_bms1 = np.array([float(r['bmssp_time_us_mean']) for r in exp1])

# --- Metrica 6: tasa de crecimiento empirica (ley de potencia t = c * n^p) ---
def power_fit(x, y):
    lx, ly = np.log(x), np.log(y)
    p, logc = np.polyfit(lx, ly, 1)
    c = np.exp(logc)
    pred = c * x**p
    r2 = 1 - np.sum((y-pred)**2)/np.sum((y-np.mean(y))**2)
    return p, c, r2

p_dij, c_dij, r2_dij = power_fit(n1, t_dij1)
p_bms, c_bms, r2_bms = power_fit(n1, t_bms1)
print(f"Dijkstra: t ~ {c_dij:.4f} * n^{p_dij:.4f}  (R2={r2_dij:.4f})")
print(f"BMSSP:    t ~ {c_bms:.4f} * n^{p_bms:.4f}  (R2={r2_bms:.4f})")

# --- Metrica 7: punto de cruce (si p_bms < p_dij) ---
if p_bms < p_dij:
    n_cross = (c_bms/c_dij)**(1/(p_dij-p_bms))
    print(f"Punto de cruce (modelo potencia, 10 reps): n ~ {n_cross:.3e}")
else:
    print("Modelo de potencia no predice cruce finito (BMSSP no decrece su exponente por debajo de Dijkstra)")

with open("growth_analysis.txt","w") as f:
    f.write(f"Dijkstra: t ~ {c_dij:.4f} * n^{p_dij:.4f}  (R2={r2_dij:.4f})\n")
    f.write(f"BMSSP:    t ~ {c_bms:.4f} * n^{p_bms:.4f}  (R2={r2_bms:.4f})\n")
    if p_bms < p_dij:
        n_cross = (c_bms/c_dij)**(1/(p_dij-p_bms))
        f.write(f"Punto de cruce estimado (extrapolacion, modelo potencia): n ~ {n_cross:.3e}\n")
    else:
        f.write("Sin cruce finito predicho por el modelo de potencia en este rango.\n")

# --- Figuras ---
plt.rcParams.update({'font.size':11})

fig, ax = plt.subplots(figsize=(5.2,3.6))
ax.plot(n1, t_dij1, 'o-', label='Dijkstra', color='#1f77b4')
ax.plot(n1, t_bms1, 's-', label='BMSSP', color='#d62728')
ax.set_xscale('log'); ax.set_yscale('log')
ax.set_xlabel('n (vertices)'); ax.set_ylabel('Tiempo medio (us), 10 rep.')
ax.legend(); ax.grid(True, which='both', ls='--', alpha=0.4)
fig.tight_layout(); fig.savefig('fig_tiempo_exp1.png', dpi=150)

speedup1 = t_bms1/t_dij1
fig, ax = plt.subplots(figsize=(5.2,3.6))
ax.plot(n1, speedup1, 'D-', color='#2ca02c')
ax.axhline(1.0, color='gray', ls=':')
ax.set_xlabel('n (vertices)'); ax.set_ylabel('Speedup BMSSP/Dijkstra')
ax.set_xscale('log')
ax.grid(True, ls='--', alpha=0.4)
fig.tight_layout(); fig.savefig('fig_speedup_exp1.png', dpi=150)

heap_dij1 = np.array([float(r['dijkstra_heap_bytes_mean']) for r in exp1])
heap_bms1 = np.array([float(r['bmssp_heap_bytes_mean']) for r in exp1])
fig, ax = plt.subplots(figsize=(5.2,3.6))
ax.plot(n1, heap_dij1/1024, 'o-', label='Dijkstra', color='#1f77b4')
ax.plot(n1, heap_bms1/1024, 's-', label='BMSSP', color='#d62728')
ax.set_xscale('log'); ax.set_yscale('log')
ax.set_xlabel('n (vertices)'); ax.set_ylabel('Heap asignado (KB)')
ax.legend(); ax.grid(True, which='both', ls='--', alpha=0.4)
fig.tight_layout(); fig.savefig('fig_memoria_estructura_exp1.png', dpi=150)

print("Figuras generadas.")

# --- Metrica 4: memoria pico (peak RSS) ---
rss_dij1 = np.array([float(r['dijkstra_peak_rss_kb_mean']) for r in exp1])
rss_bms1 = np.array([float(r['bmssp_peak_rss_kb_mean']) for r in exp1])
fig, ax = plt.subplots(figsize=(5.2,3.6))
ax.plot(n1, rss_dij1, 'o-', label='Dijkstra', color='#1f77b4')
ax.plot(n1, rss_bms1, 's-', label='BMSSP', color='#d62728')
ax.set_xlabel('n (vertices)'); ax.set_ylabel('Memoria pico RSS (KB)')
ax.legend(); ax.grid(True, ls='--', alpha=0.4)
fig.tight_layout(); fig.savefig('fig_memoria_pico_exp1.png', dpi=150)
print("RSS Dijkstra:", rss_dij1)
print("RSS BMSSP:", rss_bms1)
