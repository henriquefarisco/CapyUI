# CapyUI compatibility and integration contract

CapyUI modules must remain portable retained UI model logic and must not call CapyOS compositor internals directly.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.240+20260519`
- Authoritative cross-repo matrix: `CapyOS/docs/reference/integration/compatibility-matrix.md`
- Canonical manifest format consumed by the in-tree adapter: `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- Manual deploy runbook: `CapyOS/docs/operations/manual-module-deploy-runbook.md`

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/capyui-widget-integration-contract.md`
- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`

Detailed CapyUI contracts (versioned by area) live in `docs/roadmap/contracts/`:

- `abi-versions.md` — historical ABI matrix.
- `widget-model.md`, `layout.md`, `display-list.md` — delivered (since 0.0.1/0.1/0.2).
- `focus.md`, `text-edit.md`, `animation.md`, `theme-tokens.md`, `adapter.md`, `compositor.md`, `text-metrics-hook.md`, `input.md`, `accessibility.md`, `shell.md`, `locale-rtl.md`, `theme-serialization.md`, `transforms.md`, `plugin-abi.md` — planned, with full specifications.
- `deprecation-policy.md` — formal policy that takes effect at 1.0.

Roadmap: see `docs/roadmap/README.md`. Current state: see `docs/roadmap/STATUS.md`.

## Owned ABI

CapyUI owns the `capy-ui-widget` ABI.

Current ABI version: `0.6` (additive).

This ABI covers:

- widget tree model;
- abstract events;
- retained state;
- style token contracts;
- layout primitives (since 0.1);
- display-list schema (since 0.2);
- focus traversal (since 0.3);
- text editing model (since 0.4);
- animation and timing (since 0.5);
- theme tokens v2 (since 0.6).

## Compatibility rules

- Widget model changes must be additive until the relevant CapyOS stage permits migration.
- Event names and payloads must be versioned once serialized or exposed across modules.
- Display-list output must be deterministic and independent from CapyOS surface pointers.
- CapyUI must not depend on fonts, framebuffer, compositor, input drivers or shell internals.
- Host adapters must convert CapyOS events/surfaces into CapyUI abstractions.

## Layout contract (since 0.1)

- Layout kinds: `CAPY_LAYOUT_NONE`, `CAPY_LAYOUT_STACK`, `CAPY_LAYOUT_GRID`, `CAPY_LAYOUT_FLOW`.
- `capy_widget_measure(root, avail_w, avail_h)` clamps root bounds by `min_*`/`max_*` constraints; `max_*` value of zero means unbounded.
- `capy_widget_arrange(root)` distributes children inside `root->bounds` minus padding, using `gap`, `axis`, `cols`, `grow`.
- Composition `measure` + `arrange` is deterministic and idempotent given the same tree and inputs.
- The `capy_layout_node` struct is embedded additively in `capy_widget` and must remain at the tail of the struct until the next ABI break window.

## Display-list schema (since 0.2)

- Schema version exposed via `CAPY_DISPLAY_LIST_SCHEMA_VERSION` (currently `2`); stored in `capy_display_list.version`.
- Ops: `CAPY_DL_NONE`, `CAPY_DL_RECT`, `CAPY_DL_BORDER`, `CAPY_DL_TEXT`, `CAPY_DL_CLIP_PUSH`, `CAPY_DL_CLIP_POP`, `CAPY_DL_IMAGE_REF`, `CAPY_DL_FOCUS_RING` (since 0.3).
- `capy_display_list` uses caller-provided command and text buffers; no internal allocation.
- Per-widget emit order is fixed: `CLIP_PUSH`, `RECT`, optional `BORDER`, optional `TEXT`, children (recursive), `CLIP_POP`. Invisible widgets emit nothing.
- `capy_widget_emit(root, dl)` is deterministic: same tree and bounds yield a byte-identical command sequence.
- On overflow `emit` returns `-1` and rolls back `count` and `text_used` to the pre-call values; commands beyond the rolled-back `count` are not observable.
- Text bytes are copied into `text_pool` without NUL terminators; consumers use `(text_offset, text_len)`.
- Op enum entries and `capy_dl_cmd` payload fields must be additive (append-only) until the next ABI break window.

## Focus traversal (since 0.3)

