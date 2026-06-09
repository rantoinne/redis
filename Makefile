# Compiler
CC      ?= cc
# Compiler flags
CFLAGS  ?= -Wall -Wextra -Werror -std=c11 -O2 -g
# Linker flags
LDFLAGS ?=

PORT ?= 6379
IMAGE   ?= redis-scratch:latest

# Build directory
BUILD_DIR := build
TARGET    := $(BUILD_DIR)/redis-scratch

# Source files
SRCS := src/main.c

.PHONY: all clean run docker-build docker-run docker-up docker-up-host docker-down docker-test

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

docker-build:
	docker build -t $(IMAGE) .

docker-run: docker-build
	docker run --rm -p $(PORT):$(PORT) $(IMAGE) --port $(PORT)

docker-up:
	docker compose up --build -d

docker-up-host:
	docker compose -f docker-compose.host.yml up --build -d

docker-down:
	docker compose down
	docker compose -f docker-compose.host.yml down 2>/dev/null || true

docker-test: docker-build
	docker run -d --rm --name redis-scratch-test -p $(PORT):$(PORT) $(IMAGE) --port $(PORT)
	sleep 1
	printf 'PING\n' | nc -w 2 127.0.0.1 $(PORT)
	docker stop redis-scratch-test
