#!/usr/bin/env python3
"""
Generatore di Grafici Accademici e Report Scientifico per la Tesi/Presentazione.
Produce 3 figure in formato PNG e PDF ad alta risoluzione:
1. fig_convergence_ucb1: Learning Curves con bande di confidenza Hoeffding nel Successive Rejects.
2. fig_ablation_c: Ablation Study / Curva di sensibilità della costante C (forma concava).
3. fig_oos_validation: Risultati del torneo out-of-sample di sfruttamento con p-value e IC al 95%.
"""

import json
import os
import sys
import numpy as np
import matplotlib
matplotlib.use('Agg') # Backend non interattivo per rendering da script
import matplotlib.pyplot as plt

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
RESULTS_DIR = os.path.join(REPO_ROOT, "training_results")
OUTPUT_DIR = RESULTS_DIR
UCB1_FILE = os.path.join(RESULTS_DIR, "bai_ucb1_results.json")
PUCT_FILE = os.path.join(RESULTS_DIR, "bai_puct_results.json")
OOS_FILE = os.path.join(RESULTS_DIR, "validation_oos_results.json")

# Stile accademico elegante
plt.rcParams.update({
    'font.sans-serif': 'Helvetica',
    'font.family': 'sans-serif',
    'figure.titlesize': 14,
    'axes.labelsize': 11,
    'axes.titlesize': 12,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 9,
    'figure.dpi': 300,
    'axes.grid': True,
    'grid.alpha': 0.3,
    'grid.linestyle': '--'
})

# Palette Blu Istituzionale Federico II
FED_BLUE = "#003366"
ACCENT_ORANGE = "#D9534F"
COLORS = ["#003366", "#2B7A78", "#3AAFA9", "#E07A5F", "#8E44AD", "#C0392B"]

def plot_convergence():
    if not os.path.exists(UCB1_FILE):
        print(f"File {UCB1_FILE} non trovato, salto grafico convergenza.")
        return

    with open(UCB1_FILE) as f:
        data = json.load(f)

    stats = data["all_stats"]
    fig, ax = plt.subplots(figsize=(8, 5))

    for idx, (arm_id, arm_data) in enumerate(stats.items()):
        traj = arm_data.get("trajectory", [])
        if not traj:
            continue
        
        # Campioniamo ogni 2 o 3 punti per pulizia
        xs = [p["n"] for p in traj]
        means = [p["mean"] * 100 for p in traj]
        errs = [p["std_err"] * 100 for p in traj]

        color = COLORS[idx % len(COLORS)]
        label = f"{arm_id} (α={arm_id})" # Default label
        # Recupera label esatta se disponibile
        for arm in data.get("all_stats", {}):
            if arm == arm_id and "label" in arm_data:
                label = f"{arm_id} {arm_data['label']}"

        is_best = (arm_id == data["best_arm"]["id"])
        lw = 2.5 if is_best else 1.2
        alpha_fill = 0.20 if is_best else 0.08

        line, = ax.plot(xs, means, label=f"{arm_id}", color=color, linewidth=lw, marker='o' if len(xs)<20 else None, markersize=3)
        lower = [max(0, m - e) for m, e in zip(means, errs)]
        upper = [min(100, m + e) for m, e in zip(means, errs)]
        ax.fill_between(xs, lower, upper, color=color, alpha=alpha_fill)

    ax.set_title("Tracking della Convergenza MCTS (Successive Rejects — UCB1)", fontweight='bold', color=FED_BLUE, pad=12)
    ax.set_xlabel("Numero di Partite Disputate ($n$)")
    ax.set_ylabel("Win Rate Empirico (%)")
    ax.set_ylim(-5, 105)
    ax.axhline(50, color='gray', linestyle=':', label="Baseline (50%)")
    ax.legend(title="Bracci Candidati", loc="upper right", frameon=True)

    plt.tight_layout()
    png_path = os.path.join(OUTPUT_DIR, "fig_convergence_ucb1.png")
    pdf_path = os.path.join(OUTPUT_DIR, "fig_convergence_ucb1.pdf")
    plt.savefig(png_path)
    plt.savefig(pdf_path)
    plt.close()
    print(f"Grafico convergenza salvato in: {png_path} e {pdf_path}")

