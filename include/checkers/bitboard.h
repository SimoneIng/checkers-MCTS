#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Struttura che rappresenta lo stato della scacchiera della Dama Italiana.
 *
 * Utilizza 3 bitboard da 32 bit per mappare le 32 caselle scure giocabili.
 */
typedef struct {
    uint32_t white_pieces; // Maschera dei pezzi bianchi (pedine e dame)
    uint32_t black_pieces; // Maschera dei pezzi neri (pedine e dame)
    uint32_t kings;        // Maschera delle dame (di qualsiasi colore)
} Bitboard;

/**
 * @brief Colori dei giocatori
 */
typedef enum {
    WHITE = 0,
    BLACK = 1
} Color;

/**
 * @brief Inizializza la scacchiera con la configurazione di partenza della Dama Italiana.
 * @param board Puntatore alla struttura Bitboard da inizializzare.
 */
void bitboard_init(Bitboard *board);

/**
 * @brief Stampa la scacchiera a terminale per scopi di debug.
 * @param board Puntatore alla struttura Bitboard da stampare.
 */
void bitboard_print(const Bitboard *board);

/* ========================================================================= *
 * Funzioni inline per controlli veloci sullo stato della bitboard           *
 * ========================================================================= */

/**
 * @brief Ottiene la bitboard delle sole pedine bianche.
 */
static inline uint32_t get_white_men(const Bitboard *board) {
    return board->white_pieces & ~board->kings;
}

/**
 * @brief Ottiene la bitboard delle sole dame bianche.
 */
static inline uint32_t get_white_kings(const Bitboard *board) {
    return board->white_pieces & board->kings;
}

/**
 * @brief Ottiene la bitboard delle sole pedine nere.
 */
static inline uint32_t get_black_men(const Bitboard *board) {
    return board->black_pieces & ~board->kings;
}

/**
 * @brief Ottiene la bitboard delle sole dame nere.
 */
static inline uint32_t get_black_kings(const Bitboard *board) {
    return board->black_pieces & board->kings;
}

/**
 * @brief Ottiene la bitboard delle caselle libere (non occupate).
 */
static inline uint32_t get_empty_squares(const Bitboard *board) {
    return ~(board->white_pieces | board->black_pieces);
}

#endif // BITBOARD_H
