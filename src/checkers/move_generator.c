#include "checkers/move_generator.h"

// Macro per identificare se un indice si trova sull'ultima riga (promozione)
#define IS_WHITE_PROMOTION(sq) ((sq) >= 28)
#define IS_BLACK_PROMOTION(sq) ((sq) <= 3)

/**
 * @brief Algoritmo DFS per trovare tutte le catene di salti (catture multiple)
 */
static void jump_dfs(Bitboard* board, int start_sq, int current_sq, int is_white, int is_king, 
                     uint32_t cap_men, uint32_t cap_kings, uint8_t path_score, int depth, MoveList* list) {
    
    uint32_t opponent_pieces = is_white ? board->black_pieces : board->white_pieces;
    uint32_t opponent_kings = is_white ? (board->black_pieces & board->kings) : (board->white_pieces & board->kings);
    uint32_t opponent_pawns = opponent_pieces & ~opponent_kings;
    
    uint32_t all_pieces = board->white_pieces | board->black_pieces;
    // Rimuoviamo temporaneamente i pezzi già mangiati in questa catena per non rimangiarli o bloccarci
    all_pieces &= ~(cap_men | cap_kings);
    // E rimuoviamo temporaneamente il pezzo che sta saltando (la sua casella originale) per permettere salti circolari (nella dama italiana i salti circolari sono rari ma possibili se la scacchiera si svuota in catena)
    all_pieces &= ~(1U << start_sq);
    
    int can_jump_further = 0;
    
    // Funzione helper inline-simile per processare una direzione
    #define TRY_JUMP(dir, is_valid_dir) \
        if (is_valid_dir) { \
            uint32_t cap_mask = CAPTURES[current_sq][dir]; \
            uint32_t dest_mask = JUMPS[current_sq][dir]; \
            /* Se c'è una preda potenziale e la destinazione è vuota */ \
            if (cap_mask && dest_mask && !(all_pieces & dest_mask)) { \
                int is_captured_king = (cap_mask & opponent_kings) != 0; \
                int is_captured_pawn = (cap_mask & opponent_pawns) != 0; \
                /* Una pedina non può mangiare una dama nella variante italiana */ \
                if ((is_captured_king && is_king) || is_captured_pawn) { \
                    if (!(cap_mask & (cap_men | cap_kings))) { /* Non è già stato mangiato */ \
                        can_jump_further = 1; \
                        uint32_t new_cap_men = cap_men | (is_captured_pawn ? cap_mask : 0); \
                        uint32_t new_cap_kings = cap_kings | (is_captured_king ? cap_mask : 0); \
                        /* Precedenza: i primi salti su dame valgono di più (bit più alto) */ \
                        uint8_t new_score = path_score | (is_captured_king ? (1U << (7 - depth)) : 0); \
                        int next_sq = __builtin_ctz(dest_mask); \
                        /* Regola italiana: se una pedina raggiunge l'ultima riga durante un salto, viene promossa e il suo turno finisce subito */ \
                        int promotes_now = (!is_king) && (is_white ? IS_WHITE_PROMOTION(next_sq) : IS_BLACK_PROMOTION(next_sq)); \
                        if (promotes_now) { \
                            if (list->count < MAX_MOVES) { \
                                list->moves[list->count++] = (Move){start_sq, next_sq, 1, new_score, new_cap_men, new_cap_kings}; \
                            } \
                        } else { \
                            jump_dfs(board, start_sq, next_sq, is_white, is_king, new_cap_men, new_cap_kings, new_score, depth + 1, list); \
                        } \
                    } \
                } \
            } \
        }
    
    if (is_king) {
        TRY_JUMP(DIR_NW, 1); TRY_JUMP(DIR_NE, 1); TRY_JUMP(DIR_SW, 1); TRY_JUMP(DIR_SE, 1);
    } else if (is_white) {
        TRY_JUMP(DIR_NW, 1); TRY_JUMP(DIR_NE, 1);
    } else {
        TRY_JUMP(DIR_SW, 1); TRY_JUMP(DIR_SE, 1);
    }
    
    // Se siamo alla fine della catena di salti e abbiamo mangiato almeno un pezzo
    if (!can_jump_further && depth > 0) {
        if (list->count < MAX_MOVES) {
            list->moves[list->count++] = (Move){start_sq, current_sq, 0, path_score, cap_men, cap_kings};
        }
    }
}

/**
 * @brief Filtra le mosse applicando la rigidissima gerarchia della Dama Italiana.
 */
