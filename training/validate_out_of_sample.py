#!/usr/bin/env python3
"""
Validazione Out-of-Sample (Torneo di Puro Sfruttamento).
Verifica statisticamente se il Best Arm raccomandato da Successive Rejects
mantiene un vantaggio statisticamente significativo (p-value < 0.05) rispetto al Runner-up (o Baseline)
su un campione indipendente di 200 partite a colori perfettamente alternati.
"""

import subprocess
import math
import sys
import os
import json
import time
import random
from concurrent.futures import ThreadPoolExecutor, as_completed
from scipy import stats as sp_stats

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BINARY_PATH = os.path.join(REPO_ROOT, "build", "play_match")
RESULTS_DIR = os.path.join(REPO_ROOT, "training_results")
UCB1_FILE = os.path.join(RESULTS_DIR, "bai_ucb1_results.json")
OOS_FILE = os.path.join(RESULTS_DIR, "validation_oos_results.json")

def play_match(p1, p2, p1_is_white, time_limit, seed):
    pol1, c1, cut1 = p1["policy"], p1["c"], p1["cutoff"]
    pol2, c2, cut2 = p2["policy"], p2["c"], p2["cutoff"]

    if p1_is_white:
        cmd = [
            BINARY_PATH,
            "--p1-policy", str(pol1), "--p1-c", str(c1), "--p1-cutoff", str(cut1),
            "--p2-policy", str(pol2), "--p2-c", str(c2), "--p2-cutoff", str(cut2),
            "--time", str(time_limit),
            "--seed", str(seed)
        ]
    else:
        cmd = [
            BINARY_PATH,
            "--p1-policy", str(pol2), "--p1-c", str(c2), "--p1-cutoff", str(cut2),
            "--p2-policy", str(pol1), "--p2-c", str(c1), "--p2-cutoff", str(cut1),
            "--time", str(time_limit),
            "--seed", str(seed)
        ]

    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        winner = "DRAW"
        for line in proc.stdout.splitlines():
            if line.startswith("RESULT:"):
                winner = line.split()[0].split(":")[1]
                break

        if winner == "WHITE":
            return 1.0 if p1_is_white else 0.0
        elif winner == "BLACK":
            return 0.0 if p1_is_white else 1.0
        else:
            return 0.5
    except Exception as e:
        print(f"Errore match OOS: {e}", file=sys.stderr)
        return 0.5

