# Pipeline di Tuning e Sperimentazione Accademica 🧪📊

Questa cartella raccoglie l'intera suite di script Python per l'ottimizzazione degli iperparametri, il testing statistico e la generazione dei report scientifici per il motore di Dama MCTS.

Tutti gli script sono completamente multi-core e comunicano direttamente con il runner nativo C ad alte prestazioni compilato in `build/play_match`.

---

## 📁 Contenuto della Cartella

| Script | Metodologia | Scopo Principale | Output Generato |
|---|---|---|---|
| `tune_ucb1.py` | **Best Arm Identification** (*Successive Rejects*, Audibert et al.) | Ottimizzazione congiunta $(\alpha, \text{cutoff})$ per policy UCB1 | `training_results/bai_ucb1_results.json` |
| `tune_puct.py` | **Best Arm Identification** (*Successive Rejects*) | Ottimizzazione congiunta $(C, \text{cutoff})$ per policy PUCT | `training_results/bai_puct_results.json` |
| `showdown.py` | **Torneo Head-to-Head** (Colori alternati) | Scontro diretto tra Campione UCB1 e Campione PUCT | `training_results/showdown_results.json` |
| `validate_out_of_sample.py` | **Test Statistico Out-of-Sample** (200 match) | Calcolo $Z$-score, $p$-value e intervallo di confidenza al 95% (Wilson Score) | `training_results/validation_oos_results.json` |
| `generate_academic_report.py` | **Matplotlib Academic Rendering** | Generazione figure vettoriali ad alta risoluzione (PDF + PNG a 300 DPI) per Beamer e tesi | `training_results/` |

---

## 🚀 Come Eseguire gli Script

Puoi eseguire gli script direttamente dalla root del progetto tramite i target del **Makefile**:

```bash
# Esegui il tuning UCB1
make tune-ucb1

# Esegui il tuning PUCT
make tune-puct

# Esegui lo showdown tra i due campioni
make showdown

# Esegui la validazione statistica out-of-sample (200 match)
make validate-oos

# Rigenera tutti i grafici accademici
make report
```

Oppure manualmente con Python (gli script possono essere lanciati sia dalla root che dall'interno della cartella `training/`):

```bash
python3 training/tune_ucb1.py
python3 training/validate_out_of_sample.py
python3 training/generate_academic_report.py
```
