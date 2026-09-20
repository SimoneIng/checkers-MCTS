# Moves Implementation Plan: Gestione delle Look-Up Tables

## L'idea alla base

Uno dei colli di bottiglia principali quando si sviluppa un motore di gioco basato su MCTS (Monte Carlo Tree Search) è la *Move Generation*. Dato che l'algoritmo deve simulare milioni di partite al secondo durante la fase di rollout, calcolare dinamicamente le regole fisiche (i bordi della scacchiera, le direzioni, le celle di destinazione) ad ogni singola mossa è impensabile.

Per questo motivo, ho deciso di pre-calcolare tutto all'avvio dell'applicazione. Tutte le possibili mosse per ogni singola casella sono salvate in tre matrici bidimensionali (Look-Up Tables) di dimensioni `[32][4]`:

- `MOVES[32][4]`: le maschere di bit (bitmasks) che indicano le caselle di arrivo per gli spostamenti semplici (di un solo passo).
- `JUMPS[32][4]`: le destinazioni finali dei salti (catture).
- `CAPTURES[32][4]`: le posizioni dei pezzi scavalcati, che andranno rimosse dalla board.

Le righe (0-31) indicano la casella da cui parte il pezzo, mentre le colonne (0-3) indicano la direzione. Per comodità, ho mappato le direzioni con un'enum: `0` per Nord-Ovest, `1` per Nord-Est, `2` per Sud-Ovest e `3` per Sud-Est.

---

## Perché usare tre matrici [32][4] invece di dodici array 1D?

Inizialmente potrebbe sembrare intuitivo creare array separati per ogni direzione (del tipo `MOVE_NW[32]`, `MOVE_NE[32]`, ecc., per un totale di 12 array diversi). Tuttavia, ragionando sull'architettura del sistema e su come il codice verrà poi eseguito dalla CPU, l'approccio con matrici bidimensionali `[32][4]` offre vantaggi netti in fase di esecuzione.

### 1. Spatial Locality e sfruttamento della Cache
Questo è forse l'aspetto più importante. In C, una matrice bidimensionale viene memorizzata "per righe" (Row-Major order). Questo significa che i dati relativi alle quattro direzioni di una determinata casella (es. `MOVES[sq][0]`, `MOVES[sq][1]`, ecc.) sono fisicamente adiacenti in memoria RAM, andando a occupare un piccolo blocco di 16 byte consecutivi.

Durante i rollout dell'MCTS, quando il motore valuta un pezzo su una casella, ha bisogno di esplorare quasi tutte le direzioni possibili. Nel momento in cui la CPU legge il valore della prima direzione, il memory controller porterà l'intera *cache line* (di solito da 64 byte) direttamente nella cache L1 del processore. Questo ci regala l'accesso "gratis" anche alle altre direzioni. 
Se avessi optato per array separati, i dati per le direzioni diverse sarebbero stati sparpagliati in zone distanti della memoria, scatenando continui *cache miss* e costringendo la CPU a sprecare cicli preziosi in attesa del recupero dati dalla RAM.

### 2. Codice più pulito (e ottimizzazione del branching)
Avere un array strutturato su 4 colonne mi permette di iterare sulle direzioni con un semplicissimo ciclo `for`:

```c
for (int dir = 0; dir < 4; dir++) {
    uint32_t move_mask = MOVES[square][dir];
    if (move_mask) {
        // Logica per validare e applicare la mossa
    }
}
```

Senza questa struttura, mi sarei ritrovato a scrivere e manutenere quattro blocchi di codice copiati e incollati per ogni direzione.
Inoltre, questo design si incastra perfettamente con le regole della dama italiana (dove le pedine normali muovono solo in avanti). Posso semplicemente modulare gli indici del ciclo `for`: per le pedine bianche itero solo su `0` e `1` (verso Nord), per quelle nere su `2` e `3` (verso Sud), mentre per le dame faccio il ciclo completo da `0` a `3`. È un approccio molto più elegante rispetto a infilare degli `if` condizionali specifici in mezzo alla generazione delle mosse, evitando di penalizzare la *branch prediction* della CPU.

In definitiva, l'occupazione totale in memoria è esattamente la stessa in entrambi gli approcci (circa 1.5 KB complessivi), ma la struttura `[32][4]` risulta decisamente più *cache-friendly* e rende l'implementazione del move generator estremamente più snella e accademica.

---

## Assenza delle maschere per i bordi: perché non servono?

In molti motori scacchistici basati su bitboard, si fa un uso massiccio di maschere logiche per i bordi (es. `LEFT_EDGE_MASK`, `RIGHT_EDGE_MASK`). Queste maschere servono a impedire l'effetto "pac-man": quando si trasla (*shift*) l'intero bitboard per calcolare le mosse di tutti i pezzi in parallelo (approccio *board-centric*), una pedina sul bordo sinistro potrebbe riapparire sul bordo destro della riga adiacente a causa dello shift aritmetico. In quel contesto, mascherare i bordi prima dello shift è un obbligo logico.

Tuttavia, avendo optato per un approccio basato interamente su un set di matrici pre-calcolate per la singola casella (*piece-centric*), l'uso di maschere per i bordi risulta del tutto **ridondante e controproducente**.

L'intera logica geometrica dei confini della scacchiera viene infatti risolta *a monte*, durante l'unica esecuzione della funzione `init_move_tables()` all'avvio. 
Quando, durante la partita, il motore interroga la matrice per sapere se una pedina sul bordo sinistro può muoversi a Nord-Ovest, la LUT restituisce direttamente `0` (mossa illegale). Di conseguenza, la successiva operazione bit a bit (`MOVES[sq][dir] & empty_squares`) restituirà rigorosamente `0`, scartando la mossa in modo naturale.

Introdurre ulteriori calcoli con operatori AND e maschere dei bordi all'interno del generatore di mosse significherebbe soltanto sprecare preziosi cicli di clock della CPU per risolvere un problema geometrico che la nostra architettura ha già neutralizzato alla radice.
