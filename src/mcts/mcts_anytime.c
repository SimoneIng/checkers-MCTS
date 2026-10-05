#include "mcts/mcts_anytime.h"
#include <time.h>
#include <stdio.h>

// Funzione helper ad altissima precisione per ottenere i secondi tramite clock POSIX
static double get_time_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

uint32_t mcts_get_best_move_anytime(
    const Bitboard* root_board, 
    int current_player, 
    double time_limit_sec, 
    SelectionPolicy policy, 
    float exploration_constant,
    Move* out_best_move
) {
    // 1. O(1) Reset del pool per pulire l'albero precedente senza invocare la memoria di sistema
    mcts_pool_reset();
    
    // 2. Alloca il nodo radice
    uint32_t root_idx = mcts_alloc_node(NULL_NODE, NULL);
    if (root_idx == NULL_NODE) return 0;
    
    // 3. Setup del timer
    double start_time = get_time_sec();
    double end_time = start_time + time_limit_sec;
    uint32_t iterations = 0;
    
    Bitboard current_board;
    int player;
    
    // 4. MCTS Loop Principale (Anytime)
    while (1) {
        // Controllo del tempo: clock_gettime è veloce, ma per non perdere cicli di CPU 
        // lo invochiamo solo ogni 256 iterazioni usando una bitmask velocissima.
        if ((iterations & 0xFF) == 0) {
            if (get_time_sec() >= end_time) {
                break; // Tempo scaduto, interrompe il loop istantaneamente!
            }
        }
        
        // Reset di board e player prima di scendere nell'albero
        current_board = *root_board;
        player = current_player;
        
        // === FASE 1: Selection ===
        uint32_t selected_node_idx = mcts_select(root_idx, &current_board, &player, policy, exploration_constant);
        MCTSNode* selected_node = mcts_get_node(selected_node_idx);
        
        // === FASE 2: Expansion ===
        if (!selected_node->is_fully_expanded && !selected_node->is_terminal) {
            mcts_expand(selected_node_idx, &current_board, player);
            
            // Selezioniamo il primo figlio appena creato per proseguire col rollout
            if (selected_node->first_child != NULL_NODE) {
                selected_node_idx = selected_node->first_child;
                MCTSNode* first_child = mcts_get_node(selected_node_idx);
                make_move(&current_board, &first_child->move, player);
                player = !player; // Passaggio di turno
            }
        }
        
        // === FASE 3: Simulation (Rollout) ===
        float reward = mcts_simulate(&current_board, player);
        
        // === FASE 4: Backpropagation ===
        mcts_backpropagate(selected_node_idx, reward);
        
        iterations++;
    }
    
    // 5. Estrazione della Mossa Migliore (Robust Child)
    // Selezioniamo il figlio della radice che è stato visitato più volte (il più robusto)
    MCTSNode* root = mcts_get_node(root_idx);
    uint32_t child_idx = root->first_child;
    
    uint32_t best_child_idx = NULL_NODE;
    uint32_t max_visits = 0;
    
    while (child_idx != NULL_NODE) {
        MCTSNode* child = mcts_get_node(child_idx);
        if (child->visits > max_visits) {
            max_visits = child->visits;
            best_child_idx = child_idx;
        }
        child_idx = child->next_sibling;
    }
    
    if (best_child_idx != NULL_NODE && out_best_move != NULL) {
        MCTSNode* best_child = mcts_get_node(best_child_idx);
        *out_best_move = best_child->move;
    }
    
    return iterations;
}