def run_out_of_sample_validation(num_games=200, time_limit=0.02, workers=4):
    if not os.path.exists(UCB1_FILE):
        print(f"Errore: File {UCB1_FILE} non trovato. Esegui prima 'tune_ucb1.py'.", file=sys.stderr)
        sys.exit(1)

    with open(UCB1_FILE) as f:
        bai_data = json.load(f)

    # Identifica il Best Arm e il Runner-Up (l'ultimo braccio eliminato prima della vittoria)
    best_arm = bai_data["best_arm"]
    best_arm["policy"] = 0

    history = bai_data.get("history", [])
    if history:
        runner_up = history[-1]["rejected_arm"]
    else:
        runner_up = {"id": "Baseline", "c": 1.414, "cutoff": 40, "label": "(α=1.41, cutoff=40)"}
    runner_up["policy"] = 0

    print("\n" + "="*75)
    print(" 🔬 VALIDAZIONE OUT-OF-SAMPLE: TORNEO DI SFRUTTAMENTO INDIPENDENTE")
    print("="*75)
    print(f" • Campione (Best Arm): {best_arm['id']} -> {best_arm['label']}")
    print(f" • Sfidante (Runner-Up): {runner_up['id']} -> {runner_up['label']}")
    print(f" • Campione di validazione: {num_games} partite (100 Bianco, 100 Nero)")
    print(f" • Tempo per mossa: {time_limit}s | Parallelismo: {workers} worker")
    print("-" * 75)

    jobs = []
    half = num_games // 2
    for _ in range(half):
        jobs.append((True, time_limit, random.randint(1, 2147483647)))
    for _ in range(half):
        jobs.append((False, time_limit, random.randint(1, 2147483647)))
    random.shuffle(jobs)

    p1_wins = 0
    p1_draws = 0
    p1_losses = 0
    completed = 0
    trajectory = []

    t_start = time.time()
    with ThreadPoolExecutor(max_workers=workers) as executor:
        futures = [
            executor.submit(play_match, best_arm, runner_up, j[0], j[1], j[2])
            for j in jobs
        ]
        for future in as_completed(futures):
            res = future.result()
            if res == 1.0:
                p1_wins += 1
            elif res == 0.5:
                p1_draws += 1
            else:
                p1_losses += 1

            completed += 1
            cur_score = p1_wins + 0.5 * p1_draws
            cur_rate = cur_score / completed
            trajectory.append({
                "n": completed,
                "score": cur_score,
                "win_rate": round(cur_rate, 4)
            })

            if completed % max(1, num_games // 10) == 0 or completed == num_games:
                sys.stdout.write(f"\r  Progresso: {completed}/{num_games} match completati ({completed*100//num_games}%)")
                sys.stdout.flush()

    elapsed = time.time() - t_start
    print()

    total_score = p1_wins + 0.5 * p1_draws
    win_rate = (total_score / num_games) * 100
    effective_wins = p1_wins + 0.5 * p1_draws

    # =========================================================================
    # TEST D'IPOTESI STATISTICA (p-value e Intervallo di Confidenza)
    # =========================================================================
    # H0: p = 0.5 (Nessuna differenza di forza tra i due bracci)
    # H1: p > 0.5 (Il Best Arm è statisticamente superiore)
    z_score = (total_score - (num_games * 0.5)) / math.sqrt(num_games * 0.25)
    p_value_one_tailed = sp_stats.norm.sf(z_score) # Test a una coda (superiorità)
    p_value_two_tailed = 2 * sp_stats.norm.sf(abs(z_score)) # Test a due code (differenza)

    # 95% Confidence Interval (Wilson Score Interval)
    p_hat = total_score / num_games
    z_crit = 1.96
    denominator = 1 + (z_crit**2) / num_games
    centre = (p_hat + (z_crit**2) / (2 * num_games)) / denominator
    margin = (z_crit * math.sqrt((p_hat * (1 - p_hat) / num_games) + (z_crit**2) / (4 * (num_games**2)))) / denominator
    ci_lower = max(0.0, centre - margin) * 100
    ci_upper = min(1.0, centre + margin) * 100

    is_significant = p_value_one_tailed < 0.05

    print("\n" + "="*75)
    print(" 📊 ESITO DEL TEST DI SFRUTTAMENTO OUT-OF-SAMPLE")
    print("="*75)
    print(f" • Record Best Arm: {p1_wins} Vinte, {p1_draws} Patte, {p1_losses} Perse")
    print(f" • Punteggio Totale: {total_score:.1f} / {num_games} ({win_rate:.2f}%)")
    print(f" • 95% Intervallo di Confidenza: [{ci_lower:.2f}%, {ci_upper:.2f}%]")
    print(f" • Statistica Z: z = {z_score:.3f}")
    print(f" • p-value (Test a una coda H1: p > 0.5): p = {p_value_one_tailed:.4e}")
    print(f" • p-value (Test a due code): p = {p_value_two_tailed:.4e}")
    if is_significant:
        print(f" ✅ RISULTATO SIGNIFICATIVO (p < 0.05): L'ipotesi nulla è respinta!")
        print(f"    Il Best Arm {best_arm['id']} è genuinamente superiore al Runner-up.")
    else:
        print(f" ℹ️ p >= 0.05: La differenza tra i due finalisti rientra nella soglia di equivalenza.")
    print(f" • Tempo Esecuzione: {elapsed:.1f}s")
    print("="*75 + "\n")

    oos_results = {
        "num_games": int(num_games),
        "best_arm": best_arm,
        "runner_up": runner_up,
        "wins": int(p1_wins),
        "draws": int(p1_draws),
        "losses": int(p1_losses),
        "total_score": float(total_score),
        "win_rate_pct": round(float(win_rate), 2),
        "ci_95_lower": round(float(ci_lower), 2),
        "ci_95_upper": round(float(ci_upper), 2),
        "z_score": round(float(z_score), 4),
        "p_value_one_tailed": float(p_value_one_tailed),
        "p_value_two_tailed": float(p_value_two_tailed),
        "statistically_significant": bool(is_significant),
        "trajectory": trajectory
    }

    os.makedirs(RESULTS_DIR, exist_ok=True)
    with open(OOS_FILE, "w") as f:
        json.dump(oos_results, f, indent=2)
    print(f"Risultati salvati in: {OOS_FILE}")

if __name__ == "__main__":
    run_out_of_sample_validation(num_games=200, time_limit=0.02, workers=4)
