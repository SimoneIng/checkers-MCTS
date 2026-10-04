#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "checkers/bitboard.h"
#include "checkers/move_generator.h"

/**
 * @brief Stati terminali e non della partita.
 */
typedef enum {
    ONGOING = 0,
    WHITE_WINS = 1,
    BLACK_WINS = 2,
    DRAW = 3
} GameStatus;

/**
 * @brief Verifica se il gioco è terminato e restituisce lo stato.
 * @param board La scacchiera corrente.
 * @param is_white_turn 1 se è il turno del bianco, 0 altrimenti.
 * @param half_moves (Ply) senza catture o mosse di pedina.
 * @return GameStatus lo stato della partita.
 */
GameStatus check_game_status(Bitboard* board, int is_white_turn, int half_moves);

/**
 * @brief Verifica casi noti di finale che portano inevitabilmente alla patta.
 * Usato soprattutto per tagliare i rollout randomici (come da requisiti di progetto).
 * @param board La scacchiera corrente.
 * @return 1 se è patta euristica, 0 altrimenti.
 */
int is_heuristic_draw(const Bitboard* board);

#endif // GAME_STATE_H
