CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -O2
CPPFLAGS:= -Iinclude             # где искать .h
LDFLAGS :=

SRC_DIR := src
BUILD_DIR := build
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(wildcard $(SRC_DIR)/*.c))

TARGET := $(BUILD_DIR)/app

TEST_SRC := test/test.c
TEST_OBJ := $(BUILD_DIR)/test.o
TEST_BIN := $(BUILD_DIR)/test_app

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Built $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

test: $(OBJ) $(TEST_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $(TEST_BIN)
	@echo "Built test_app"
	@./$(TEST_BIN)

$(TEST_OBJ): $(TEST_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean test

