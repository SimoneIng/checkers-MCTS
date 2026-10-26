#!/usr/bin/env python3
"""
Tuning UCB1 tramite Best Arm Identification (Successive Rejects).
Ottimizza congiuntamente le coppie (Alpha / Costante C, Rollout Cutoff).
Include:
- Tracciamento della traiettoria partita-per-partita con intervalli di confidenza (Hoeffding).
- Calcolo teorico della complessità H2 e del bound di errore formale.
- Esecuzione parallela multi-core con seed casuali indipendenti.
"""

import subprocess
import math
import sys
import os
import json
import time
import random
from concurrent.futures import ThreadPoolExecutor, as_completed

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BINARY_PATH = os.path.join(REPO_ROOT, "build", "play_match")
RESULTS_DIR = os.path.join(REPO_ROOT, "training_results")
OUTPUT_FILE = os.path.join(RESULTS_DIR, "bai_ucb1_results.json")

# Definizione dei Bracci Candidati: (alpha / costante_c, cutoff)
CANDIDATE_ARMS = [
    {"id": "Arm_1", "c": 0.8,   "cutoff": 30, "label": "(α=0.8,  cutoff=30)"},
    {"id": "Arm_2", "c": 1.0,   "cutoff": 40, "label": "(α=1.0,  cutoff=40)"},
    {"id": "Arm_3", "c": 1.414, "cutoff": 30, "label": "(α=1.41, cutoff=30)"},
    {"id": "Arm_4", "c": 1.414, "cutoff": 50, "label": "(α=1.41, cutoff=50)"},
    {"id": "Arm_5", "c": 1.8,   "cutoff": 40, "label": "(α=1.8,  cutoff=40)"},
    {"id": "Arm_6", "c": 2.2,   "cutoff": 50, "label": "(α=2.2,  cutoff=50)"},
]

# Baseline standard di confronto (Bot di riferimento equilibrato)
BASELINE = {"policy": 0, "c": 1.414, "cutoff": 40}

def play_game(arm_c, arm_cutoff, as_white=True, time_limit=0.02, seed=None):
    if seed is None:
        seed = random.randint(1, 2147483647)

    if as_white:
        cmd = [
            BINARY_PATH,
            "--p1-policy", "0", "--p1-c", str(arm_c), "--p1-cutoff", str(arm_cutoff),
            "--p2-policy", str(BASELINE["policy"]), "--p2-c", str(BASELINE["c"]), "--p2-cutoff", str(BASELINE["cutoff"]),
            "--time", str(time_limit),
            "--seed", str(seed)
        ]
    else:
        cmd = [
            BINARY_PATH,
            "--p1-policy", str(BASELINE["policy"]), "--p1-c", str(BASELINE["c"]), "--p2-cutoff", str(BASELINE["cutoff"]),
            "--p2-policy", "0", "--p2-c", str(arm_c), "--p2-cutoff", str(arm_cutoff),
            "--time", str(time_limit),
            "--seed", str(seed)
        ]

    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
        for line in proc.stdout.splitlines():
            if line.startswith("RESULT:"):
                parts = line.split()
                res = parts[0].split(":")[1]
                if res == "WHITE":
                    return 1.0 if as_white else 0.0
                elif res == "BLACK":
                    return 0.0 if as_white else 1.0
                else:
                    return 0.5
    except Exception as e:
        print(f"Errore match: {e}", file=sys.stderr)
        return 0.5

    return 0.5

