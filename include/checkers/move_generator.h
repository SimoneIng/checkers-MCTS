#ifndef MOVE_GENERATOR_H
#define MOVE_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "checkers/bitboard.h"
#include "checkers/move_tables.h"

/**
 * @brief Struttura ottimizzata da 12 byte per rappresentare una mossa
 * 
 * Questa struttura è disegnata specificamente per gestire le catture multiple
 * della Dama Italiana e per permettere una funzione UnmakeMove velocissima.
 */
typedef struct {
    uint8_t from;              // Indice casella di partenza (0-31)
    uint8_t to;                // Indice casella di arrivo finale (0-31)
    uint8_t is_promotion;      // 1 se la mossa causa promozione a Dama, 0 altrimenti
    uint8_t path_score;        // Usato per calcolare la precedenza (es. mangiare Dama il prima possibile)
    
    uint32_t captured_men;     // Maschera bit delle PEDINE avversarie mangiate (0 se nessuna)
    uint32_t captured_kings;   // Maschera bit delle DAME avversarie mangiate (0 se nessuna)
} Move;

#define MAX_MOVES 64

/**
 * @brief Struttura allocata staticamente per contenere le mosse di un turno (niente malloc)
 */
typedef struct {
    Move moves[MAX_MOVES];
    int count;
} MoveList;

/**
 * @brief Genera tutte le mosse legali per il giocatore corrente applicando il filtro della Dama Italiana.
 */
void generate_moves(Bitboard* board, int is_white, MoveList* list);

/**
 * @brief Applica una mossa alla scacchiera (Make) o la annulla (Unmake).
 * Dato che usano l'operatore XOR, la logica è identica e simmetrica al 100%.
 */
void apply_move_xor(Bitboard* board, const Move* move, int is_white);
#define make_move apply_move_xor
#define unmake_move apply_move_xor

#endif // MOVE_GENERATOR_H
