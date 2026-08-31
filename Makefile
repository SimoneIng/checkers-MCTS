CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O3
SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/checkers_app # eseguibile finale

# Trova tutti i file sorgente nella cartella src e sottocartelle
SRCS = $(shell find $(SRC_DIR) -name '*.c')
# Genera i nomi dei file oggetto mantenendo la struttura delle directory
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Regola di default
all: $(TARGET)

# Link dell'eseguibile
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

# Compilazione dei singoli file .c in .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Regola per compilare ed eseguire direttamente
run: $(TARGET)
	./$(TARGET)

# Pulizia dei file compilati
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean
