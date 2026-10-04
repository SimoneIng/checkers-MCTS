#include "checkers/game_state.h"

int is_heuristic_draw(const Bitboard* board) {
    uint32_t white_men = get_white_men(board);
    uint32_t black_men = get_black_men(board);
    
    // Se c'è almeno una pedina, il gioco può ancora evolvere e non è un finale chiuso.
    if (white_men > 0 || black_men > 0) {
        return 0;
    }
    
    uint32_t white_kings = get_white_kings(board);
    uint32_t black_kings = get_black_kings(board);
    
    int num_wk = __builtin_popcount(white_kings);
    int num_bk = __builtin_popcount(black_kings);
    
    // Finali di sole Dame
    if (num_wk > 0 && num_bk > 0) {
        // 1 Dama contro 1 Dama è patta garantita
        if (num_wk == 1 && num_bk == 1) return 1;
        
        // 2 Dame contro 1 Dama è teoricamente patta nella dama italiana
        // (La dama singola può rifugiarsi sulla "strada grande")
        if (num_wk == 2 && num_bk == 1) return 1;
        if (num_wk == 1 && num_bk == 2) return 1;
        
        // 3 Dame vs 1 Dama è normalmente vinta per le 3 dame, ma nei rollout randomici
        // potrebbe durare all'infinito. Lo lasceremo gestire alla regola delle 40 mosse.
    }
    
    return 0;
}

GameStatus check_game_status(Bitboard* board, int is_white_turn, int half_moves) {
    // 1. Regola dei 40 tratti (80 ply senza prese né spostamenti di pedina).
    // È essenziale per impedire loop infiniti durante i rollout dell'MCTS.
    if (half_moves >= 80) {
        return DRAW;
    }
    
    // 2. Patta euristica (taglio immediato per finali morti es. 1v1 Dame).
    // Risponde al requisito: "Utilizzare casi noti in cui un finale di partita è determinato"
    if (is_heuristic_draw(board)) {
        return DRAW;
    }
    
    // 3. Generazione mosse per capire se il giocatore di turno è bloccato (o senza pezzi).
    MoveList list;
    generate_moves(board, is_white_turn, &list);
    
    // Se non ci sono mosse legali, il giocatore di turno ha perso.
    // (Include il caso in cui i suoi pezzi sono = 0, dato che in tal caso list.count = 0).
    if (list.count == 0) {
        return is_white_turn ? BLACK_WINS : WHITE_WINS;
    }
    
    return ONGOING;
}
