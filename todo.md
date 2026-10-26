# TODO: Progetto Dama Italiana con MCTS

## Fase 0: Setup e Rappresentazione (Completata)
- [x] Analisi dei vincoli del progetto (C, MCTS, velocità, memoria).
- [x] Scelta della rappresentazione: 3 Bitboard a 32-bit (`uint32_t`) per le 32 caselle giocabili.
- [x] Stesura del documento sulle motivazioni architetturali (`docs/analysis/bitboard_representation.md`).
- [x] Implementazione base (`bitboard.h` e `bitboard.c`) e inizializzazione.
- [x] Stampa della scacchiera a terminale.
- [x] Organizzazione cartelle (`src/checkers/`, `include/checkers/`) e creazione del `Makefile` ricorsivo.

## Fase 1: Logica di Gioco e Regole della Dama Italiana (Move Generation)
- [x] Creazione delle Look-up Tables (LUTs) pre-calcolate per spostamenti e catture.
- [x] Sviluppo del generatore di mosse legali (Move Generator) operando a bit.
- [x] Implementazione dei vincoli e priorità di cattura (obbligo di presa, presa massima, presa con/di maggior valore).
- [x] Implementazione funzione `Make Move` (aggiornamenti tramite XOR e promozione a Dama).
- [x] Implementazione funzione `Unmake Move` per annullare la mossa ripristinando lo stato (senza clonare nodi).

## Fase 2: Identificazione degli Stati Terminali
- [x] Condizioni di Vittoria/Sconfitta (giocatore senza pezzi o bloccato, stallo).
- [x] Rilevamento della Patta (stallo su mosse ripetute, regola delle 40 mosse senza cattura/avanzamento).

## Fase 3: Motore Monte Carlo Tree Search (MCTS)
- [x] Progettazione della struttura del Nodo MCTS (punteggio `W`, visite `N`, array/lista figli).
- [x] Sviluppo di un **Memory Pool** pre-allocato per minimizzare i tempi di `malloc/free` durante le allocazioni dei nodi.
- [x] Fase 1: **Selection** (Implementazione formula UCB1 e sue varianti es. PUCT).
- [x] Fase 2: **Expansion** (Istanziazione e collegamento nuovi figli).
- [x] Fase 3: **Simulation / Rollout** (Svolgimento della partita fino a fine gioco).
- [x] Fase 4: **Backpropagation** (Aggiornamento dei rami percorsi).

## Fase 4: Ottimizzazioni per il Rollout e Tempi (Anytime)
- [x] Applicazione di euristiche leggere per tagliare (cut-off) i Rollout inconcludenti e guidare le fasi finali fisse (es. Dama vs Dama = Patta certa).
- [x] Implementazione algoritmo *Anytime* (gestione limite di tempo fisso 0.2s, 1s, 3s senza budget di iterazioni).

## Fase 5: Interfaccia Grafica e Input Giocatore (WebAssembly + React)
- [x] Scelta e setup del framework UI (WebAssembly + React + Vite).
- [x] Rendering della finestra, della scacchiera e delle pedine tramite Tailwind CSS.
- [x] Gestione degli input utente (click validati con le regole del C) e suoni.
- [x] Integrazione del ciclo di eventi UI con il motore logico dell'AP in background.

## Fase 6: Tuning degli Iperparametri e Analisi Sperimentale
- [x] Estensione del motore C con parametrizzazione del `rollout_cutoff` ed euristica di materiale.
- [x] Creazione del runner nativo C ad alte prestazioni (`build/play_match`).
- [x] Implementazione script Best Arm Identification con Successive Rejects per UCB1 (`training/tune_ucb1.py`).
- [x] Implementazione script Best Arm Identification con Successive Rejects per PUCT (`training/tune_puct.py`).
- [x] Implementazione scontro diretto tra campioni UCB1 vs PUCT (`training/showdown.py`).
- [x] Stesura documentazione teorica e metodologica (`docs/analysis/metodologia_tuning_bai.md`).
- [x] Esecuzione dei test di tuning ed estrazione dei risultati JSON.
- [x] Aggiornamento slide finale e report con i risultati sperimentali reali.