def plot_ablation_c():
    if not os.path.exists(UCB1_FILE):
        return

    with open(UCB1_FILE) as f:
        data = json.load(f)

    stats = data["all_stats"]
    # Raggruppa per costante C
    c_vals = []
    win_rates = []
    errs = []
    labels = []

    # Cerchiamo di estrarre c e cutoff dai dati
    candidates = [
        ("Arm_1", 0.8, 30),
        ("Arm_2", 1.0, 40),
        ("Arm_3", 1.414, 30),
        ("Arm_4", 1.414, 50),
        ("Arm_5", 1.8, 40),
        ("Arm_6", 2.2, 50)
    ]

    for arm_id, c, cut in candidates:
        if arm_id in stats:
            g = stats[arm_id]["total_games"]
            if g > 0:
                wr = (stats[arm_id]["total_score"] / g) * 100
                se = 1.96 * np.sqrt(max(0.001, (wr/100)*(1 - wr/100)) / g) * 100
                c_vals.append(c)
                win_rates.append(wr)
                errs.append(se)
                labels.append(f"C={c}\n(cut={cut})")

    fig, ax = plt.subplots(figsize=(7, 4.5))

    # Ordina per c
    sorted_idx = np.argsort(c_vals)
    c_sorted = np.array(c_vals)[sorted_idx]
    wr_sorted = np.array(win_rates)[sorted_idx]
    err_sorted = np.array(errs)[sorted_idx]

    ax.errorbar(c_sorted, wr_sorted, yerr=err_sorted, fmt='-o', color=FED_BLUE, ecolor='salmon',
                elinewidth=2, capsize=5, capthick=1.5, linewidth=2, markersize=7, label="Win Rate Empirico ± 95% IC")

    # Evidenzia il massimo (ottimo empirico)
    max_idx = np.argmax(wr_sorted)
    ax.scatter([c_sorted[max_idx]], [wr_sorted[max_idx]], color=ACCENT_ORANGE, s=120, zorder=5, label=f"Ottimo: C={c_sorted[max_idx]} ({wr_sorted[max_idx]:.1f}%)")

    ax.set_title("Analisi di Sensibilità (Ablation Study) della Costante $C$", fontweight='bold', color=FED_BLUE, pad=12)
    ax.set_xlabel("Costante di Esplorazione $C$")
    ax.set_ylabel("Win Rate (%)")
    ax.set_ylim(0, 100)
    ax.axhline(50, color='gray', linestyle=':', label="Parità Teorica (50%)")
    ax.legend(loc="upper right", frameon=True)

    plt.tight_layout()
    png_path = os.path.join(OUTPUT_DIR, "fig_ablation_c.png")
    pdf_path = os.path.join(OUTPUT_DIR, "fig_ablation_c.pdf")
    plt.savefig(png_path)
    plt.savefig(pdf_path)
    plt.close()
    print(f"Grafico ablation salvato in: {png_path} e {pdf_path}")

def plot_oos_validation():
    if not os.path.exists(OOS_FILE):
        print(f"File {OOS_FILE} non trovato, salto grafico OOS.")
        return

    with open(OOS_FILE) as f:
        data = json.load(f)

    fig, ax = plt.subplots(figsize=(6.5, 4.5))

    best_arm_name = data["best_arm"]["label"]
    runner_up_name = data["runner_up"]["label"]

    categories = [f"Best Arm\n{best_arm_name}", f"Runner-Up\n{runner_up_name}", "Patte (Draw)"]
    p1_wins = data["wins"]
    draws = data["draws"]
    p1_losses = data["losses"]
    tot = data["num_games"]

    counts = [p1_wins, p1_losses, draws]
    pcts = [c / tot * 100 for c in counts]
    colors = [FED_BLUE, "#7F8C8D", "#BDC3C7"]

    bars = ax.bar(categories, pcts, color=colors, width=0.55, edgecolor='black', linewidth=0.8)

    # Aggiungi etichette percentuali sopra le barre
    for bar, pct, cnt in zip(bars, pcts, counts):
        yval = bar.get_height()
        ax.text(bar.get_x() + bar.get_width()/2.0, yval + 1.5, f"{cnt} ({pct:.1f}%)", ha='center', va='bottom', fontweight='bold', fontsize=9)

    p_val = data.get("p_value_one_tailed", 0.0)
    sig_text = f"p-value = {p_val:.3e} (p < 0.05: Significativo)" if p_val < 0.05 else f"p-value = {p_val:.3f} (Equivalenza)"
    
    ax.set_title(f"Validazione Out-of-Sample ({tot} Match Indipendenti)\n{sig_text}", fontweight='bold', color=FED_BLUE, pad=12)
    ax.set_ylabel("Frequenza Relativa (%)")
    ax.set_ylim(0, max(pcts) + 15)

    plt.tight_layout()
    png_path = os.path.join(OUTPUT_DIR, "fig_oos_validation.png")
    pdf_path = os.path.join(OUTPUT_DIR, "fig_oos_validation.pdf")
    plt.savefig(png_path)
    plt.savefig(pdf_path)
    plt.close()
    print(f"Grafico Out-of-Sample salvato in: {png_path} e {pdf_path}")

if __name__ == "__main__":
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    plot_convergence()
    plot_ablation_c()
    plot_oos_validation()
    print("\n✅ Generazione completata con successo in training_results/")
