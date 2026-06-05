# Compiler
CC      ?= cc
# Compiler flags
CFLAGS  ?= -Wall -Wextra -Werror -std=c11 -O2 -g
# Linker flags
LDFLAGS ?=

PORT ?= 6379

# Build directory
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/redis-scratch

# Source files
SRCS := src/main.c

.PHONY: all clean run

# Targets
all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $@ $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET) --port $(PORT)
