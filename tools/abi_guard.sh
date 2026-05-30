#!/usr/bin/env bash
# CapyUI ABI removal guard.
#
# Implements the automated check mandated by
# docs/roadmap/contracts/deprecation-policy.md ("Verificação automática"):
# detect public symbols that disappeared from the widget-core public headers
# relative to a baseline git ref.
#
# Policy enforced here:
#   - Within the SAME major version, public-symbol removals are FORBIDDEN
#     (the policy requires >=2 deprecation minors, then removal only in a
#     major bump). This is the hard, automatable part.
#   - Across a MAJOR bump, removals are reported as informational (a major
#     bump is the only place removals are allowed; the >=2-minor deprecation
#     window itself is checked by humans against CHANGELOG/release notes).
#
# Heuristic surface: function-like names `capy_*(...)` plus uppercase
# `CAPY*` macro/enum tokens taken from the public headers below. Intentional
# removals during a major bump can be acknowledged with
# ABI_GUARD_ALLOW_REMOVALS=1. Known false positives can be silenced via
# tools/abi_guard_ignore.txt (one symbol per line; '#' starts a comment).
#
# Usage:
#   bash tools/abi_guard.sh                 # baseline = latest v* tag
#   BASE_REF=v2.13.0 bash tools/abi_guard.sh
#   ABI_GUARD_ALLOW_REMOVALS=1 bash tools/abi_guard.sh   # ack major-bump removals
set -euo pipefail

HEADERS="src/widget/capy_widget.h src/widget/capy_layout.h src/widget/capy_display_list.h src/widget/capy_dl_gpu.h"
IGNORE_FILE="tools/abi_guard_ignore.txt"

# ── resolve baseline ref ────────────────────────────────────────────────────
base_ref="${BASE_REF:-}"
if [ -z "$base_ref" ]; then
  base_ref="$(git tag --list 'v*' --sort=-v:refname --merged HEAD 2>/dev/null | head -n1 || true)"
fi
if [ -z "$base_ref" ]; then
  echo "[abi-guard] no baseline 'v*' tag reachable from HEAD; skipping (first release?)."
  exit 0
fi
echo "[abi-guard] baseline ref: $base_ref"

# ── symbol extraction ───────────────────────────────────────────────────────
# $1 = "git-show <ref>" sink or "cat" sink chooser handled by callers.
extract_from_stream() {
  # reads header text on stdin, prints one public symbol per line (unsorted)
  local content
  content="$(cat)"
  # function-like names: capy_xxx(
  printf '%s\n' "$content" | grep -oE 'capy_[a-z0-9_]+ *\(' | sed 's/[ (]//g' || true
  # object/function macros: #define CAPY... / #define CAPYUI...
  printf '%s\n' "$content" \
    | grep -oE '^[[:space:]]*#[[:space:]]*define[[:space:]]+CAPY[A-Z0-9_]+' \
    | sed -E 's/.*define[[:space:]]+//' || true
  # enum constants at line start: CAPY_FOO,  /  CAPY_FOO = 1
  printf '%s\n' "$content" \
    | grep -oE '^[[:space:]]*CAPY_[A-Z0-9_]+' \
    | sed -E 's/[[:space:]]+//g' || true
}

collect_base() {
  for h in $HEADERS; do
    git show "$base_ref:$h" 2>/dev/null || true
  done | extract_from_stream | sort -u
}

collect_current() {
  # Reads the WORKING TREE so the guard also catches uncommitted edits and
  # works on a CI checkout of the PR head.
  for h in $HEADERS; do
    [ -f "$h" ] && cat "$h" || true
  done | extract_from_stream | sort -u
}

major_of_ref() {
  git show "$1:src/widget/capy_widget.h" 2>/dev/null \
    | grep -E '^#define[[:space:]]+CAPYUI_API_VERSION_MAJOR' \
    | grep -oE '[0-9]+' | head -n1 || true
}
major_of_worktree() {
  grep -E '^#define[[:space:]]+CAPYUI_API_VERSION_MAJOR' src/widget/capy_widget.h \
    | grep -oE '[0-9]+' | head -n1 || true
}

base_file="$(mktemp)"; cur_file="$(mktemp)"; rm_file="$(mktemp)"
trap 'rm -f "$base_file" "$cur_file" "$rm_file"' EXIT

collect_base    > "$base_file"
collect_current > "$cur_file"

# removed = present in base, absent in current
comm -23 "$base_file" "$cur_file" > "$rm_file"

# drop ignored symbols
if [ -f "$IGNORE_FILE" ]; then
  grep -vE '^[[:space:]]*(#|$)' "$IGNORE_FILE" | sed -E 's/[[:space:]]+//g' | sort -u > "${rm_file}.ign" || true
  if [ -s "${rm_file}.ign" ]; then
    comm -23 "$rm_file" "${rm_file}.ign" > "${rm_file}.f" || true
    mv "${rm_file}.f" "$rm_file"
  fi
  rm -f "${rm_file}.ign"
fi

base_major="$(major_of_ref "$base_ref")"; [ -n "$base_major" ] || base_major=0
head_major="$(major_of_worktree)";       [ -n "$head_major" ] || head_major=0

if [ ! -s "$rm_file" ]; then
  echo "[abi-guard] OK — no public symbols removed vs $base_ref (major $base_major -> $head_major)."
  exit 0
fi

echo "[abi-guard] public symbols present in $base_ref but missing now:"
sed 's/^/  - /' "$rm_file"

if [ "$head_major" -gt "$base_major" ]; then
  echo "[abi-guard] major bump detected ($base_major -> $head_major): removals are allowed."
  echo "[abi-guard] Reviewer must confirm each removed symbol completed a >=2-minor deprecation window (deprecation-policy.md)."
  exit 0
fi

if [ "${ABI_GUARD_ALLOW_REMOVALS:-0}" = "1" ]; then
  echo "[abi-guard] removals acknowledged via ABI_GUARD_ALLOW_REMOVALS=1 (use only for a real major bump)."
  exit 0
fi

echo "[abi-guard] FAILURE: public symbols removed within major version $head_major."
echo "[abi-guard] Per deprecation-policy.md, removals are only allowed in a MAJOR bump after a >=2-minor deprecation window."
echo "[abi-guard] If this is intentional, bump CAPYUI_API_VERSION_MAJOR, or add the symbol to $IGNORE_FILE, or set ABI_GUARD_ALLOW_REMOVALS=1."
exit 1
