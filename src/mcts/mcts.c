#include "mcts/mcts.h"
#include <stdio.h>
#include <string.h>

// Definizione della variabile globale del pool
NodePool g_node_pool = {NULL, 0, 0};

void mcts_pool_init(uint32_t capacity) {
    if (g_node_pool.nodes != NULL) {
        printf("Avviso: Memory Pool già inizializzato.\n");
        return;
    }
    
    // Allocazione singola massiva di RAM
    g_node_pool.nodes = (MCTSNode*)malloc(capacity * sizeof(MCTSNode));
    if (g_node_pool.nodes == NULL) {
        fprintf(stderr, "ERRORE CRITICO: Impossibile allocare %u nodi per l'MCTS!\n", capacity);
        exit(1);
    }
    
    g_node_pool.capacity = capacity;
    g_node_pool.count = 0;
    
    printf("Memory Pool MCTS inizializzato con %u nodi (circa %.2f MB).\n", 
           capacity, (float)(capacity * sizeof(MCTSNode)) / (1024 * 1024));
}

void mcts_pool_free() {
    if (g_node_pool.nodes != NULL) {
        free(g_node_pool.nodes);
        g_node_pool.nodes = NULL;
        g_node_pool.capacity = 0;
        g_node_pool.count = 0;
        printf("Memory Pool MCTS liberato.\n");
    }
}

void mcts_pool_reset() {
    // Basta azzerare il counter! I vecchi dati verranno semplicemente sovrascritti.
    // L'algoritmo non andrà mai a leggere nodi oltre `g_node_pool.count`.
    g_node_pool.count = 0;
}

uint32_t mcts_alloc_node(uint32_t parent_idx, const Move* move) {
    // Se abbiamo finito la RAM preallocata, dobbiamo fermarci per prevenire segfault
    if (g_node_pool.count >= g_node_pool.capacity) {
        // Raggiunto il limite dell'albero (il root controller dovrà gestire questo limite gentilmente e interrompere l'esplorazione)
        return NULL_NODE;
    }
    
    uint32_t new_idx = g_node_pool.count++;
    MCTSNode* node = &g_node_pool.nodes[new_idx];
    
    // Inizializzazione pulita del nodo
    node->visits = 0;
    node->score = 0.0f;
    node->parent = parent_idx;
    node->first_child = NULL_NODE;
    node->next_sibling = NULL_NODE;
    
    if (move != NULL) {
        node->move = *move;
    } else {
        memset(&node->move, 0, sizeof(Move));
    }
    
    node->is_terminal = 0;
    node->is_fully_expanded = 0;
    node->padding = 0;
    
    return new_idx;
}

#include "checkers/game_state.h"
#include "checkers/game_state.h"
#include <stdlib.h>

uint32_t mcts_select(uint32_t root_idx, Bitboard* current_board, int* current_player, SelectionPolicy policy, float exploration_constant) {
    uint32_t current_idx = root_idx;
    MCTSNode* node = mcts_get_node(current_idx);
    
    while (node->is_fully_expanded && !node->is_terminal) {
        uint32_t best_child = NULL_NODE;
        float best_score = -1000000.0f;
        
        uint32_t child_idx = node->first_child;
        while (child_idx != NULL_NODE) {
            MCTSNode* child = mcts_get_node(child_idx);
            
            if (child->visits == 0) {
                best_child = child_idx;
                break;
            }
            
            float score = 0.0f;
            if (policy == POLICY_UCB1) {
                score = compute_ucb1(node->visits, child->visits, child->score, *current_player, exploration_constant);
            } else if (policy == POLICY_PUCT) {
                score = compute_puct(node->visits, child->visits, child->score, *current_player, exploration_constant);
            }
            
            if (score > best_score) {
                best_score = score;
                best_child = child_idx;
            }
            
            child_idx = child->next_sibling;
        }
        
        if (best_child == NULL_NODE) break; // Fallback di sicurezza
        
        // Applichiamo la mossa per mantenere sincronizzato il board
        MCTSNode* chosen = mcts_get_node(best_child);
        make_move(current_board, &chosen->move, *current_player);
        *current_player = !(*current_player); // Cambia turno
        
        current_idx = best_child;
        node = chosen;
    }
    
    return current_idx;
}

void mcts_expand(uint32_t node_idx, Bitboard* board, int current_player) {
    MCTSNode* node = mcts_get_node(node_idx);
    
    // Controlliamo se siamo in uno stato terminale (senza considerare le 40 mosse per ora)
    GameStatus status = check_game_status(board, current_player, 0);
    if (status != ONGOING) {
        node->is_terminal = 1;
        return;
    }
    
    MoveList list;
    generate_moves(board, current_player, &list);
    
    if (list.count == 0) {
        node->is_terminal = 1;
        return;
    }
    
    uint32_t prev_child_idx = NULL_NODE;
    
    for (int i = 0; i < list.count; i++) {
        uint32_t child_idx = mcts_alloc_node(node_idx, &list.moves[i]);
        if (child_idx == NULL_NODE) break; // Memoria esaurita!
        
        if (prev_child_idx == NULL_NODE) {
            node->first_child = child_idx;
        } else {
            MCTSNode* prev_child = mcts_get_node(prev_child_idx);
            prev_child->next_sibling = child_idx;
        }
        prev_child_idx = child_idx;
    }
    
    node->is_fully_expanded = 1;
}

float mcts_simulate(Bitboard* board, int current_player) {
    Bitboard sim_board = *board; // Lavoriamo su una copia per preservare lo stato dell'albero
    int player = current_player;
    int half_moves = 0;
    
    while (1) {
        GameStatus status = check_game_status(&sim_board, player, half_moves);
        if (status == WHITE_WINS) return 1.0f;
        if (status == BLACK_WINS) return 0.0f;
        if (status == DRAW) return 0.5f;
        
        MoveList list;
        generate_moves(&sim_board, player, &list);
        
        // Sicurezza: se non ci sono mosse e check_game_status non l'ha preso (es. board invalida)
        if (list.count == 0) {
             return player == 1 ? 0.0f : 1.0f; // Il giocatore corrente perde
        }
        
        // Mossa completamente casuale
        int random_idx = rand() % list.count;
        Move* move = &list.moves[random_idx];
        
        // Aggiorna contatore patta per inattività (regola delle 40 mosse)
        if (move->captured_men > 0 || move->captured_kings > 0 || move->is_promotion) {
            half_moves = 0; // Reset se c'è mangiata o promozione
        } else {
            half_moves++;
        }
        
        make_move(&sim_board, move, player);
        player = !player;
    }
}

void mcts_backpropagate(uint32_t node_idx, float reward_for_white) {
    uint32_t curr = node_idx;
    while (curr != NULL_NODE) {
        MCTSNode* node = mcts_get_node(curr);
        node->visits++;
        node->score += reward_for_white;
        curr = node->parent;
    }
}
