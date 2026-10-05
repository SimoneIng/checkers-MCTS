#ifndef MCTS_ANYTIME_H
#define MCTS_ANYTIME_H

#include "checkers/bitboard.h"
#include "checkers/move_generator.h"
#include "mcts/mcts.h"

/**
 * @brief Esegue l'algoritmo MCTS per un tempo specificato (Anytime Algorithm).
 * 
 * @param root_board La scacchiera da cui partire (stato attuale del gioco).
 * @param current_player Il giocatore che deve muovere (1 = Bianco, 0 = Nero).
 * @param time_limit_sec Tempo limite in secondi (es. 0.2, 1.0, 3.0 richiesti dal progetto).
 * @param policy Politica di esplorazione da utilizzare (UCB1 o PUCT).
 * @param exploration_constant La costante 'c' di esplorazione (es. 1.414).
 * @param out_best_move Puntatore dove salvare la mossa migliore trovata (parametro di output).
 * @return Numero di iterazioni (rollouts completi) eseguiti nel tempo a disposizione.
 */
uint32_t mcts_get_best_move_anytime(
    const Bitboard* root_board, 
    int current_player, 
    double time_limit_sec, 
    SelectionPolicy policy, 
    float exploration_constant,
    Move* out_best_move
);

#endif // MCTS_ANYTIME_H
