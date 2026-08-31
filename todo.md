# TODO: Progetto Dama Italiana con MCTS

## Fase 0: Setup e Rappresentazione (Completata)
- [x] Analisi dei vincoli del progetto (C, MCTS, velocità, memoria).
- [x] Scelta della rappresentazione: 3 Bitboard a 32-bit (`uint32_t`) per le 32 caselle giocabili.
- [x] Stesura del documento sulle motivazioni architetturali (`docs/bitboard_representation.md`).
- [x] Implementazione base (`bitboard.h` e `bitboard.c`) e inizializzazione.
- [x] Stampa della scacchiera a terminale.
- [x] Organizzazione cartelle (`src/checkers/`, `include/checkers/`) e creazione del `Makefile` ricorsivo.

## Fase 1: Logica di Gioco e Regole della Dama Italiana (Move Generation)
- [ ] Creazione delle Look-up Tables (LUTs) pre-calcolate per spostamenti e catture.
- [ ] Sviluppo del generatore di mosse legali (Move Generator) operando a bit.
- [ ] Implementazione dei vincoli e priorità di cattura (obbligo di presa, presa massima, presa con/di maggior valore).
- [ ] Implementazione funzione `Make Move` (aggiornamenti tramite XOR e promozione a Dama).
- [ ] Implementazione funzione `Unmake Move` per annullare la mossa ripristinando lo stato (senza clonare nodi).

## Fase 2: Identificazione degli Stati Terminali
- [ ] Condizioni di Vittoria/Sconfitta (giocatore senza pezzi o bloccato, stallo).
- [ ] Rilevamento della Patta (stallo su mosse ripetute, regola delle 40 mosse senza cattura/avanzamento).

## Fase 3: Motore Monte Carlo Tree Search (MCTS)
- [ ] Progettazione della struttura del Nodo MCTS (punteggio `W`, visite `N`, array/lista figli).
- [ ] Sviluppo di un **Memory Pool** pre-allocato per minimizzare i tempi di `malloc/free` durante le allocazioni dei nodi.
- [ ] Fase 1: **Selection** (Implementazione formula UCB1 e sue varianti es. PUCT).
- [ ] Fase 2: **Expansion** (Istanziazione e collegamento nuovi figli).
- [ ] Fase 3: **Simulation / Rollout** (Svolgimento della partita fino a fine gioco).
- [ ] Fase 4: **Backpropagation** (Aggiornamento dei rami percorsi).

## Fase 4: Ottimizzazioni per il Rollout e Tempi (Anytime)
- [ ] Applicazione di euristiche leggere per tagliare (cut-off) i Rollout inconcludenti e guidare le fasi finali fisse (es. Dama vs Dama = Patta certa).
- [ ] Implementazione algoritmo *Anytime* (gestione limite di tempo fisso 0.2s, 1s, 3s senza budget di iterazioni).

## Fase 5: Interfaccia Grafica e Input Giocatore
- [ ] Scelta e setup della libreria grafica in C (SDL2, Raylib o equivalenti).
- [ ] Rendering della finestra, della scacchiera e delle pedine tramite assets/forme.
- [ ] Gestione degli input utente (click/drag del mouse per gli spostamenti umani).
- [ ] Integrazione del ciclo di eventi UI con il motore logico dell'AP in background.

## Fase 6: Tuning degli Iperparametri e Analisi Sperimentale
- [ ] Automazione dei match (Tornei Round-robin o Algoritmi Genetici) tra AP con valori di costanti di esplorazione differenti.
- [ ] Estrazione dei dati di esecuzione (es. Win Rate per parametro, nodi/sec espansi).
- [ ] Stesura del documento finale (Analisi Sperimentale delle configurazioni).
