#include <emscripten.h>
#include "checkers/bitboard.h"
#include "checkers/move_tables.h"
#include "checkers/move_generator.h"
#include "checkers/game_state.h"
#include "mcts/mcts_anytime.h"
#include <stdio.h>
#include <string.h>

// Stato Globale della partita mantenuto direttamente in C
static Bitboard g_board;
static int g_player;
static int g_half_moves;

EMSCRIPTEN_KEEPALIVE
void init_engine() {
    init_move_tables();
    mcts_pool_init(2000000); // 72MB per l'albero
}

EMSCRIPTEN_KEEPALIVE
void start_game() {
    bitboard_init(&g_board);
    g_player = 1; // 1 = Bianco
    g_half_moves = 0;
}

// Getters per la GUI React
EMSCRIPTEN_KEEPALIVE
uint32_t get_white_pieces() { return g_board.white_pieces; }
EMSCRIPTEN_KEEPALIVE
uint32_t get_black_pieces() { return g_board.black_pieces; }
EMSCRIPTEN_KEEPALIVE
uint32_t get_kings() { return g_board.kings; }
EMSCRIPTEN_KEEPALIVE
int get_current_player() { return g_player; }

EMSCRIPTEN_KEEPALIVE
int get_game_status() {
    GameStatus s = check_game_status(&g_board, g_player, g_half_moves);
    if(s == ONGOING) return 0;
    if(s == WHITE_WINS) return 1;
    if(s == BLACK_WINS) return 2;
    return 3; // DRAW
}

// Restituisce le mosse legali per l'utente umano come stringa "from-to-captures,from-to-captures"
EMSCRIPTEN_KEEPALIVE
char* get_legal_moves_string() {
    static char buffer[2048];
    buffer[0] = '\0';
    
    MoveList list;
    generate_moves(&g_board, g_player, &list);
    
    for(int i = 0; i < list.count; i++) {
        char temp[32];
        int caps = __builtin_popcount(list.moves[i].captured_men | list.moves[i].captured_kings);
        sprintf(temp, "%d-%d-%d,", list.moves[i].from, list.moves[i].to, caps);
        strcat(buffer, temp);
    }
    return buffer;
}

// Applica la mossa scelta dall'utente (Ritorna (caps<<16 | from<<8 | to) se valida, -1 altrimenti)
EMSCRIPTEN_KEEPALIVE
int play_human_move(int from, int to) {
    MoveList list;
    generate_moves(&g_board, g_player, &list);
    
    for(int i = 0; i < list.count; i++) {
        if(list.moves[i].from == from && list.moves[i].to == to) {
            int caps = __builtin_popcount(list.moves[i].captured_men | list.moves[i].captured_kings);
            // Aggiorna contatori patta
            if (caps > 0 || list.moves[i].is_promotion) {
                g_half_moves = 0;
            } else {
                g_half_moves++;
            }
            // Applica e cambia turno
            make_move(&g_board, &list.moves[i], g_player);
            g_player = !g_player;
            return (caps << 16) | (from << 8) | to;
        }
    }
    return -1; // Mossa non trovata o illegale
}

// Fa ragionare il bot e applica la mossa
EMSCRIPTEN_KEEPALIVE
int play_bot_move(double time_limit, int policy, float exploration_c) {
    Move best_move;
    mcts_get_best_move_anytime(&g_board, g_player, time_limit, (SelectionPolicy)policy, exploration_c, 80, &best_move);
    
    int caps = __builtin_popcount(best_move.captured_men | best_move.captured_kings);
    
    if (caps > 0 || best_move.is_promotion) {
        g_half_moves = 0;
    } else {
        g_half_moves++;
    }
    
    make_move(&g_board, &best_move, g_player);
    g_player = !g_player;
    
    return (caps << 16) | (best_move.from << 8) | best_move.to;
}
