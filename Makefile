CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
BUILD_DIR := build
SRC := src/widget/capy_widget.c
TEST_BIN := $(BUILD_DIR)/test_widget_contracts

.PHONY: all clean lint security test validate version-check

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_BIN): $(SRC) tests/test_widget_contracts.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/widget $(SRC) tests/test_widget_contracts.c $(LDFLAGS) -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

lint:
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only $(SRC)
	git diff --check
	test "$$(cat VERSION)" = "0.0.1"

security:
	$(CC) $(CPPFLAGS) $(CFLAGS) -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fsyntax-only $(SRC)

version-check:
	test "$$(cat VERSION)" = "0.0.1"
	grep -q "Version: 0.0.1" README.md

validate: lint security test version-check

clean:
	rm -rf $(BUILD_DIR)
