# Scelta della Rappresentazione a Bitboard per la Dama Italiana

## 1. Contesto e Vincoli del Progetto
In base alle specifiche definite nel file `progettoIW.pdf`, la realizzazione del giocatore artificiale (AP) per la Dama Italiana tramite Monte Carlo Tree Search (MCTS) impone requisiti stringenti sulle performance e sull'occupazione di memoria:
* **Linguaggio C**: Il sistema deve essere interamente sviluppato in C. Nessun altro linguaggio è accettato.
* **Velocità dei Rollout e Gestione Memoria**: Il successo dell'algoritmo MCTS dipende direttamente dal numero di simulazioni (rollout) che si riescono ad eseguire nell'unità di tempo e da un'oculata gestione della memoria.
* **Uso di Bitboard e Operazioni Bitwise**: È esplicitamente richiesto l'uso di "bitboard con maschere di bit che consentano operazioni bitwise veloci".
* **Look-up Tables**: È suggerito l'uso di look-up tables per velocizzare e individuare facilmente le mosse possibili.
* **Divieto di clonazione degli stati**: Durante il rollout non bisogna clonare gli stati, richiedendo quindi operazioni rapide in-place.

## 2. Struttura della Scacchiera e Modello Proposto
La scacchiera della Dama Italiana è composta da 64 caselle (8x8), di cui solo 32 caselle (quelle scure) sono giocabili e possono ospitare delle pedine. Poiché 32 è esattamente il numero di bit di un intero standard a 32 bit, **la scelta ottimale è rappresentare la scacchiera utilizzando tipi `uint32_t` (interi senza segno a 32 bit)** messi a disposizione dallo standard C (`<stdint.h>`).

Il modo migliore e più compatto per mappare l'intero stato del gioco, minimizzando la ridondanza, consiste nell'utilizzare **tre bitboard da 32 bit**:

1. `uint32_t white_pieces`: un bit a 1 indica la presenza di un pezzo bianco (pedina o dama) in quella casella.
2. `uint32_t black_pieces`: un bit a 1 indica la presenza di un pezzo nero (pedina o dama) in quella casella.
3. `uint32_t kings`: un bit a 1 indica la presenza di una dama (indipendentemente dal colore, che si deduce dalle prime due).

A partire da questi 3 interi, è possibile ricavare istantaneamente qualsiasi informazione sullo stato del gioco tramite semplici operazioni logiche bit a bit:
* **Pedine bianche**: `white_pieces & ~kings`
* **Dame bianche**: `white_pieces & kings`
* **Pedine nere**: `black_pieces & ~kings`
* **Dame nere**: `black_pieces & kings`
* **Caselle vuote**: `~(white_pieces | black_pieces)`

## 3. Motivazioni e Vantaggi della Scelta

Questa architettura risponde perfettamente e in modo ottimale ai vincoli del progetto per i seguenti motivi:

### Efficienza di Memoria (Ottimizzazione MCTS)
L'intero stato della scacchiera richiede solamente 3 variabili `uint32_t`, per un totale di appena **12 byte** per rappresentare i pezzi. Aggiungendo pochi bit per tenere traccia del turno corrente o di regole specifiche (mosse senza catture per la regola della patta), il nodo MCTS manterrà una dimensione minima. Ciò permette di memorizzare un numero enormemente maggiore di nodi nella cache della CPU e nella RAM, accelerando le fasi di *Selection* ed *Expansion* e riducendo il garbage collection e i cache miss.

### Massima Velocità nei Rollout (Operazioni Bitwise)
Nei moderni processori a 64 bit e 32 bit, le operazioni bit a bit (AND, OR, XOR, SHIFT, NOT) su tipi interi nativi vengono eseguite in **singole istruzioni CPU**, richiedendo solitamente un solo ciclo di clock. Utilizzando routine intrinseche del compilatore (es. `__builtin_ctz()` in GCC) per trovare la posizione dei bit a 1, si evitano completamente i lenti cicli `for` necessari invece su matrici bidimensionali (es. `int board[8][8]`).

### Integrazione con le Look-up Tables
Come suggerito dai requisiti ("*Usare nel caso look-up table per individuare le mosse*"), con la scacchiera mappata su indici da 0 a 31, è possibile pre-calcolare gli spostamenti. Si possono costruire costanti in memoria all'avvio dell'applicazione come:
* Tabelle delle mosse valide per ogni casella (spostamenti Nord-Ovest, Nord-Est, ecc.).
* Tabelle per i salti delle catture (casella di partenza, casella saltata, casella di arrivo).

In fase di rollout, determinare se una mossa è valida si traduce in un rapido accesso all'array (look-up) combinato con un AND bit a bit (`&`) sulla bitboard delle *caselle vuote*, rendendo la complessità della Move Generation vicina a **O(1)**.

### Make / Unmake Move e Assenza di Clonazione
Il requisito di "*non clonare gli stati durante il rollout*" diventa banale. Spostare un pezzo equivale ad applicare un operatore **XOR (`^`)** logico per invertire lo stato delle caselle di partenza e di destinazione:
`bitboard ^= (1 << casella_partenza) | (1 << casella_arrivo);`
Anche le catture si risolvono rimuovendo (tramite XOR o `&= ~mask`) il pezzo mangiato. Poiché le operazioni bitwise sono invertibili, per annullare la mossa (Unmake) e tornare al nodo padre dopo un rollout, è sufficiente riapplicare la stessa operazione logica. Questo paradigma, tipico dei motori scacchistici, permette di simulare un'intera partita modificando sul posto lo stato corrente, senza alcuna allocazione di memoria aggiuntiva (malloc/free).
