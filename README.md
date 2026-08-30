# checkers-MCTS
AI Agent for checkers based on Monte Carlo Search Tree Search (MCTS). Developed in C with bitboard optimization

## Project Description 
This repo contains an artificial player for Checkers game. The system allows a human user to challenge the AI by moving pieces via mouse-driver interactive interface. 
The decision engine uses the Monte Carlo Tree Search algorithm, which relies on random simulations and cyclically through Selection, Expansion, Simulation and Backpropagation phases. 

## Technical Features
- Asymmetric Anytime algorithm search, calibrated to stop and return the best move at analysis intervals of 0.2, 1, and 3 seconds.
- Game state representation entirely based on bitboards to ensure fast bitwise operations, using look-up tables for legal moves and strictly avoiding state cloning during the roll-out.
- Integration of two models to manage exploration vs exploitation dilemma: classic UCB1 formula and an advanced alternative variant (such as PUCT).
- Optimized roll-out routing using checkers rules or known endgame states to prevent the excess of draws and inconclusive endings typical of purely random simulations.  