def run_successive_rejects(budget_games=400, time_limit=0.02, workers=4):
    """
    Algoritmo Successive Rejects con tracking della convergenza partita-per-partita.
    """
    K = len(CANDIDATE_ARMS)
    log_bar_K = 0.5 + sum(1.0 / i for i in range(2, K + 1))

    # n_k = ceil( (1 / log_bar_K) * ((T - K) / (K + 1 - k)) )
    n_phases = [0]
    for k in range(1, K):
        allocated = math.ceil((1.0 / log_bar_K) * ((budget_games - K) / (K + 1 - k)))
        n_phases.append(allocated)

    print("\n" + "="*75)
    print(" 🎯 BEST ARM IDENTIFICATION (SUCCESSIVE REJECTS) — UCB1 TUNING (T=400)")
    print("="*75)
    print(f"Bracci candidati (K): {K}")
    print(f"Budget totale allocato: ~{budget_games} partite")
    print(f"Tempo mossa per partita: {time_limit}s")
    print(f"Parallelismo: {workers} worker attivi")
    print("-" * 75)

    active_arms = list(CANDIDATE_ARMS)
    stats = {
        arm["id"]: {
            "wins": 0, "draws": 0, "losses": 0, 
            "total_score": 0.0, "total_games": 0,
            "trajectory": [] # Salva (n, mean, hoeffding_ci, std_err) a ogni match
        }
        for arm in CANDIDATE_ARMS
    }

    history_log = []
    prev_n = 0
    t_start = time.time()

    for phase in range(1, K):
        current_n = n_phases[phase]
        delta_games = max(2, current_n - prev_n)
        if delta_games % 2 != 0:
            delta_games += 1

        total_phase_matches = len(active_arms) * delta_games
        print(f"\n▶ FASE {phase}/{K-1} : {len(active_arms)} bracci in gara (+{delta_games} partite ciascuno = {total_phase_matches} match)")

        jobs = []
        for arm in active_arms:
            arm_id = arm["id"]
            half = delta_games // 2
            for _ in range(half):
                jobs.append((arm_id, arm["c"], arm["cutoff"], True, time_limit, random.randint(1, 2147483647)))
            for _ in range(half):
                jobs.append((arm_id, arm["c"], arm["cutoff"], False, time_limit, random.randint(1, 2147483647)))

        # Shuffle dei job per evitare bias di schedulazione
        random.shuffle(jobs)

        completed = 0
        with ThreadPoolExecutor(max_workers=workers) as executor:
            future_to_job = {
                executor.submit(play_game, j[1], j[2], j[3], j[4], j[5]): j[0]
                for j in jobs
            }
            for future in as_completed(future_to_job):
                arm_id = future_to_job[future]
                score = future.result()
                stats[arm_id]["total_score"] += score
                stats[arm_id]["total_games"] += 1
                if score == 1.0:
                    stats[arm_id]["wins"] += 1
                elif score == 0.5:
                    stats[arm_id]["draws"] += 1
                else:
                    stats[arm_id]["losses"] += 1

                n = stats[arm_id]["total_games"]
                mean = stats[arm_id]["total_score"] / n
                # Hoeffding confidence bound al 95% (delta = 0.05): eps = sqrt(ln(2/delta) / (2n))
                hoeffding_eps = math.sqrt(math.log(40.0) / (2.0 * n))
                # Standard error
                std_err = 1.96 * math.sqrt(max(0.001, mean * (1.0 - mean)) / n)

                stats[arm_id]["trajectory"].append({
                    "n": n,
                    "mean": round(mean, 4),
                    "hoeffding_eps": round(hoeffding_eps, 4),
                    "ci_lower": round(max(0.0, mean - hoeffding_eps), 4),
                    "ci_upper": round(min(1.0, mean + hoeffding_eps), 4),
                    "std_err": round(std_err, 4)
                })

                completed += 1
                if completed % max(1, total_phase_matches // 10) == 0 or completed == total_phase_matches:
                    sys.stdout.write(f"\r  Progresso: {completed}/{total_phase_matches} match ({completed*100//total_phase_matches}%)")
                    sys.stdout.flush()

        print()
        for arm in active_arms:
            arm_id = arm["id"]
            g = stats[arm_id]["total_games"]
            wr = (stats[arm_id]["total_score"] / g) * 100 if g > 0 else 0
            print(f"  • {arm['id']} {arm['label']:<20} | Score: {stats[arm_id]['total_score']:5.1f}/{g:<3} ({wr:5.1f}%) [W:{stats[arm_id]['wins']:<2} D:{stats[arm_id]['draws']:<2} L:{stats[arm_id]['losses']:<2}]")

        worst_arm = min(active_arms, key=lambda a: stats[a["id"]]["total_score"] / stats[a["id"]]["total_games"])
        worst_wr = (stats[worst_arm["id"]]["total_score"] / stats[worst_arm["id"]]["total_games"]) * 100
        
        print(f"  ❌ SCARTATO alla Fase {phase}: {worst_arm['id']} {worst_arm['label']} con Win Rate {worst_wr:.1f}%")
        
        history_log.append({
            "phase": phase,
            "rejected_arm": worst_arm,
            "score": stats[worst_arm["id"]]["total_score"],
            "games": stats[worst_arm["id"]]["total_games"],
            "win_rate": worst_wr
        })

        active_arms.remove(worst_arm)
        prev_n = current_n

    elapsed = time.time() - t_start

    best_arm = active_arms[0]
    best_stats = stats[best_arm["id"]]
    best_wr = (best_stats["total_score"] / best_stats["total_games"]) * 100

    # =========================================================================
    # CALCOLO TEORICO DELLA COMPLESSITÀ H2 E DEL BOUND DI ERRORE
    # =========================================================================
    # Ordiniamo tutti i bracci per media empirica decrescente
    sorted_arms = sorted(CANDIDATE_ARMS, key=lambda a: stats[a["id"]]["total_score"] / stats[a["id"]]["total_games"], reverse=True)
    mu_star = stats[sorted_arms[0]["id"]]["total_score"] / stats[sorted_arms[0]["id"]]["total_games"]
    
    gaps = {}
    h2_terms = []
    for rank, arm in enumerate(sorted_arms[1:], start=2):
        mu_i = stats[arm["id"]]["total_score"] / stats[arm["id"]]["total_games"]
        delta_i = max(0.02, mu_star - mu_i) # Floor a 0.02 per stabilità
        gaps[arm["id"]] = delta_i
        term = rank / (delta_i ** 2)
        h2_terms.append((arm["id"], term))

    h2_hat = max(t[1] for t in h2_terms) if h2_terms else 1.0
    theoretical_error_prob = (K * (K - 1) / 2.0) * math.exp(-(budget_games - K) / (log_bar_K * h2_hat))
    confidence_level = max(0.0, 1.0 - theoretical_error_prob) * 100

    print("\n" + "="*75)
    print(" 🏆 VINCITORE BEST ARM IDENTIFICATION (UCB1)")
    print("="*75)
    print(f" Braccio Eletto: {best_arm['id']} -> {best_arm['label']}")
    print(f" Parametri Ottimi: Costante Alpha = {best_arm['c']}, Cutoff = {best_arm['cutoff']}")
    print(f" Score Finale: {best_stats['total_score']}/{best_stats['total_games']} ({best_wr:.1f}% Win Rate)")
    print(f" Complessità Empirica H2: {h2_hat:.2f}")
    print(f" Bound Teorico Probabilità d'Errore P(err) <= {theoretical_error_prob:.4f} (Confidenza: {confidence_level:.1f}%)")
    print(f" Tempo Totale Tuning: {elapsed:.1f}s")
    print("="*75 + "\n")

    output_data = {
        "policy": "UCB1",
        "budget_games": budget_games,
        "best_arm": best_arm,
        "best_win_rate": best_wr,
        "h2_complexity": round(h2_hat, 2),
        "theoretical_error_bound": round(theoretical_error_prob, 5),
        "confidence_level_pct": round(confidence_level, 2),
        "all_stats": stats,
        "history": history_log,
        "elapsed_seconds": round(elapsed, 1)
    }
    
    os.makedirs(RESULTS_DIR, exist_ok=True)
    with open(OUTPUT_FILE, "w") as f:
        json.dump(output_data, f, indent=2)
    print(f"Risultati completi salvati in: {OUTPUT_FILE}")

    return best_arm

if __name__ == "__main__":
    if not os.path.exists(BINARY_PATH):
        print(f"Errore: Eseguibile non trovato in {BINARY_PATH}. Esegui 'make' prima.", file=sys.stderr)
        sys.exit(1)
    
    run_successive_rejects(budget_games=400, time_limit=0.02, workers=4)