static void filter_moves(MoveList* list, Bitboard* board) {
    if (list->count <= 1) return;
    
    int best_score = -1;
    
    // 1. Calcola il punteggio massimo tra tutte le mosse
    for (int i = 0; i < list->count; i++) {
        Move* m = &list->moves[i];
        int num_caps = __builtin_popcount(m->captured_men) + __builtin_popcount(m->captured_kings);
        if (num_caps == 0) continue; // Spostamento semplice, score 0
        
        int is_king_capturing = (board->kings & (1U << m->from)) != 0;
        int num_kings_cap = __builtin_popcount(m->captured_kings);
        
        // Costruiamo un intero comparabile che rispetta la gerarchia:
        // Bit 24-31: Max quantità pezzi catturati (Regola 1)
        // Bit 23: Qualità pezzo catturante (Regola 2: Dama mangia a preferenza)
        // Bit 16-22: Max Dame catturate (Regola 3)
        // Bit 0-7: Precedenza (Regola 4: Mangiare Dama il prima possibile)
        int score = (num_caps << 24) | (is_king_capturing << 23) | (num_kings_cap << 16) | m->path_score;
        
        if (score > best_score) {
            best_score = score;
        }
    }
    
    if (best_score == -1) return; // Erano tutti spostamenti semplici
    
    // 2. Compatta l'array tenendo solo le mosse che pareggiano il punteggio massimo
    int new_count = 0;
    for (int i = 0; i < list->count; i++) {
        Move* m = &list->moves[i];
        int num_caps = __builtin_popcount(m->captured_men) + __builtin_popcount(m->captured_kings);
        int is_king_capturing = (board->kings & (1U << m->from)) != 0;
        int num_kings_cap = __builtin_popcount(m->captured_kings);
        
        int score = (num_caps << 24) | (is_king_capturing << 23) | (num_kings_cap << 16) | m->path_score;
        
        if (score == best_score) {
            list->moves[new_count++] = *m;
        }
    }
    list->count = new_count;
}

void generate_moves(Bitboard* board, int is_white, MoveList* list) {
    list->count = 0;
    uint32_t my_pieces = is_white ? board->white_pieces : board->black_pieces;
    uint32_t my_kings = my_pieces & board->kings;
    uint32_t empty_squares = ~(board->white_pieces | board->black_pieces);
    
    // FASE 1: Ricerca dei Salti (Obbligo di presa)
    uint32_t pieces_to_check = my_pieces;
    while (pieces_to_check) {
        int sq = __builtin_ctz(pieces_to_check);           // Trovato il pezzo (Bit-Scanner)
        pieces_to_check &= (pieces_to_check - 1);         // Rimuovi il bit per il prossimo ciclo
        int is_king = (my_kings & (1U << sq)) != 0;
        
        jump_dfs(board, sq, sq, is_white, is_king, 0, 0, 0, 0, list);
    }
    
    // Filtro regole italiane sui salti trovati
    if (list->count > 0) {
        filter_moves(list, board);
        return; // Obbligo di presa: se ci sono salti, ignora gli spostamenti semplici
    }
    
    // FASE 2: Spostamenti semplici (se nessun salto era possibile)
    pieces_to_check = my_pieces;
    while (pieces_to_check) {
        int sq = __builtin_ctz(pieces_to_check);
        pieces_to_check &= (pieces_to_check - 1);
        int is_king = (my_kings & (1U << sq)) != 0;
        
        #define TRY_MOVE(dir, is_valid_dir) \
            if (is_valid_dir) { \
                uint32_t dest = MOVES[sq][dir]; \
                if (dest && (dest & empty_squares)) { \
                    int next_sq = __builtin_ctz(dest); \
                    int promotes = (!is_king) && (is_white ? IS_WHITE_PROMOTION(next_sq) : IS_BLACK_PROMOTION(next_sq)); \
                    if (list->count < MAX_MOVES) { \
                        list->moves[list->count++] = (Move){sq, next_sq, promotes, 0, 0, 0}; \
                    } \
                } \
            }
            
        if (is_king) {
            TRY_MOVE(DIR_NW, 1); TRY_MOVE(DIR_NE, 1); TRY_MOVE(DIR_SW, 1); TRY_MOVE(DIR_SE, 1);
        } else if (is_white) {
            TRY_MOVE(DIR_NW, 1); TRY_MOVE(DIR_NE, 1);
        } else {
            TRY_MOVE(DIR_SW, 1); TRY_MOVE(DIR_SE, 1);
        }
    }
}

void apply_move_xor(Bitboard* b, const Move* m, int is_white) {
    uint32_t from_to_mask = (1U << m->from) | (1U << m->to);
    
    // 1. Sposta fisicamente il pezzo sulla scacchiera
    if (is_white) {
        b->white_pieces ^= from_to_mask;
        b->black_pieces ^= (m->captured_men | m->captured_kings); // Rimuove/Ripristina le vittime
    } else {
        b->black_pieces ^= from_to_mask;
        b->white_pieces ^= (m->captured_men | m->captured_kings);
    }
    
    // 2. Gestione Promozione e status di Dama del pezzo mosso
    if (m->is_promotion) {
        // Se è una promozione, lo XOR sul 'to' trasforma magicamente la pedina in Dama (Make)
        // oppure le toglie la corona ripristinandola a pedina (Unmake)
        b->kings ^= (1U << m->to);
    } else if (b->kings & from_to_mask) {
        // Se era GIA' una Dama (sia che parta da 'from' in Make, sia da 'to' in Unmake), 
        // muove lo status di Dama da una casella all'altra.
        b->kings ^= from_to_mask;
    }
    
    // 3. Ripristina/Rimuove le corone delle vittime (Le pedine mangiate non avevano corona)
    b->kings ^= m->captured_kings;
}