- Widget gains `focusable` (uint8_t) and `tab_index` (int16_t) fields, both additive at the tail of `capy_widget`.
- Default focusable types: `BUTTON`, `TEXTBOX`, `CHECKBOX`, `LIST`, `MENU_ITEM`, `TABS`. Other types default to non-focusable.
- Traversal order is deterministic lexicographic by `(tab_index, dfs_preorder_index)` over visible+enabled+focusable widgets; ties are broken by insertion order.
- `capy_widget_focus_next(root, current)` and `_prev(root, current)` wrap around when no candidate exists in the forward/backward direction.
- `capy_widget_dispatch_key(root, event)` consumes `KEY_DOWN` events with keycodes `CAPY_KEY_TAB`, `CAPY_KEY_ENTER`, `CAPY_KEY_SPACE`, `CAPY_KEY_ESCAPE` (arrows reserved for future use). `CAPY_MOD_SHIFT` reverses TAB direction.
- `capy_widget_event` gains an additive `uint32_t modifiers` field (since 0.3; expanded with WHEEL/TOUCH/GAMEPAD in 0.10).
- Display-list emits `CAPY_DL_FOCUS_RING` after `BORDER` and before `TEXT` when `widget->focused != 0`. Consumers below schema 2 must ignore the op.

## Text editing (since 0.4)

- `struct capy_text_edit` (additive, embedded at tail of `capy_widget`, valid when `type == TEXTBOX`) tracks `caret`, `sel_start`, `sel_end`, `multiline`, `readonly`, `password`.
- APIs: `capy_textbox_insert(w, utf8, len)`, `capy_textbox_delete(w, count)` (negative = backspace via UTF-8 prev-char), `capy_textbox_set_selection(w, start, end)`, `capy_textbox_copy(w, out, cap)`, `capy_widget_ime_compose(w, preedit, len)` (stub; real composition state lands in 1.8).
- Minimal UTF-8 validation rejects malformed leading bytes, truncated sequences, and invalid continuation bytes.
- Backspace and forward delete advance by UTF-8 characters, not raw bytes; multibyte characters preserved when not crossed.
- `readonly = 1` blocks `insert`/`delete` (returns `-1`) but `copy` still works.
- `password = 1` causes the display-list TEXT op to emit one `*` per UTF-8 character of `widget->text`. The original bytes are never written into the display-list text pool.
- Display-list emits a small caret RECT (1 px wide, `font_size` tall) after `TEXT` for TEXTBOX widgets that are `focused && !readonly && !password`. Caret horizontal position uses the deterministic fallback `caret * font_size / 2` until the v0.9 text metrics hook is wired.
- `capy_textbox_insert` with active selection (`sel_start != sel_end`) replaces the selected region before inserting.
- All mutations clamp `caret`/`sel_*` to the current text length to defend against externally tampered state.

## Animation and timing (since 0.5)

- Host provides `now` as a monotonic millisecond tick (`uint32_t`). CapyUI never reads a real clock.
- `struct capy_anim` (standalone, not embedded in `capy_widget` in 0.5) tracks `start_tick`, `duration_ticks`, `from`, `to`, `easing`, `active`.
- `enum capy_anim_easing` lists `CAPY_ANIM_LINEAR`, `CAPY_ANIM_EASE_IN`, `CAPY_ANIM_EASE_OUT`, `CAPY_ANIM_EASE_IN_OUT`.
- All easings implemented in **integer fixed-point 16.16** with `uint64_t` intermediates; **no `float`/`double` anywhere**.
- `capy_anim_start(a, now, duration, from, to, easing)` sets the animation and marks it active.
- `capy_anim_sample(a, now, &out)` returns `0` on success, `-1` on null args. Edge behaviour:
  - `!active` or `duration == 0` → `out = from`.
  - `now <= start_tick` (including rewound clock) → `out = from`.
  - `now ≥ start_tick + duration` → `out = to`.
  - In between, `out = from + (to - from) * easing(t) / 65536` with `t = elapsed * 65536 / duration`.
- `capy_widget_tick(root, now)` walks the tree deterministically. In 0.5 it performs no per-widget mutation; future slices will hook per-widget animations into the walk.
- Determinism guarantee: identical `(struct capy_anim, now)` inputs produce identical `int32_t` outputs across runs and platforms (no platform-dependent float rounding).
- Easings are monotonic in `[from, to]` (or `[to, from]`) and hit exact endpoints when `now == start_tick` and `now == start_tick + duration`.

## Theme tokens v2 (since 0.6)

- `enum capy_token` lists semantic colour roles: `CAPY_TOKEN_NONE = 0` (reserved for "use literal"), `CAPY_TOKEN_BG_BASE/RAISED/SUNKEN`, `CAPY_TOKEN_FG_PRIMARY/MUTED/INVERSE`, `CAPY_TOKEN_ACCENT/ACCENT_HOVER`, `CAPY_TOKEN_BORDER/BORDER_FOCUS`, `CAPY_TOKEN_FOCUS_RING`, `CAPY_TOKEN_DANGER/WARNING/SUCCESS/INFO`, `CAPY_TOKEN_DISABLED`. `CAPY_TOKEN_COUNT` is the upper bound.
- `struct capy_theme` (standalone) holds `tokens[CAPY_TOKEN_COUNT]`, `variant` (`LIGHT`/`DARK`/`HIGH_CONTRAST`), `high_contrast` flag, and `version`.
- Three built-in factories: `capy_theme_default_light()`, `capy_theme_default_dark()`, `capy_theme_high_contrast()`. High-contrast pair `BG_BASE = #000000` / `FG_PRIMARY = #FFFFFF` trivially exceeds WCAG AA (21:1 ratio).
- `capy_widget_context` gains an embedded `theme` field (additive). `capy_theme_apply(ctx, theme)` copies the theme into the context.
- `capy_widget_style` gains additive `bg_token`, `fg_token`, `border_token` (each `uint8_t`). Value `0` (`CAPY_TOKEN_NONE`) means "use literal `*_color`" — backward compatible.
- `capy_display_list` gains an optional `const struct capy_theme *theme` pointer (additive at tail; NULL by default after `capy_display_list_init`).
- Display-list emit resolves colours via `capy_theme_resolve(dl->theme, style.*_token, style.*_color)`:
  - NULL theme → fallback.
  - `token == 0` → fallback.
  - `token >= CAPY_TOKEN_COUNT` → fallback.
  - Otherwise → `theme->tokens[token]`.
- Op resolution: `RECT` uses `bg_token`/`bg_color`; `BORDER` uses `border_token`/`border_color`; `TEXT` and caret `RECT` use `fg_token`/`text_color`. `FOCUS_RING` keeps `active_color` literal in 0.6.
- Determinism: identical `(tree, theme)` inputs yield identical command sequences and colours; swapping themes yields the **same op sequence** with different colours.
- Pre-0.6 callers (no token use, no theme set) behave bitwise-identically to pre-0.6 emit output.

## Install/update boundary

CapyUI may be shipped as an optional UI-model component when the roadmap permits. CapyOS owns:

- compositor;
- windows;
- input device plumbing;
- fonts;
- drawing surfaces;
- theme provider;
- accessibility and shell integration;
- staging, activation and rollback.

## Dependency rules

UI components may depend on `capy-ui-widget` and CapyOS UI adapter ABIs. They must not depend on raw GUI structs from CapyOS source.

## Validation before CapyOS integration

Before CapyOS consumes a CapyUI release, externally validate:

- widget tree construction;
- event routing;
- focus traversal when available;
- display-list fixtures when available;
- deterministic error handling;
- no direct CapyOS runtime includes.

CapyUI model integration is gated by Etapas 4 and 6.

## Publishing as a Capy package (future, when the relevant stage opens)

When CapyUI is delivered as a remote module to the CapyOS
`services/capypkg` adapter, the publisher must follow
`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`.
The key requirements that affect CapyUI are:

- `payload_url` must be HTTPS only;
- `payload_sha256` must be lowercase 64 hex of the published artifact;
- `payload_size` ≤ 1 MiB during the alpha streaming-buffer window;
- `name` must follow `[a-zA-Z0-9._-]` (suggested `org.capyos.ui.widget-core`);
- `install_root` must live under `/var/capypkg` or `/opt/`;
- `signature_ed25519` must cover the canonical descriptor
  `name=N|version=V|payload_sha256=H|payload_url=U\n`;
- the CapyUI ABI version pinned in `docs/roadmap/contracts/abi-versions.md`
  must be mirrored in `required_abis[]` on the JSON index.

Until CapyAgent publishes its Ed25519 signer, CapyUI cannot be
installed from a `signed` repository in production.
