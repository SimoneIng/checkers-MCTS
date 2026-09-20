#include "checkers/move_tables.h"

uint32_t MOVES[32][4];
uint32_t JUMPS[32][4];
uint32_t CAPTURES[32][4];

/**
 * @brief Calcola l'indice della casella adiacente in una certa direzione.
 * 
 * @param sq Indice della casella di partenza (0-31)
 * @param d_row Variazione di riga (+1 Nord, -1 Sud)
 * @param d_col Variazione di colonna (+1 Est, -1 Ovest)
 * @return int L'indice 0-31 della nuova casella, o -1 se esce dalla scacchiera.
 */
static int get_adjacent_square(int sq, int d_row, int d_col) {
    // Ricostruiamo riga e colonna della scacchiera 8x8 (0-7)
    int row = sq / 4;
    
    // Nelle righe pari (0,2,4,6) le caselle scure sono alle colonne pari (0,2,4,6).
    // Nelle righe dispari (1,3,5,7) le caselle scure sono alle colonne dispari (1,3,5,7).
    int col = (sq % 4) * 2;
    if (row % 2 != 0) {
        col += 1;
    }

    int new_row = row + d_row;
    int new_col = col + d_col;

    // Controllo limiti della scacchiera
    if (new_row < 0 || new_row > 7 || new_col < 0 || new_col > 7) {
        return -1;
    }

    // Ricava l'indice 0-31 della nuova casella e lo restituisce
    return (new_row * 4) + (new_col / 2);
}

void init_move_tables(void) {
    for (int i = 0; i < 32; i++) {
        // --- Spostamenti semplici (1 passo) ---
        int adj[4];
        adj[DIR_NW] = get_adjacent_square(i, 1, -1);
        adj[DIR_NE] = get_adjacent_square(i, 1, 1);
        adj[DIR_SW] = get_adjacent_square(i, -1, -1);
        adj[DIR_SE] = get_adjacent_square(i, -1, 1);

        // Mapping degli step riga/colonna in base all'enum Direction
        int d_rows[4] = {1, 1, -1, -1};
        int d_cols[4] = {-1, 1, -1, 1};

        for (int dir = 0; dir < 4; dir++) {
            int target = adj[dir];
            
            // Popoliamo array di spostamento
            MOVES[i][dir] = (target != -1) ? (1U << target) : 0;

            // --- Catture / Salti (2 passi) ---
            // Un salto è valido solo se *entrambe* le caselle (intermedia e destinazione) sono dentro la scacchiera.
            if (target != -1) {
                int jump_target = get_adjacent_square(target, d_rows[dir], d_cols[dir]);
                if (jump_target != -1) {
                    JUMPS[i][dir] = (1U << jump_target);
                    CAPTURES[i][dir] = (1U << target);
                } else {
                    JUMPS[i][dir] = 0;
                    CAPTURES[i][dir] = 0;
                }
            } else {
                JUMPS[i][dir] = 0;
                CAPTURES[i][dir] = 0;
            }
        }
    }
}
