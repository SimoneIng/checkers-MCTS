#ifndef MCTS_H
#define MCTS_H

#include "checkers/bitboard.h"
#include "checkers/move_generator.h"
#include <stdint.h>
#include <stdlib.h>

#define NULL_NODE 0xFFFFFFFF // Costante per rappresentare un indice nullo

/**
 * @brief Struttura ottimizzata di un nodo MCTS.
 * L'obiettivo è mantenere questa struttura sotto i 64 byte (cache line size).
 * Usa indici a 32 bit (invece di puntatori a 64 bit) per referenziare gli altri nodi.
 * Modello ad albero: "First Child / Next Sibling" per allocare dinamicamente i figli senza array fissi.
 */
typedef struct {
    uint32_t visits;          // N: Numero di volte che il nodo è stato visitato
    float score;              // W: Punteggio cumulato (vittorie + patte). Usiamo float per frazioni (es. patta = 0.5)
    
    uint32_t parent;          // Indice del nodo padre nel pool
    uint32_t first_child;     // Indice del primo figlio nel pool
    uint32_t next_sibling;    // Indice del fratello successivo nel pool
    
    Move move;                // (12 byte) La mossa eseguita per arrivare a questo stato
    
    uint8_t is_terminal;      // 1 se lo stato è una fine partita (vittoria/patta), 0 altrimenti
    uint8_t is_fully_expanded;// 1 se tutti i figli legali sono stati istanziati
    uint16_t padding;         // Allineamento memoria
} MCTSNode;

// Dimensione totale stimata: 4+4+4+4+4+12+1+1+2 = 36 byte per nodo. 
// Entra comodamente in una singola Cache Line della CPU (tipicamente 64 byte).

/**
 * @brief Struttura globale per il Memory Pool dei nodi.
 */
typedef struct {
    MCTSNode* nodes;          // L'array contiguo pre-allocato
    uint32_t count;           // Quanti nodi sono attualmente allocati
    uint32_t capacity;        // Capacità massima del pool
} NodePool;

// Espone il pool a livello globale (dichiarato in mcts.c)
extern NodePool g_node_pool;

/**
 * @brief Inizializza il memory pool con la capacità specificata.
 * Da chiamare una sola volta all'avvio del programma.
 */
void mcts_pool_init(uint32_t capacity);

/**
 * @brief Dealloca l'intero pool liberando la memoria.
 */
void mcts_pool_free();

/**
 * @brief Svuota l'albero per riutilizzare la memoria (es. tra una mossa e l'altra o per una nuova partita)
 * senza invocare free/malloc. Azzera semplicemente il counter.
 */
void mcts_pool_reset();

/**
 * @brief Alloca (logicamente) un nuovo nodo dal pool in tempo O(1).
 * @param parent_idx Indice del padre.
 * @param move La mossa che ha generato questo nodo.
 * @return L'indice del nuovo nodo, o NULL_NODE se il pool è pieno.
 */
uint32_t mcts_alloc_node(uint32_t parent_idx, const Move* move);

/**
 * @brief Restituisce il puntatore alla struttura MCTSNode dato il suo indice.
 */
static inline MCTSNode* mcts_get_node(uint32_t index) {
    if (index == NULL_NODE || index >= g_node_pool.count) return NULL;
    return &g_node_pool.nodes[index];
}

#include "mcts/selection_policies.h"

/* ========================================================================= *
 * Fasi Algoritmo MCTS
 * ========================================================================= */

/**
 * @brief Fase 1: SELECTION (Percorre l'albero usando la policy indicata).
 * Aggiorna il puntatore current_board scendendo nell'albero tramite make_move.
 * @param root_idx L'indice del nodo radice.
 * @param current_board Puntatore alla scacchiera da aggiornare.
 * @param current_player Puntatore al giocatore corrente da aggiornare.
 * @param policy Quale formula usare (UCB1, PUCT).
 * @param exploration_constant La costante 'c' di esplorazione.
 * @return L'indice del nodo foglia selezionato.
 */
uint32_t mcts_select(uint32_t root_idx, Bitboard* current_board, int* current_player, SelectionPolicy policy, float exploration_constant);

/**
 * @brief Fase 2: EXPANSION.
 * Dato un nodo foglia, ne istanzia tutti i figli legali nel Memory Pool.
 */
void mcts_expand(uint32_t node_idx, Bitboard* board, int current_player);

/**
 * @brief Fase 3: SIMULATION (Rollout).
 * Esegue una partita randomica partendo dalla scacchiera corrente.
 * @return Il reward dal punto di vista del BIANCO (1.0 = vittoria B, 0.0 = vittoria N, 0.5 = patta).
 */
float mcts_simulate(Bitboard* board, int current_player);

/**
 * @brief Fase 4: BACKPROPAGATION.
 * Risale l'albero aggiornando visite (N) e punteggio (W) per ogni nodo antenato.
 */
void mcts_backpropagate(uint32_t node_idx, float reward_for_white);

#endif // MCTS_H
