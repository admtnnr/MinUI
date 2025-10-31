# Test suite for dev platform

CC = gcc
CFLAGS = -Wall -std=gnu99 -I../platform
LDFLAGS = -ldl

TESTS = test_core_loading

all: $(TESTS)
	@echo "All tests built successfully"

test_core_loading: test_core_loading.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

run: all
	@echo "Running unit tests..."
	@echo ""
	@./test_core_loading || true

clean:
	rm -f $(TESTS)

.PHONY: all run clean
