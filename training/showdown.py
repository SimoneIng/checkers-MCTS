#!/usr/bin/env python3
"""
Grand Finale: Scontro Diretto tra il Miglior UCB1 e il Miglior PUCT
estratti tramite Best Arm Identification.
Supporta esecuzione parallela multi-core.
"""

import subprocess
import json
import os
import sys
import random
from concurrent.futures import ThreadPoolExecutor, as_completed

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BINARY_PATH = os.path.join(REPO_ROOT, "build", "play_match")
RESULTS_DIR = os.path.join(REPO_ROOT, "training_results")
UCB1_FILE = os.path.join(RESULTS_DIR, "bai_ucb1_results.json")
PUCT_FILE = os.path.join(RESULTS_DIR, "bai_puct_results.json")
SHOWDOWN_FILE = os.path.join(RESULTS_DIR, "showdown_results.json")

def play_showdown_match(idx, ucb1_res, puct_res, time_limit, seed):
    ucb1_is_white = (idx % 2 == 0)
    p1_pol = 0 if ucb1_is_white else 1
    p1_c = ucb1_res["c"] if ucb1_is_white else puct_res["c"]
    p1_cut = ucb1_res["cutoff"] if ucb1_is_white else puct_res["cutoff"]

    p2_pol = 1 if ucb1_is_white else 0
    p2_c = puct_res["c"] if ucb1_is_white else ucb1_res["c"]
    p2_cut = puct_res["cutoff"] if ucb1_is_white else ucb1_res["cutoff"]

    cmd = [
        BINARY_PATH,
        "--p1-policy", str(p1_pol), "--p1-c", str(p1_c), "--p1-cutoff", str(p1_cut),
        "--p2-policy", str(p2_pol), "--p2-c", str(p2_c), "--p2-cutoff", str(p2_cut),
        "--time", str(time_limit),
        "--seed", str(seed)
    ]

    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
    winner = "DRAW"
    for line in proc.stdout.splitlines():
        if line.startswith("RESULT:"):
            winner = line.split()[0].split(":")[1]
            break

    if winner == "WHITE":
        if ucb1_is_white:
            return idx, "UCB1", "Vince UCB1 (Bianco)"
        else:
            return idx, "PUCT", "Vince PUCT (Bianco)"
    elif winner == "BLACK":
        if ucb1_is_white:
            return idx, "PUCT", "Vince PUCT (Nero)"
        else:
            return idx, "UCB1", "Vince UCB1 (Nero)"
    else:
        return idx, "DRAW", "Patta"

def main(num_games=20, time_limit=0.03, workers=4):
    if not os.path.exists(UCB1_FILE) or not os.path.exists(PUCT_FILE):
        print(f"Errore: Esegui prima 'python3 tune_ucb1.py' e 'python3 tune_puct.py' per estrarre i campioni.", file=sys.stderr)
        sys.exit(1)

    with open(UCB1_FILE) as f:
        ucb1_res = json.load(f)["best_arm"]
    with open(PUCT_FILE) as f:
        puct_res = json.load(f)["best_arm"]

    print("\n" + "="*70)
    print(" ⚔️ GRAND FINALE: CAMPIONE UCB1 vs CAMPIONE PUCT")
    print("="*70)
    print(f" • Campione UCB1: {ucb1_res['label']}")
    print(f" • Campione PUCT: {puct_res['label']}")
    print(f" • Match da disputare: {num_games} partite (perfettamente alternate Bianco/Nero)")
    print(f" • Tempo per mossa: {time_limit}s")
    print(f" • Parallelismo: {workers} worker attivi")
    print("-" * 70)

    ucb1_wins = 0
    puct_wins = 0
    draws = 0
    results = [None] * num_games

    with ThreadPoolExecutor(max_workers=workers) as executor:
        futures = {
            executor.submit(
                play_showdown_match, i, ucb1_res, puct_res, time_limit, random.randint(1, 2147483647)
            ): i for i in range(num_games)
        }
        for future in as_completed(futures):
            idx, victor, desc = future.result()
            results[idx] = (victor, desc)
            if victor == "UCB1":
                ucb1_wins += 1
            elif victor == "PUCT":
                puct_wins += 1
            else:
                draws += 1

    for i, (_, desc) in enumerate(results):
        print(f" Match {i+1:2d}/{num_games}: {desc}")

    print("\n" + "="*70)
    print(" 📊 ESITO FINALE")
    print("="*70)
    print(f" • UCB1: {ucb1_wins} vittorie ({ucb1_wins/num_games*100:.1f}%)")
    print(f" • PUCT: {puct_wins} vittorie ({puct_wins/num_games*100:.1f}%)")
    print(f" • Patte: {draws} ({draws/num_games*100:.1f}%)")
    
    overall_winner = "PUCT" if puct_wins > ucb1_wins else ("UCB1" if ucb1_wins > puct_wins else "PARITÀ")
    print(f" 👑 Vincitore Assoluto: {overall_winner}")
    print("="*70 + "\n")

    summary = {
        "ucb1_best": ucb1_res,
        "puct_best": puct_res,
        "games_played": num_games,
        "ucb1_wins": ucb1_wins,
        "puct_wins": puct_wins,
        "draws": draws,
        "winner": overall_winner
    }

    os.makedirs(RESULTS_DIR, exist_ok=True)
    with open(SHOWDOWN_FILE, "w") as f:
        json.dump(summary, f, indent=2)
    print(f"Risultati salvati in: {SHOWDOWN_FILE}")

if __name__ == "__main__":
    main()
