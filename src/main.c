#include "checkers/bitboard.h"
#include "checkers/move_tables.h"
#include "checkers/move_generator.h"
#include "checkers/game_state.h"
#include "mcts/mcts_anytime.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void print_status(GameStatus status) {
    switch(status) {
        case ONGOING: printf("Stato: Partita in corso...\n"); break;
        case WHITE_WINS: printf("VITTORIA BIANCO!\n"); break;
        case BLACK_WINS: printf("VITTORIA NERO!\n"); break;
        case DRAW: printf("PATTA (Stallo euristico / 40 Mosse)!\n"); break;
    }
}

int main() {
    // Inizializza il seed randomico per i rollout
    srand((unsigned int)time(NULL));
    
    printf("Inizializzazione delle tabelle di lookup...\n");
    init_move_tables();

    // Alloca 2 Milioni di nodi per il MCTS
    mcts_pool_init(2000000);

    Bitboard board;
    bitboard_init(&board);
    
    int current_player = 1; // Inizia il Bianco (1)
    int half_moves = 0;
    int turn_number = 1;

    printf("\n--- SCACCHIERA INIZIALE ---\n");
    bitboard_print(&board);
    
    // Configurazione del bot
    double time_limit = 0.2; // mezzo secondo a mossa (dà il ritmo all'animazione)
    float exploration_c = 1.414f; 
    SelectionPolicy policy = POLICY_UCB1;

    while (1) {
        // Pulisce il terminale (sequenza ANSI standard) posizionando il cursore in alto
        printf("\033[H\033[J");
        
        GameStatus status = check_game_status(&board, current_player, half_moves);
        if (status != ONGOING) {
            printf("\n\n===== PARTITA TERMINATA =====\n");
            print_status(status);
            bitboard_print(&board);
            break;
        }
        
        printf("\n--- TURNO %d : TOCCA AL %s ---\n", turn_number, current_player == 1 ? "BIANCO" : "NERO");
        bitboard_print(&board);
        fflush(stdout); // Forza la stampa a video per evitare buffering
        
        Move best_move;
        uint32_t iterations = mcts_get_best_move_anytime(&board, current_player, time_limit, policy, exploration_c, &best_move);
        
        printf("MCTS ha eseguito %u rollout in %.1f sec.\n", iterations, time_limit);
        printf("Mossa scelta: da %d a %d (promozione: %d, catturate: %u)\n", 
               best_move.from, best_move.to, best_move.is_promotion, __builtin_popcount(best_move.captured_men | best_move.captured_kings));
        
        // Applica mossa
        if (best_move.captured_men > 0 || best_move.captured_kings > 0 || best_move.is_promotion) {
            half_moves = 0;
        } else {
            half_moves++;
        }
        
        make_move(&board, &best_move, current_player);
        
        current_player = !current_player; // Passaggio turno
        if (current_player == 1) turn_number++; // Incrementa il turno reale (turno completo B+N)
    }

    mcts_pool_free();
    return 0;
}
