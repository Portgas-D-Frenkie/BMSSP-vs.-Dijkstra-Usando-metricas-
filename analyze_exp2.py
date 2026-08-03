import csv, numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

rows = list(csv.DictReader(open('results_7metricas.csv')))
exp2 = [r for r in rows if r['experimento']=='exp2']
m2 = np.array([float(r['aristas']) for r in exp2])
rss_dij2 = np.array([float(r['dijkstra_peak_rss_kb_mean']) for r in exp2])
rss_bms2 = np.array([float(r['bmssp_peak_rss_kb_mean']) for r in exp2])

fig, ax = plt.subplots(figsize=(5.2,3.6))
ax.plot(m2, rss_dij2, 'o-', label='Dijkstra', color='#1f77b4')
ax.plot(m2, rss_bms2, 's-', label='BMSSP', color='#d62728')
ax.set_xlabel('m (aristas), n=5000 fijo'); ax.set_ylabel('Memoria pico RSS (KB)')
ax.legend(); ax.grid(True, ls='--', alpha=0.4)
fig.tight_layout(); fig.savefig('fig_memoria_pico_exp2.png', dpi=150)
print("Peak RSS solo se diferencia claramente desde m=125000 en adelante (antes domina el piso base del proceso, ~12900KB)")

# resumen speedup memoria (ultima fila, m=250000)
last = exp2[-1]
ratio_rss = float(last['bmssp_peak_rss_kb_mean'])/float(last['dijkstra_peak_rss_kb_mean'])
print(f"En m=250000: BMSSP usa {ratio_rss:.3f}x la RSS pico de Dijkstra ({last['bmssp_peak_rss_kb_mean']} vs {last['dijkstra_peak_rss_kb_mean']} KB)")

heap_dij2 = np.array([float(r['dijkstra_heap_bytes_mean']) for r in exp2])
heap_bms2 = np.array([float(r['bmssp_heap_bytes_mean']) for r in exp2])
ratio_heap_avg = np.mean(heap_bms2/heap_dij2)
print(f"BMSSP asigna en promedio {ratio_heap_avg:.1f}x mas bytes de heap que Dijkstra durante la fase de solucion (Exp2)")
