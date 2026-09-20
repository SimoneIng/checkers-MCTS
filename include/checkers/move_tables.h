#ifndef MOVE_TABLES_H
#define MOVE_TABLES_H

#include <stdint.h>

/**
 * Indici per le direzioni di movimento.
 * - Le pedine bianche muovono verso Nord (NW, NE)
 * - Le pedine nere muovono verso Sud (SW, SE)
 * - Le dame muovono in tutte le direzioni
 */
typedef enum {
    DIR_NW = 0,
    DIR_NE = 1,
    DIR_SW = 2,
    DIR_SE = 3
} Direction;

/* Array di look-up pre-calcolati per gli spostamenti (1 passo) */
/* MOVES[square][dir] contiene la bitmask (1 << destinazione) se valido, altrimenti 0 */
extern uint32_t MOVES[32][4];

/* Array di look-up pre-calcolati per le catture (salti di 2 passi) */
/* JUMPS[square][dir] contiene la bitmask della destinazione finale, altrimenti 0 */
extern uint32_t JUMPS[32][4];

/* Array di look-up per la bitmask del pezzo scavalcato durante una cattura */
/* CAPTURES[square][dir] contiene la maschera della casella intermedia */
extern uint32_t CAPTURES[32][4];

/**
 * @brief Inizializza tutte le Look-up Tables per spostamenti e catture.
 * Questa funzione deve essere chiamata una sola volta all'avvio dell'applicazione.
 */
void init_move_tables(void);

#endif // MOVE_TABLES_H
