#include "checkers/bitboard.h"
#include <stdio.h>

/*
 * Mappatura della scacchiera a 32 caselle giocabili:
 * La dama italiana gioca sulle caselle scure.
 * Orientamento: in basso a sinistra c'è una casella scura (casella 0).
 * In basso a destra c'è una casella chiara.
 *
 * Mappatura bit (da 0 a 31):
 * R7:    28    29    30    31
 * R6: 24    25    26    27
 * R5:    20    21    22    23
 * R4: 16    17    18    19
 * R3:    12    13    14    15
 * R2: 08    09    10    11
 * R1:    04    05    06    07
 * R0: 00    01    02    03
 *
 * Il giocatore Bianco occupa le prime 3 righe (bit 0-11).
 * Il giocatore Nero occupa le ultime 3 righe (bit 20-31).
 */

#define INITIAL_WHITE 0x00000FFF // 12 bit meno significativi (caselle 0-11)
#define INITIAL_BLACK 0xFFF00000 // 12 bit più significativi (caselle 20-31)

void bitboard_init(Bitboard *board) {
    if (!board) return;
    
    board->white_pieces = INITIAL_WHITE;
    board->black_pieces = INITIAL_BLACK;
    board->kings = 0; // Nessuna dama all'inizio
}

void bitboard_print(const Bitboard *board) {
    if (!board) return;

    printf("\nScacchiera Dama Italiana:\n");
    printf("  +-----------------+\n");
    
    // Stampiamo partendo dalla riga in alto (7) fino in basso (0)
    for (int row = 7; row >= 0; row--) {
        printf("%d | ", row);
        
        for (int col = 0; col < 8; col++) {
            // Nella dama italiana la casella in basso a sinistra (riga 0, col 0) è scura.
            // Pertanto, se la somma di riga e colonna è pari, la casella è scura (giocabile).
            if ((row + col) % 2 == 0) {
                // Calcola l'indice della bitboard
                int bit_index = (row * 4) + (col / 2);
                uint32_t mask = 1U << bit_index;
                
                if ((board->white_pieces & mask) && (board->kings & mask)) {
                    printf("W "); // Dama bianca
                } else if (board->white_pieces & mask) {
                    printf("w "); // Pedina bianca
                } else if ((board->black_pieces & mask) && (board->kings & mask)) {
                    printf("B "); // Dama nera
                } else if (board->black_pieces & mask) {
                    printf("b "); // Pedina nera
                } else {
                    printf(". "); // Vuota giocabile
                }
            } else {
                printf("  "); // Casella chiara non giocabile
            }
        }
        printf("|\n");
    }
    
    printf("  +-----------------+\n");
    printf("    a b c d e f g h\n\n");
}
