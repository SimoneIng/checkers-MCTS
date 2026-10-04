#include "checkers/bitboard.h"
#include "checkers/move_tables.h"
#include "checkers/move_generator.h"
#include "checkers/game_state.h"
#include <stdio.h>

void print_status(GameStatus status) {
    switch(status) {
        case ONGOING: printf("Stato: Partita in corso...\n"); break;
        case WHITE_WINS: printf("Stato: VITTORIA BIANCO! (Il Nero non ha mosse o pezzi)\n"); break;
        case BLACK_WINS: printf("Stato: VITTORIA NERO! (Il Bianco è senza pezzi o BLOCCATO in stallo)\n"); break;
        case DRAW: printf("Stato: PATTA (Stallo euristico / 40 Mosse)!\n"); break;
    }
}

int main() {
    Bitboard board;
    
    printf("Inizializzazione delle tabelle di lookup...\n");
    init_move_tables();

    // Creiamo uno scenario di stallo (blocco totale).
    // Il Bianco ha una sola pedina intrappolata nell'angolo (casella 0).
    // Il Nero ha pedine in 4, 8, 9, 10.
    // Tocca al Nero, che muovendo chiuderà l'unica via di fuga del Bianco.
    board.white_pieces = (1U << 0);
    board.black_pieces = (1U << 4) | (1U << 8) | (1U << 9) | (1U << 10);
    board.kings = 0;

    printf("\n--- SCACCHIERA INIZIALE ---\n");
    bitboard_print(&board);
    
    // Turno del Nero: muove la pedina da 10 a 5.
    // Questo blocca la casella 5. E il salto (da 0 oltre 5 fino a 9) è bloccato dalla pedina nera in 9.
    printf("\n[Turno del Nero] Mossa: pedina da 10 a 5\n");
    Move m = { .from = 10, .to = 5, .is_promotion = 0, .path_score = 0, .captured_men = 0, .captured_kings = 0 };
    make_move(&board, &m, 0); // 0 indica che muove il Nero
    
    printf("\n--- SCACCHIERA DOPO LA MOSSA ---\n");
    bitboard_print(&board);
    
    // Ora tocca al Bianco. Verifichiamo quante mosse legali ha a disposizione.
    printf("\n--- CONTROLLO FINE PARTITA (Turno del Bianco) ---\n");
    MoveList list;
    generate_moves(&board, 1, &list);
    printf("Mosse legali trovate per il Bianco: %d\n", list.count);
    
    // Chiamiamo il modulo game_state per vedere se rileva la sconfitta per stallo
    GameStatus status = check_game_status(&board, 1, 0);
    print_status(status);

    return 0;
}
