CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -O2
CPPFLAGS:= $(addprefix -I, $(shell find include -type d))
LDFLAGS :=

SRC_DIR := src
BUILD_DIR := build

# Рекурсивно ищем все .c файлы в src/
SRC := $(shell find $(SRC_DIR) -type f -name '*.c')
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

TARGET := $(BUILD_DIR)/app

# --- Тесты ---
TEST_SRC := $(shell find test -type f -name 'test_*.c')
TEST_BIN := $(patsubst test/%.c,$(BUILD_DIR)/%,$(TEST_SRC))

# Цвета для вывода
GREEN := \033[1;32m
RED := \033[1;31m
RESET := \033[0m

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "$(GREEN)Built $@$(RESET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# --- Тестирование ---
test: $(TEST_BIN)
	@echo -e "\n\nRunning tests..."
	@set -e; \
	for t in $(TEST_BIN); do \
		echo "Testing $$t..."; \
		if ./$$t; then \
			printf "$(GREEN)✔ $$t PASSED$(RESET)\n"; \
		else \
			printf "$(RED)✖ $$t FAILED$(RESET)\n"; \
		fi; \
	done

$(BUILD_DIR)/test_%: test/test_%.c $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@
	@echo "$(GREEN)Built $@$(RESET)"

clean:
	rm -rf $(BUILD_DIR)/

.PHONY: all clean test
