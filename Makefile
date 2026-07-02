# CapyUI Makefile — 2.22.2 — ABI capy-ui-widget 2.22 (multi-touch gestures: pinch + rotate)
#
# CapyUI owns and publishes its own capypkg modules. The build does NOT
# touch CapyOS sources. After the alpha.241 migration the desktop session
# lives entirely in this repo under src/desktop, src/window and src/apps.
#
# Modules produced by `make package`:
#   org.capyos.ui.widget-core      — widget primitives
#   org.capyos.ui.desktop-session  — desktop + window mgr + apps
#
# Asset naming is intentionally version-less inside each GitHub Release, but
# manifests use versioned release URLs so the CapyOS kernel downloader does
# not depend on the extra /releases/latest redirect before each payload.

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
BUILD_DIR := build
CURRENT_UID := $(shell id -u 2>/dev/null || echo unknown)
CURRENT_GID := $(shell id -g 2>/dev/null || echo unknown)
ALLOW_ROOT_BUILD ?= 0
ifeq ($(CURRENT_UID),0)
  ifneq ($(ALLOW_ROOT_BUILD),1)
    $(error Refusing to run make as root; run as your normal user to avoid root-owned build artifacts)
  endif
endif
ifneq ($(wildcard $(BUILD_DIR)),)
  ifneq ($(shell test -w "$(BUILD_DIR)" -a -x "$(BUILD_DIR)" && echo ok),ok)
    $(error $(BUILD_DIR) is not writable by the current user; fix ownership with: sudo chown -R $(CURRENT_UID):$(CURRENT_GID) $(BUILD_DIR))
  endif
endif

VERSION := $(shell cat VERSION)

# Widget-core sources.
SRC_WIDGET := src/widget/capy_widget.c src/widget/capy_layout.c src/widget/capy_display_list.c src/widget/capy_dl_gpu.c src/widget/capy_widget_serialize.c
TEST_BIN := $(BUILD_DIR)/test_widget_contracts

# Desktop-session sources (owned by CapyUI after the alpha.241 migration).
SRC_DESKTOP_SESSION := src/desktop/*.c src/window/*.c src/apps/*.c
# Path to CapyOS public headers, required only by `make lint-desktop-session`.
# The desktop-session sources include CapyOS adapter/kernel headers and only
# compile through the cross-repo build path; point this at a CapyOS checkout.
# See docs/roadmap/contracts/desktop-session-coupling.md.
CAPYOS_INCLUDE ?= ../CapyOS/include

# ── capypkg packaging ──────────────────────────────────────────────────────
CAPY_PKG_DIR := $(BUILD_DIR)/capypkg
PUBLISH_OWNER ?= henriquefarisco
PUBLISH_REPO ?= CapyUI
PUBLISH_TAG ?= v$(VERSION)
PUBLISH_URL_BASE ?= https://github.com/$(PUBLISH_OWNER)/$(PUBLISH_REPO)/releases/download/$(PUBLISH_TAG)

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

.PHONY: all clean lint security test validate version-check check-decoupling \
        lint-desktop-session \
        package package-widget-core package-desktop-session package-index package-clean

all: test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TEST_BIN): $(SRC_WIDGET) tests/test_widget_contracts.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/widget $(SRC_WIDGET) tests/test_widget_contracts.c $(LDFLAGS) -o $@
	chmod 755 $@

test: $(TEST_BIN)
	$(TEST_BIN)

lint:
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only $(SRC_WIDGET)
	git diff --check
	test "$(VERSION)" = "2.23.0"

security:
	$(CC) $(CPPFLAGS) $(CFLAGS) -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -fsyntax-only $(SRC_WIDGET)

# version-check also pins the contract-test count so silent test drift trips
# CI: every defined test_*() must be registered in main(), so the count of
# definitions must equal the count of call-sites and both must equal the
# documented total. Bump this number (and the docs) when adding tests.
version-check:
	test "$(VERSION)" = "2.23.0"
	grep -q "Version: 2.22.2" README.md
	test "$$(grep -cE '^  test_[a-z0-9_]+\(\);' tests/test_widget_contracts.c)" = "348"
	test "$$(grep -cE '^static void test_[a-z0-9_]+\(void\)' tests/test_widget_contracts.c)" = "348"

validate: lint security check-decoupling test version-check

# Decoupling invariant (cheap, local): the PORTABLE widget core must never
# include CapyOS kernel/gui/driver headers (rule 10-decoupling-discipline).
# This is a documented release gate (see STATUS.md "próximos passos") that
# used to be run by hand; it is now part of `validate`. src/widget today only
# pulls <stddef.h>/<stdint.h> + local "capy_*.h", so this passes silently.
check-decoupling:
	@bad=$$(grep -rnE '^[[:space:]]*#[[:space:]]*include[[:space:]]*<(gui|kernel|capyos|drivers|fs|net|auth|arch|services|shell|core|lang|util|memory)/' src/widget || true) ; \
	if [ -n "$$bad" ]; then \
	  echo "decoupling violation: src/widget must not include CapyOS headers:" ; \
	  echo "$$bad" ; \
	  exit 1 ; \
	fi ; \
	echo "[check-decoupling] src/widget is free of CapyOS headers"

# External-only syntax gate for the desktop-session sources. These include
# CapyOS adapter/kernel headers (see docs/roadmap/contracts/desktop-session-coupling.md)
# and therefore DO NOT build standalone — the canonical gate is the cross-repo
# build (CapyOS `make all64 PROFILE=full`). This target is a best-effort
# syntax check; run it with a CapyOS checkout:
#   make lint-desktop-session CAPYOS_INCLUDE=../CapyOS/include
# It is intentionally NOT part of `validate` because it needs external headers.
lint-desktop-session:
	@if [ ! -d "$(CAPYOS_INCLUDE)" ]; then \
	  echo "lint-desktop-session: CAPYOS_INCLUDE='$(CAPYOS_INCLUDE)' not found." ; \
	  echo "Point it at a CapyOS checkout, e.g.:" ; \
	  echo "  make lint-desktop-session CAPYOS_INCLUDE=../CapyOS/include" ; \
	  echo "Canonical gate is CapyOS: make all64 PROFILE=full" ; \
	  exit 2 ; \
	fi
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsyntax-only \
	  -Isrc/widget -Isrc -I$(CAPYOS_INCLUDE) $(wildcard $(SRC_DESKTOP_SESSION))
	@echo "[lint-desktop-session] syntax OK against $(CAPYOS_INCLUDE)"

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
