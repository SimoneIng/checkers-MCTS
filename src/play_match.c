#include "checkers/bitboard.h"
#include "checkers/move_tables.h"
#include "checkers/move_generator.h"
#include "checkers/game_state.h"
#include "mcts/mcts_anytime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * CLI runner per singola partita tra due configurazioni bot.
 * Utilizzato dagli script Python di Best Arm Identification.
 * 
 * Uso:
 * ./build/play_match --p1-policy <0|1> --p1-c <float> --p1-cutoff <int> \
 *                   --p2-policy <0|1> --p2-c <float> --p2-cutoff <int> \
 *                   --time <float> [--seed <int>]
 */

int main(int argc, char* argv[]) {
    int p1_policy = 0;
    float p1_c = 1.414f;
    int p1_cutoff = 40;

    int p2_policy = 0;
    float p2_c = 1.414f;
    int p2_cutoff = 40;

    double time_limit = 0.05; // 50ms default per simulazioni veloci
    unsigned int seed = (unsigned int)time(NULL);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--p1-policy") == 0 && i + 1 < argc) p1_policy = atoi(argv[++i]);
        else if (strcmp(argv[i], "--p1-c") == 0 && i + 1 < argc) p1_c = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--p1-cutoff") == 0 && i + 1 < argc) p1_cutoff = atoi(argv[++i]);
        else if (strcmp(argv[i], "--p2-policy") == 0 && i + 1 < argc) p2_policy = atoi(argv[++i]);
        else if (strcmp(argv[i], "--p2-c") == 0 && i + 1 < argc) p2_c = (float)atof(argv[++i]);
        else if (strcmp(argv[i], "--p2-cutoff") == 0 && i + 1 < argc) p2_cutoff = atoi(argv[++i]);
        else if (strcmp(argv[i], "--time") == 0 && i + 1 < argc) time_limit = atof(argv[++i]);
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) seed = (unsigned int)atoi(argv[++i]);
    }

    srand(seed);
    init_move_tables();
    mcts_pool_init(1000000); // 1 Milione di nodi sufficiente per match veloci

    Bitboard board;
    bitboard_init(&board);

    int current_player = 1; // 1 = Bianco (P1), 0 = Nero (P2)
    int half_moves = 0;
    int total_moves = 0;
    uint64_t p1_iters_sum = 0, p2_iters_sum = 0;
    int p1_moves_cnt = 0, p2_moves_cnt = 0;

    int winner = -1; // -1 = DRAW, 1 = P1 (White), 0 = P2 (Black)

    while (total_moves < 200) { // Safety cap a 200 semimosse
        GameStatus status = check_game_status(&board, current_player, half_moves);
        if (status == WHITE_WINS) { winner = 1; break; }
        if (status == BLACK_WINS) { winner = 0; break; }
        if (status == DRAW) { winner = -1; break; }

        int policy = (current_player == 1) ? p1_policy : p2_policy;
        float c = (current_player == 1) ? p1_c : p2_c;
        int cutoff = (current_player == 1) ? p1_cutoff : p2_cutoff;

        Move best_move;
        uint32_t iters = mcts_get_best_move_anytime(
            &board, current_player, time_limit, 
            (SelectionPolicy)policy, c, cutoff, &best_move
        );

        if (current_player == 1) {
            p1_iters_sum += iters;
            p1_moves_cnt++;
        } else {
            p2_iters_sum += iters;
            p2_moves_cnt++;
        }

        if (best_move.captured_men > 0 || best_move.captured_kings > 0 || best_move.is_promotion) {
            half_moves = 0;
        } else {
            half_moves++;
        }

        make_move(&board, &best_move, current_player);
        current_player = !current_player;
        total_moves++;
    }

    mcts_pool_free();

    // Output formattato per il parser Python
    const char* result_str = "DRAW";
    if (winner == 1) result_str = "WHITE";
    else if (winner == 0) result_str = "BLACK";

    uint32_t p1_avg = p1_moves_cnt > 0 ? (uint32_t)(p1_iters_sum / p1_moves_cnt) : 0;
    uint32_t p2_avg = p2_moves_cnt > 0 ? (uint32_t)(p2_iters_sum / p2_moves_cnt) : 0;

    printf("RESULT:%s MOVES:%d P1_ITERS:%u P2_ITERS:%u\n", result_str, total_moves, p1_avg, p2_avg);
    return 0;
}
