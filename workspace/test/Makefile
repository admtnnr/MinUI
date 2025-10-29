# MinUI Configuration System Tests

CC = gcc
CFLAGS = -Wall -Wextra -I. -I../all/common -DUSE_CONFIG_SYSTEM=1 -DUSE_SDL2=1 -g
LDFLAGS = `pkg-config --libs sdl2`

SRCS = test_config.c ../all/common/config.c
OBJS = $(SRCS:.c=.o)
TARGET = test_config

.PHONY: all clean test

all: $(TARGET)

$(TARGET): test_config.c ../all/common/config.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TARGET)
	@echo "Running configuration system tests..."
	@./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: help
help:
	@echo "MinUI Configuration Tests"
	@echo ""
	@echo "Targets:"
	@echo "  make        - Build test executable"
	@echo "  make test   - Build and run tests"
	@echo "  make clean  - Remove built files"
