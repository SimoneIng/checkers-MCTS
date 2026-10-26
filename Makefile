CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O3
SRC_DIR = src
BUILD_DIR = build

TARGET = $(BUILD_DIR)/checkers_app
MATCH_TARGET = $(BUILD_DIR)/play_match

# File oggetto comuni della logica del gioco e MCTS (senza main, play_match, wasm_api)
CORE_SRCS = $(filter-out src/main.c src/play_match.c src/wasm_api.c, $(shell find $(SRC_DIR) -name '*.c'))
CORE_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(CORE_SRCS))

# Regola di default: compila entrambi gli eseguibili
all: $(TARGET) $(MATCH_TARGET)

# Eseguibile principale (interattivo a terminale)
$(TARGET): $(CORE_OBJS) $(BUILD_DIR)/main.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

# Runner CLI per partite singole (utilizzato da Python BAI)
$(MATCH_TARGET): $(CORE_OBJS) $(BUILD_DIR)/play_match.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

# Compilazione generica dei file oggetto
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

# Automazione Tuning e Validazione Sperimentale (Python)
tune-ucb1: $(MATCH_TARGET)
	python3 training/tune_ucb1.py

tune-puct: $(MATCH_TARGET)
	python3 training/tune_puct.py

showdown: $(MATCH_TARGET)
	python3 training/showdown.py

validate-oos: $(MATCH_TARGET)
	python3 training/validate_out_of_sample.py

report:
	python3 training/generate_academic_report.py

.PHONY: all run clean tune-ucb1 tune-puct showdown validate-oos report
