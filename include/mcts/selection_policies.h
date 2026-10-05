#ifndef SELECTION_POLICIES_H
#define SELECTION_POLICIES_H

#include <math.h>

// Forward declaration per evitare loop di include, se necessario
// (In questo caso includiamo l'header dei nodi per leggere visite/score)
// Ma siccome selection_policies.h verrà incluso DA mcts.h, passiamo direttamente
// i valori numerici alle funzioni per massimizzare la modularità, 
// o dichiariamo qui solo l'enum e le macro.

/**
 * @brief Politiche di Selezione supportate.
 */
typedef enum {
    POLICY_UCB1 = 0,
    POLICY_PUCT = 1
} SelectionPolicy;

/**
 * @brief Calcola lo score UCB1 classico per un nodo figlio.
 * 
 * @param parent_visits Visite totali del nodo padre (N)
 * @param child_visits Visite del nodo figlio (n)
 * @param child_score Punteggio cumulato del nodo figlio dal punto di vista del Bianco (W)
 * @param current_player Giocatore di turno al nodo padre (1=Bianco, 0=Nero)
 * @param c Costante di esplorazione
 */
static inline float compute_ucb1(uint32_t parent_visits, uint32_t child_visits, float child_score, int current_player, float c) {
    float exploitation = child_score / child_visits;
    if (current_player == 0) {
        exploitation = 1.0f - exploitation; // Minimizza per il Nero
    }
    float exploration = sqrtf(logf((float)parent_visits) / (float)child_visits);
    return exploitation + c * exploration;
}

/**
 * @brief Calcola lo score PUCT (Predictor Upper Confidence Bound for Trees).
 */
static inline float compute_puct(uint32_t parent_visits, uint32_t child_visits, float child_score, int current_player, float c) {
    float exploitation = child_score / child_visits;
    if (current_player == 0) {
        exploitation = 1.0f - exploitation; 
    }
    // PUCT base: c * P(s,a) * sqrt(N) / (1 + n)
    // P(s,a) uniforme = 1.0
    float exploration = (sqrtf((float)parent_visits) / (1.0f + (float)child_visits));
    return exploitation + c * exploration;
}

#endif // SELECTION_POLICIES_H
