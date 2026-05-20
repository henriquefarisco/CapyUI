# CapyUI Makefile — 0.7.0 (alpha.241)
#
# Since 0.7.0 CapyUI publishes TWO capypkg modules:
#   org.capyos.ui.widget-core      — widget primitives (unchanged since 0.6)
#   org.capyos.ui.desktop-session  — desktop + window mgr + apps (NEW)
#
# The desktop-session payload is only built when the corresponding src
# subtrees (src/desktop, src/window, src/apps) are present in this
# checkout. That subtree is populated by the migrate_to_capyui.py
# helper that ships in the sibling CapyOS repo. Before that helper
# runs, this Makefile still builds and publishes just the widget-core
# module (lint/test/security all keep working).

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
BUILD_DIR := build
CAPYOS_DIR ?= ../CapyOS

VERSION := $(shell cat VERSION)

# Widget-core sources (always present).
SRC_WIDGET := src/widget/capy_widget.c src/widget/capy_layout.c src/widget/capy_display_list.c
TEST_BIN := $(BUILD_DIR)/test_widget_contracts

# Desktop-session sources (present only after migrate_to_capyui.py runs).
DESKTOP_SUBTREE_PRESENT := $(wildcard src/desktop/desktop_runtime.c)
CAPYOS_HEADERS_PRESENT := $(wildcard $(CAPYOS_DIR)/include/gui/desktop.h)
SRC_DESKTOP_SESSION := src/desktop/*.c src/window/*.c src/apps/*.c
DESKTOP_SESSION_CPPFLAGS := \
	-I$(CAPYOS_DIR)/include \
	-I$(CAPYOS_DIR)/src \
	-I$(CAPYOS_DIR)/build/generated \
	-I$(CAPYOS_DIR)/third_party/bearssl/inc \
	-I$(CAPYOS_DIR)/third_party/bearssl/src \
	-I$(CAPYOS_DIR)/third_party/tinf \
	-Isrc/desktop \
	-Isrc/desktop/internal \
	-Isrc/apps/internal

# ── capypkg packaging (alpha.241 modular profile) ──────────────────────────
CAPY_PKG_DIR := $(BUILD_DIR)/capypkg
PUBLISH_URL_BASE ?= https://github.com/henriquefarisco/CapyUI/releases/download/v$(VERSION)

WIDGET_PKG_NAME := org.capyos.ui.widget-core
WIDGET_PKG_SUMMARY := CapyUI portable widget primitives (widget + layout + display-list)
WIDGET_PKG_INSTALL_ROOT := /var/capypkg/$(WIDGET_PKG_NAME)
WIDGET_PKG_DEPENDS :=
WIDGET_PKG_BIN := $(CAPY_PKG_DIR)/$(WIDGET_PKG_NAME)-$(VERSION).bin
WIDGET_PKG_MANIFEST := $(CAPY_PKG_DIR)/$(WIDGET_PKG_NAME).manifest

DESKTOP_PKG_NAME := org.capyos.ui.desktop-session
DESKTOP_PKG_SUMMARY := CapyUI desktop session, window manager and bundled apps
DESKTOP_PKG_INSTALL_ROOT := /var/capypkg/$(DESKTOP_PKG_NAME)
DESKTOP_PKG_DEPENDS := $(WIDGET_PKG_NAME)
DESKTOP_PKG_BIN := $(CAPY_PKG_DIR)/$(DESKTOP_PKG_NAME)-$(VERSION).bin
DESKTOP_PKG_MANIFEST := $(CAPY_PKG_DIR)/$(DESKTOP_PKG_NAME).manifest

.PHONY: all clean lint security test validate version-check desktop-session-lint \
        package package-widget-core package-desktop-session package-clean

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_BIN): $(SRC_WIDGET) tests/test_widget_contracts.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/widget $(SRC_WIDGET) tests/test_widget_contracts.c $(LDFLAGS) -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

lint:
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only $(SRC_WIDGET)
	git diff --check
	test "$(VERSION)" = "0.7.0"

security:
	$(CC) $(CPPFLAGS) $(CFLAGS) -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fsyntax-only $(SRC_WIDGET)

version-check:
	test "$(VERSION)" = "0.7.0"
	grep -q "Version: 0.7.0" README.md

validate: lint security test version-check

desktop-session-lint:
ifneq ($(DESKTOP_SUBTREE_PRESENT),)
ifneq ($(CAPYOS_HEADERS_PRESENT),)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DESKTOP_SESSION_CPPFLAGS) -fsyntax-only $(SRC_DESKTOP_SESSION)
else
	@echo "[validate] CapyOS headers not found at $(CAPYOS_DIR); skipping desktop-session syntax lint."
endif
else
	@echo "[validate] desktop-session subtree absent; skipping desktop-session syntax lint."
endif

validate: desktop-session-lint

# Default `package` target builds whichever modules are available
# in this checkout. After running migrate_to_capyui.py from CapyOS,
# both modules will be packaged; before that, only widget-core.
ifeq ($(DESKTOP_SUBTREE_PRESENT),)
package: package-widget-core
	@echo "[package] desktop-session subtree absent; only widget-core packaged."
	@echo "[package] run CapyOS/tools/scripts/migrate_to_capyui.py --apply to enable it."
else
package: package-widget-core package-desktop-session
endif

# ── widget-core (always available) ─────────────────────────────────────────
package-widget-core: $(WIDGET_PKG_MANIFEST)

$(WIDGET_PKG_BIN): $(SRC_WIDGET) | $(BUILD_DIR)
	@mkdir -p $(CAPY_PKG_DIR)
	@tar --format=ustar --owner=0 --group=0 --numeric-owner \
	     --mtime='@0' --sort=name \
	     -cf $@ src/widget docs VERSION 2>/dev/null || \
	  tar -cf $@ src/widget docs VERSION
	@echo "[package] $@"

$(WIDGET_PKG_MANIFEST): $(WIDGET_PKG_BIN)
	@SHA=$$(shasum -a 256 $(WIDGET_PKG_BIN) 2>/dev/null | awk '{print $$1}' | tr 'A-F' 'a-f') ; \
	if [ -z "$$SHA" ]; then SHA=$$(sha256sum $(WIDGET_PKG_BIN) | awk '{print $$1}' | tr 'A-F' 'a-f'); fi ; \
	SIZE=$$(wc -c < $(WIDGET_PKG_BIN) | tr -d ' ') ; \
	URL="$(PUBLISH_URL_BASE)/$(WIDGET_PKG_NAME)-$(VERSION).bin" ; \
	{ \
	  echo "name=$(WIDGET_PKG_NAME)" ; \
	  echo "version=$(VERSION)" ; \
	  echo "summary=$(WIDGET_PKG_SUMMARY)" ; \
	  echo "payload_url=$$URL" ; \
	  echo "payload_sha256=$$SHA" ; \
	  echo "payload_size=$$SIZE" ; \
	  echo "install_root=$(WIDGET_PKG_INSTALL_ROOT)" ; \
	  echo "depends=$(WIDGET_PKG_DEPENDS)" ; \
	  echo "---" ; \
	} > $@
	@echo "[package] manifest: $@"

# ── desktop-session (available only after migrate_to_capyui.py) ───────────
package-desktop-session: $(DESKTOP_PKG_MANIFEST)

$(DESKTOP_PKG_BIN): | $(BUILD_DIR)
	@if [ -z "$(DESKTOP_SUBTREE_PRESENT)" ]; then \
	  echo "[package] desktop-session source subtree missing; run migrate_to_capyui.py first." ; \
	  exit 2 ; \
	fi
	@mkdir -p $(CAPY_PKG_DIR)
	@tar --format=ustar --owner=0 --group=0 --numeric-owner \
	     --mtime='@0' --sort=name \
	     -cf $@ src/desktop src/window src/apps docs VERSION 2>/dev/null || \
	  tar -cf $@ src/desktop src/window src/apps docs VERSION
	@echo "[package] $@"

$(DESKTOP_PKG_MANIFEST): $(DESKTOP_PKG_BIN)
	@SHA=$$(shasum -a 256 $(DESKTOP_PKG_BIN) 2>/dev/null | awk '{print $$1}' | tr 'A-F' 'a-f') ; \
	if [ -z "$$SHA" ]; then SHA=$$(sha256sum $(DESKTOP_PKG_BIN) | awk '{print $$1}' | tr 'A-F' 'a-f'); fi ; \
	SIZE=$$(wc -c < $(DESKTOP_PKG_BIN) | tr -d ' ') ; \
	URL="$(PUBLISH_URL_BASE)/$(DESKTOP_PKG_NAME)-$(VERSION).bin" ; \
	{ \
	  echo "name=$(DESKTOP_PKG_NAME)" ; \
	  echo "version=$(VERSION)" ; \
	  echo "summary=$(DESKTOP_PKG_SUMMARY)" ; \
	  echo "payload_url=$$URL" ; \
	  echo "payload_sha256=$$SHA" ; \
	  echo "payload_size=$$SIZE" ; \
	  echo "install_root=$(DESKTOP_PKG_INSTALL_ROOT)" ; \
	  echo "depends=$(DESKTOP_PKG_DEPENDS)" ; \
	  echo "---" ; \
	} > $@
	@echo "[package] manifest: $@"

package-clean:
	rm -rf $(CAPY_PKG_DIR)

clean:
	rm -rf $(BUILD_DIR)
