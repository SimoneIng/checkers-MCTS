#include "checkers/bitboard.h"
#include <stdio.h>

int main() {
    Bitboard board;
    
    printf("Inizializzazione della bitboard in corso...\n");
    
    // Inizializza lo stato della scacchiera
    bitboard_init(&board);
    
    printf("Scacchiera inizializzata con successo.\n");
    
    // Stampa la scacchiera a terminale
    bitboard_print(&board);
    
    return 0;
}
