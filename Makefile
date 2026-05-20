# CapyUI Makefile — capypkg publisher
#
# CapyUI owns and publishes its own capypkg modules. The build does NOT
# touch CapyOS sources. After the alpha.241 migration the desktop session
# lives entirely in this repo under src/desktop, src/window and src/apps.
#
# Modules produced by `make package`:
#   org.capyos.ui.widget-core      — widget primitives
#   org.capyos.ui.desktop-session  — desktop + window mgr + apps
#
# Asset naming is intentionally version-less so consumers can fetch the
# latest GitHub Release via the stable redirect:
#   https://github.com/<owner>/<repo>/releases/latest/download/<asset>
# The semantic `version=` field in each .manifest is still taken from the
# VERSION file so capypkg can decide whether an upgrade is needed.

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
BUILD_DIR := build

VERSION := $(shell cat VERSION)

# Widget-core sources.
SRC_WIDGET := src/widget/capy_widget.c src/widget/capy_layout.c src/widget/capy_display_list.c
TEST_BIN := $(BUILD_DIR)/test_widget_contracts

# Desktop-session sources (owned by CapyUI after the alpha.241 migration).
SRC_DESKTOP_SESSION := src/desktop/*.c src/window/*.c src/apps/*.c

# ── capypkg packaging ──────────────────────────────────────────────────────
CAPY_PKG_DIR := $(BUILD_DIR)/capypkg
PUBLISH_OWNER ?= henriquefarisco
PUBLISH_REPO ?= CapyUI
PUBLISH_URL_BASE ?= https://github.com/$(PUBLISH_OWNER)/$(PUBLISH_REPO)/releases/latest/download

WIDGET_PKG_NAME := org.capyos.ui.widget-core
WIDGET_PKG_SUMMARY := CapyUI portable widget primitives (widget + layout + display-list)
WIDGET_PKG_INSTALL_ROOT := /var/capypkg/$(WIDGET_PKG_NAME)
WIDGET_PKG_DEPENDS :=
WIDGET_PKG_BIN := $(CAPY_PKG_DIR)/$(WIDGET_PKG_NAME).bin
WIDGET_PKG_MANIFEST := $(CAPY_PKG_DIR)/$(WIDGET_PKG_NAME).manifest

DESKTOP_PKG_NAME := org.capyos.ui.desktop-session
DESKTOP_PKG_SUMMARY := CapyUI desktop session, window manager and bundled apps
DESKTOP_PKG_INSTALL_ROOT := /var/capypkg/$(DESKTOP_PKG_NAME)
DESKTOP_PKG_DEPENDS := $(WIDGET_PKG_NAME)
DESKTOP_PKG_BIN := $(CAPY_PKG_DIR)/$(DESKTOP_PKG_NAME).bin
DESKTOP_PKG_MANIFEST := $(CAPY_PKG_DIR)/$(DESKTOP_PKG_NAME).manifest
MODULES_INDEX := $(CAPY_PKG_DIR)/modules-index.txt

MODULE_INDEX_MANIFESTS := $(WIDGET_PKG_MANIFEST) $(DESKTOP_PKG_MANIFEST)

.PHONY: all clean lint security test validate version-check \
        package package-widget-core package-desktop-session package-index package-clean

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

package: package-widget-core package-desktop-session package-index

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
	URL="$(PUBLISH_URL_BASE)/$(WIDGET_PKG_NAME).bin" ; \
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

# ── desktop-session (CapyUI in-tree, alpha.241 migration) ─────────────────
package-desktop-session: $(DESKTOP_PKG_MANIFEST)

$(DESKTOP_PKG_BIN): | $(BUILD_DIR)
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
	URL="$(PUBLISH_URL_BASE)/$(DESKTOP_PKG_NAME).bin" ; \
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

package-index: $(MODULES_INDEX)

$(MODULES_INDEX): $(MODULE_INDEX_MANIFESTS)
	@mkdir -p $(CAPY_PKG_DIR)
	@cat $(MODULE_INDEX_MANIFESTS) > $@
	@echo "[package] index: $@"

package-clean:
	rm -rf $(CAPY_PKG_DIR)

clean:
	rm -rf $(BUILD_DIR)
