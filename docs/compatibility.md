# CapyUI compatibility and integration contract

CapyUI is the **authoritative owner of the CapyOS desktop session and
the portable retained widget model**. After `alpha.241`, CapyUI publishes
two installable Capy packages consumed by the CapyOS in-tree adapter
`services/capypkg`:

- `org.capyos.ui.widget-core` — portable retained widget model
  (ABI `capy-ui-widget` **v2.19**, aditivo sobre 2.18; fase 2.x major completa em 14/14 (2.0–2.13), 2.14–2.19 estendem a trilha de estado dos advanced widgets (date picker + color picker + table columns + autocomplete + tree hierarchy + chart dataset); 1.x permanece em LTS ≥12m).
- `org.capyos.ui.desktop-session` — desktop runtime, window manager
  and built-in apps (ABI `capy-ui-desktop-session` v1, depends on
  `widget-core`).

CapyUI modules must remain portable model + desktop session logic
and must not call CapyOS compositor internals directly.

## CapyOS reference version

- CapyOS core pinned for this contract: `0.8.0-alpha.261+20260529`.
- Authoritative cross-repo matrix: [`CapyOS/docs/reference/integration/compatibility-matrix.md`](../../CapyOS/docs/reference/integration/compatibility-matrix.md).
- Canonical manifest format consumed by the in-tree adapter: [`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md).
- Manual deploy runbook: [`CapyOS/docs/operations/manual-module-deploy-runbook.md`](../../CapyOS/docs/operations/manual-module-deploy-runbook.md).
- Current cross-repo audit: [`CapyOS/docs/reference/integration/compatibility-audit-2026-05-23.md`](../../CapyOS/docs/reference/integration/compatibility-audit-2026-05-23.md).

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/capyui-widget-integration-contract.md`
- `CapyOS/docs/reference/integration/external-core-repositories.md`

Detailed CapyUI contracts (versioned by area) live in `docs/roadmap/contracts/`:

- `abi-versions.md` — historical ABI matrix and pre/post-1.0 policy.
- `widget-model.md`, `layout.md`, `display-list.md` — delivered (since 0.0.1/0.1/0.2).
- `focus.md`, `text-edit.md`, `animation.md`, `theme-tokens.md`, `adapter.md`, `compositor.md`, `text-metrics-hook.md`, `input.md`, `accessibility.md`, `shell.md`, `locale-rtl.md`, `theme-serialization.md`, `transforms.md`, `plugin-abi.md` — planned with full specifications.
- `deprecation-policy.md` — formal policy that takes effect at 1.0.

Roadmap: see `docs/roadmap/README.md`. Current state: see `docs/roadmap/STATUS.md`.

## Owned ABIs

| ABI name | Version | Status | Module package | Adapter consumption |
|---|---|---|---|---|
| `capy-ui-widget` | **`v2.19` (additive over 2.18; fase 2.x major completa 14/14; advanced-widget state track open: date + color picker + table columns + autocomplete + tree hierarchy + chart dataset; 1.x em LTS ≥12m)** | Delivered | `org.capyos.ui.widget-core` | Linked statically by `org.capyos.ui.desktop-session` and any future CapyUI consumer. The 1.0 baseline (= union of pre-1.0 minors 0.0..0.15) was frozen; 1.x minors add fields/APIs without removing or renaming. Deprecation policy in `docs/roadmap/contracts/deprecation-policy.md`. |
| `capy-ui-desktop-session` | `v1` (delivered in `alpha.241`) | Delivered | `org.capyos.ui.desktop-session` | Consumed by CapyOS `kernel/module_gate.c` via marker `/var/capypkg/org.capyos.ui.desktop-session/installed`. When present, CapyOS activates desktop runtime; when absent, kernel keeps shell-only mode |

### `capy-ui-widget` v2.19 covers (additive over 2.18; fase 2.x major completa 14/14; 2.14–2.19 advanced-widget state: date + color picker + table columns + autocomplete + tree hierarchy + chart dataset; 1.x em LTS ≥12m)

- widget tree model;
- abstract events (POINTER/KEY since 0.0; WHEEL/TOUCH/GAMEPAD since 0.10);
- retained state;
- style token contracts;
- layout primitives (since 0.1);
- display-list schema v7 (since 0.2; `FOCUS_RING` since 0.3; `DIRTY_HINT` since 0.8; `font_id` in TEXT since 0.9; `DPI_SCOPE` since 1.5; `TRANSFORM_PUSH/POP` since 1.9; `PLUGIN_OP` since 2.0);
- focus traversal (since 0.3);
- text editing model (since 0.4);
- animation and timing (since 0.5);
- theme tokens v2 (since 0.6);
- compositor diff API `capy_widget_diff` (since 0.8);
- host text metrics hook + `font_id` propagation (since 0.9);
- input plumbing (WHEEL→LIST scroll, TOUCH→POINTER translation, GAMEPAD reserved channel, CAPSLOCK/NUMLOCK modifiers, typed payloads via `event->payload`) (since 0.10);
- accessibility tree snapshot (`capy_widget_a11y_snapshot`, role mapping for all 12 widget types, FNV-1a path-stable `widget_id`, 8 state flags) (since 0.11);
- locale + RTL (locale struct in context with BCP 47 tag and plural rule, RTL post-layout mirror, plural rule engine for EN/PT/AR/RU/OTHER, printf-subset format) (since 0.13);
- theme serialization (canonical text format `version=2`, `variant=...`, `high_contrast=0|1`, then each token as `name=0xAARRGGBB`; serialize + deserialize with whitespace/comment tolerance and unknown-key forward-compat) (since 0.14);
- pool allocator + measure cache (`capy_widget_pool` bump-allocator with 16-byte alignment, `capy_widget_allocator_from_pool` wrapper, `layout_dirty`/`layout_version` tail fields on every widget, short-circuit in `capy_widget_measure`, `capy_widget_invalidate` walks up parents) (since 0.15);
- ABI freeze markers: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` macros and `CAPYUI_API_DEPRECATED(msg)` attribute macro for future minor-deprecations (since 1.0; macros bumped to 1/1/0 in 1.1);
- damage tracking (since 1.1): `dirty_version` tail field on every widget, `capy_widget_invalidate_subtree`, `capy_widget_dirty_version`, `capy_widget_frame_budget`, `capy_display_list_diff_incremental` (positional diff with leading-prefix short-circuit);
- optional GPU translator (since 1.2): new header `capy_dl_gpu.h` + source `capy_dl_gpu.c`, `struct capy_gpu_quad`, `capy_dl_to_quads(dl, out, cap, out_count)` decomposes a `capy_display_list` into axis-aligned coloured/textured quads with an internal scissor stack (max depth `CAPY_DL_GPU_CLIP_STACK_MAX = 16`). DL schema remains 4;
- rich animation (since 1.3): `struct capy_anim_keyframe` + `struct capy_anim_track` (loop policy: 0=once, -1=infinite, N>0=repeat-then-clamp), `struct capy_spring` (Q8.8 stiffness/damping, Q24.8 velocity, plain-units position/target), `capy_anim_track_sample(track, now, &out)`, `capy_spring_step(s, dt_ms)` (1 ms symplectic-Euler substeps), `capy_anim_bezier_eval(p0, p1, p2, p3, t_q16)` (1D cubic via de Casteljau in Q16.16). Zero float; all signed shifts replaced by `/` and `*` to stay strictly-defined under C11;
- gesture engine (since 1.4): single-touch deterministic recognizer over the v0.10 TOUCH plumbing. Public surface: `enum capy_gesture_kind` (13 values including reserved PINCH_IN/OUT and ROTATE_CW/CCW), `struct capy_gesture`, `struct capy_gesture_recognizer` (caller-embedded, no malloc, depth-1 output queue), and 4 APIs `capy_gesture_recognizer_init`, `capy_gesture_feed(r, ev, now)`, `capy_gesture_tick(r, now)`, `capy_gesture_poll(r, &out)`. Recognizes TAP, DOUBLE_TAP, LONG_PRESS, SWIPE_LEFT/RIGHT/UP/DOWN, DRAG. Multi-touch (PINCH/ROTATE) deferred to a future minor; enum values reserved so future detection is ABI-compatible;
- multi-display + DPI scaling (since 1.5): `dpi_scale_x256` tail field on `capy_widget_context` and on `capy_display_list` (Q8.8, default `256` = 1.0×); APIs `capy_widget_set_dpi_scale`, `capy_widget_get_dpi_scale`, `capy_widget_dpi_scale_dim` (pure integer helper, saturating clamps to `INT32_MAX/MIN`). New display-list op `CAPY_DL_DPI_SCOPE` (DL schema **4 → 5**) prepended once by `capy_widget_emit` when `dl->dpi_scale_x256 != 256`; carries the scale on `image_id` and the scoped region on `rect`. Layout `measure`/`arrange` is **not** auto-scaled in 1.5 to preserve byte-equivalent output for pre-1.5 callers; auto-scale is reserved for a future additive minor;
- drag and drop (since 1.6): 4 new abstract event types (`DRAG_BEGIN`/`DRAG_MOVE`/`DRAG_END`/`DROP`), `struct capy_dnd_payload` (`type[32]` + caller-owned `data` ptr + `len`); 5 widget tail fields (`drag_payload`, `drop_accepted_types`/`drop_types_count`, `on_drop`, `drop_user_data`); APIs `capy_widget_set_draggable`, `capy_widget_set_drop_target`, `capy_widget_set_on_drop`; matchers `capy_dnd_type_matches` (case-insensitive `*` / `prefix/*` / exact) and `capy_widget_dnd_accepts`. DROP routed deepest-first in `capy_widget_handle_event`; DRAG_BEGIN/MOVE/END are no-ops in core (host dispatcher owns the preview/cancel cycle). DL schema unchanged;
- slab allocator (since 1.7): `struct capy_slab` over a caller-provided byte buffer, zero malloc, deterministic LIFO reuse via free-list inline in freed slots; APIs `capy_slab_init(s, buf, size, element_size)`, `capy_slab_alloc(s)`, `capy_slab_free(s, ptr)`. Companion to `capy_widget_pool` (since 0.15): pool for monotonically-growing arenas, slab for tight loops with frequent free+reuse. Degenerate inits (`element_size < sizeof(void *)`, NULL buffer or zero size) fail-closed — every subsequent `_alloc` returns `NULL`. Display-list compression and hash measure-cache (also listed in the slice plan) are reserved for a future additive minor; existing `capy_widget_diff` / `capy_display_list_diff_incremental` / `layout_dirty` already cover most of the use cases;
- 2D transforms (since 1.9): `struct capy_dl_transform` (2×3 affine in Q16.16 fixed-point) declared in `capy_display_list.h` plus new field `transform` on `capy_dl_cmd` (adds 24 bytes to sizeof). Widget tail field `transform` (caller-owned pointer; NULL = identity — no PUSH/POP emitted). 5 APIs: `capy_transform_identity`, `capy_transform_scale(sx_q16, sy_q16)`, `capy_transform_translate(tx, ty)`, `capy_transform_rotate_quadrants(n)` (exact 0°/90°/180°/270°), `capy_transform_multiply(a, b)` (Q16.16 matrix multiply with saturating clamp). 1 setter: `capy_widget_set_transform(w, t)`. 2 new display-list ops appended to `capy_dl_op`: `CAPY_DL_TRANSFORM_PUSH` (carries the matrix in `cmd->transform`) and `CAPY_DL_TRANSFORM_POP`. DL schema **5 → 6**. Arbitrary-angle rotation and hit-test compensation are deferred to a future minor; hosts that need them compute matrices externally and pass via `set_transform`. The GPU translator (`capy_dl_to_quads`, since 1.2) recognises both transform ops and skips them — the host backend feeds them into its own matrix stack;
- rich IME (since 1.8): `struct capy_ime_state` (preedit + candidate list + composition phase) plus 9 widget tail fields valid only when `type == CAPY_WIDGET_TEXTBOX`. 5 APIs: `capy_ime_set_preedit`, `capy_ime_set_candidates`, `capy_ime_commit` (inserts via `capy_textbox_insert` and clears state), `capy_ime_cancel` (clears without inserting), `capy_ime_get_state` (snapshot for candidate-window rendering). 4 event types appended to `capy_widget_event_type`: `IME_COMPOSE_START`/`PREEDIT_UPDATE`/`COMMIT`/`CANCEL` — pass-through in core (host dispatcher signals phase transitions). All APIs are no-ops on non-TEXTBOX widgets; CapyUI never allocates/copies/frees the host-owned strings or candidate array. The pre-1.8 stub `capy_widget_ime_compose` (0.4) is preserved for minimal callers. Candidate window rendering via a future `CAPY_DL_CANDIDATE_WINDOW` op is deferred to a follow-up minor;
- state serialization (since 1.10): new source `src/widget/capy_widget_serialize.c`. 3 APIs (`capy_widget_serialize`, `capy_widget_deserialize`, `capy_widget_serialize_text`) plus a canonical binary format declared in `capy_widget.h` (magic `"CUIS"`, format_version `1`, FNV-1a 32-bit checksum, little-endian throughout). 6 flag constants (`CAPY_STATE_FLAG_VISIBLE/ENABLED/FOCUSED/FOCUSABLE/CHECKED` and `CAPY_STATE_HEADER_SIZE`). Preserves widget type, bounds, visible/enabled/focused/focusable/checked flags, tab_index, text, and child-tree structure. Deferred to a future minor: text_edit state, animations, theme tokens, locale, 2D transform, DnD config, IME state, callbacks and host-owned resource handles (`font_id`/`image_id`/`user_data`). Foundation for crash recovery, hot-reload, devtools snapshot, undo/redo (v2.3) and time-travel debugging (v2.5);
- undo/redo per-context (since 2.3): `struct capy_undo_entry` (caller-owned `action_id`/`redo_data`/`undo_data` pointers + `redo_len`/`undo_len`/`timestamp_ms`). `struct capy_undo_stack` (caller-buffer-backed; `entries`/`capacity`/`count`/`redo_count`/`coalesce_window_ms`). Tail field `undo_stack` on `capy_widget_context` (NULL until `capy_undo_stack_init`). 6 APIs: `capy_undo_stack_init(ctx, buf, buf_size)`, `capy_undo_push(ctx, action_id, redo, rlen, undo, ulen)` (FIFO evicts oldest on full; branch-cuts the redo stack), `capy_undo_undo(ctx, *out)` / `capy_undo_redo(ctx, *out)` (out optional), `capy_undo_can_undo(ctx)` / `capy_undo_can_redo(ctx)`. Coalescing automatic + auto-hook of TEXTBOX/RICH_TEXT mutations are deferred to a follow-up additive minor; hosts implement coalescing externally before calling `_push`;
- virtualization data source (since 2.2, **plumbing only**): `typedef int (*capy_virtual_get_count_fn)(void *user_data)` + `typedef int (*capy_virtual_get_item_fn)(void *user_data, uint32_t index, char *out_text, uint16_t cap)`. `struct capy_virtual_data_source { get_count, get_item, user_data; }`. Tail field `virtual_source` on `capy_widget` (NULL = eager rendering, pre-2.2 behaviour). 3 APIs: `capy_widget_set_virtual_source(w, src)`, `capy_widget_virtual_count(w)` (`-1` on NULL source / fn), `capy_widget_virtual_get_item(w, index, out, cap)` (forwards to callback; `-1` on NULL source/out, cap=0). New event type `CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE` appended to `capy_widget_event_type` (host emits with `event->x = start_index`, `event->y = end_exclusive`; core is pass-through). **Viewport-only emit rewiring + row recycling deferred** to a follow-up fatia that touches `capy_widget_emit_recursive` — the plumbing is in place but the eager render path remains the default;
- advanced widgets (since 2.1, **enum-only**): 8 new entries appended to `enum capy_widget_type` — `CAPY_WIDGET_TREE`, `TABLE`, `RICH_TEXT`, `CANVAS`, `CHART`, `COLOR_PICKER`, `DATE_PICKER`, `AUTOCOMPLETE`. Each type integrates with the generic widget pipeline (layout, emit, focus, serialize, a11y) using the existing `capy_widget` fields. Per-widget state (tree hierarchy, table columns, rich-text ranges, canvas draw callback, chart datasets, pickers, autocomplete suggestions) and dedicated APIs are deferred to subsequent 2.x minors, each adding focused tail fields + setters for one widget family. `capy_widget_serialize` (since 1.10) accepts the new types: the deserialize type-byte ceiling moved from `CAPY_WIDGET_TABS` to `CAPY_WIDGET_AUTOCOMPLETE`. DL schema unchanged (the new widgets emit via the generic pipeline);
- plugin system + SDK (since 2.0): **major bump conservador**. New surface in `capy_widget.h`: `struct capy_plugin_descriptor` (id, version, `capy_ui_abi_required`, allocator, timeout, init/destroy/on_event/emit callbacks), `struct capy_plugin_context`, 2 APIs `capy_plugin_register(ctx, &desc)` and `capy_plugin_unregister_all(ctx)`, two macros `CAPYUI_API_VERSION_TAG` (`0x00020000`) and `CAPY_MAX_PLUGINS` (`8`), and two tail fields on `capy_widget_context` (`plugins[CAPY_MAX_PLUGINS]` + `plugin_count`). New display-list op `CAPY_DL_PLUGIN_OP` (DL schema **6 → 7**) carrying an opaque 32-byte payload (`rect`/`color`/`text_offset`/`text_len`/`border_width`/`font_size`/`font_id`/`image_id`); `image_id` is the canonical plugin-id channel. `capy_widget_emit` never emits the op — host-side plugin code does. The widget core never invokes plugin callbacks: the host dispatcher iterates the registry and applies its own sandbox (timeout / signal handling). SDK seed in `sdk/` (`README.md` + `plugin-template.c`). **No 1.x symbol removed in 2.0.0** — the deprecation policy is now ARMED for 2.x minors; 1.x continues in LTS for ≥12 months post-2.0.

All 14 pre-1.0 ABI minors (`0.0` through `0.15`, with `0.7` and `0.12` reserved), the 1.0 baseline, the 1.1–1.10 additions, the 2.0 plugin surface, the 2.1 advanced widget enums, the 2.2 virtualization plumbing and the 2.3 undo/redo stack are part of the live ABI. 2.0 is a major SemVer bump (DL schema 7, new plugin surface) but introduces **zero source-level removals or renames**; any host compiling against 1.10 compiles against 2.0 modulo the three literal schema asserts. No symbol is currently annotated deprecated; the macro is published so call sites can compile against it without conditional guards.

### `capy-ui-desktop-session` v1 covers

- desktop runtime (taskbar, launcher, system tray, desktop icons,
  wallpaper, notifications);
- window manager (dispatcher, focus, lifecycle, snap/maximize/minimize,
  decoration, drag/drop);
- built-in apps (calculator, file manager, settings, task manager,
  text editor, terminal frontend);
- shell context activation hooks consumed by `auth/login_runtime`.

The desktop session **does not** ship its own compositor, fonts,
framebuffer driver, raw input drivers, theme provider, accessibility
provider or session persistence. Those live in the CapyOS core.

## Compatibility rules

- Widget model and desktop session changes must be additive until the
  relevant CapyOS stage permits migration.
- Event names and payloads must be versioned once serialized or
  exposed across modules.
- Display-list output must be deterministic and independent from
  CapyOS surface pointers.
- CapyUI must not depend on fonts, framebuffer, compositor, input
  drivers or shell internals.
- Host adapters must convert CapyOS events/surfaces into CapyUI
  abstractions.
- The desktop session **must not** assume widget-core is statically
  linked when delivered as a separate Capy package; declare
  `depends=org.capyos.ui.widget-core` in the manifest of
  `org.capyos.ui.desktop-session`.

## Error model

| Code family | Trigger | Adapter behaviour |
|---|---|---|
| Widget tree malformed | `capy_widget_*` returns `-1` | UI app receives controlled error; desktop session continues |
| Layout overflow | `capy_widget_emit` returns `-1` and rolls back | Display list partial commit invisible; desktop continues |
| Theme token out of range | resolver falls back to literal `*_color` | Pre-0.6 behaviour preserved |
| Text input UTF-8 invalid | `capy_textbox_insert/delete` returns `-1`; readonly stays valid | App rejects input; no crash |
| Animation sample with rewound clock | `out = from` until next forward tick | Deterministic across runs |
| Desktop session not activated (gate missing) | `kernel_module_desktop_session_available()` returns 0 | CapyOS keeps shell-only mode; no GUI starts |
| Desktop runtime panic in app | dispatcher contains the failure | Desktop session continues; no kernel impact |

All errors must be deterministic. CapyUI never returns indeterminate
results, never panics the desktop on a single-app fault, and never
leaks raw pointers across the host ABI.

## Resource and performance limits

| Limit | Value | Owner |
|---|---|---|
| Maximum widget tree depth | bounded by caller; no internal recursion past `capy_widget_emit` budget | CapyUI |
| Display-list command buffer | caller-provided; emit rolls back on overflow | CapyUI |
| Display-list text pool | caller-provided; emit rolls back on overflow | CapyUI |
| Animation tick rate | host-provided monotonic `uint32_t` ms | CapyOS |
| Desktop session memory | bounded by `kalloc` quotas in CapyOS | CapyOS |
| Per-app failure containment | desktop session must isolate via dispatcher | CapyUI |
| Capy package payload | ≤ 1 MiB during the alpha streaming-buffer window; will rise to 8 MiB when CapyOS streaming writer lands | CapyOS adapter |

## Layout contract (since 0.1)

- Layout kinds: `CAPY_LAYOUT_NONE`, `CAPY_LAYOUT_STACK`, `CAPY_LAYOUT_GRID`, `CAPY_LAYOUT_FLOW`.
- `capy_widget_measure(root, avail_w, avail_h)` clamps root bounds by `min_*`/`max_*` constraints; `max_*` value of zero means unbounded.
- `capy_widget_arrange(root)` distributes children inside `root->bounds` minus padding, using `gap`, `axis`, `cols`, `grow`.
- Composition `measure` + `arrange` is deterministic and idempotent given the same tree and inputs.
- The `capy_layout_node` struct is embedded additively in `capy_widget` and must remain at the tail of the struct until the next ABI break window.

## Display-list schema (since 0.2)

- Schema version exposed via `CAPY_DISPLAY_LIST_SCHEMA_VERSION` (currently `4`); stored in `capy_display_list.version`.
- Ops: `CAPY_DL_NONE`, `CAPY_DL_RECT`, `CAPY_DL_BORDER`, `CAPY_DL_TEXT`, `CAPY_DL_CLIP_PUSH`, `CAPY_DL_CLIP_POP`, `CAPY_DL_IMAGE_REF`, `CAPY_DL_FOCUS_RING` (since 0.3), `CAPY_DL_DIRTY_HINT` (since 0.8 — optional inline hint, never produced by `capy_widget_emit`).
- `capy_dl_cmd` payload includes `uint16_t font_id` (since 0.9 — layout-compatible rename of the former `reserved` slot). `capy_widget_emit` copies `widget->font_id` into every `CAPY_DL_TEXT`. `0` is the default font.
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

## Advanced widget state — chart dataset (since 2.19)

- Sixth "advanced widget" family lifted into tail fields. Like the table/autocomplete it stores a **caller-owned** array (zero-alloc: the `const int32_t *` data is never copied/freed), but it adds a **numeric reduction**: `capy_widget_chart_range` does a single-pass signed min/max scan. Total / deterministic / zero-alloc / zero-float / fail-closed by widget type.
- Tail fields on `capy_widget`, meaningful only when `type == CAPY_WIDGET_CHART`: `const int32_t *chart_values` (caller-owned signed data points, must outlive the widget's use; NULL = no data) + `uint16_t chart_count` (0 = no data). `sizeof(struct capy_widget)` grows 864 → 880 bytes.
- 5 APIs:
  - `int capy_widget_set_chart_data(w, values, count)` — stores the dataset. `count == 0` clears (values ignored). `count > 0` requires non-NULL `values`. Returns `0`, or `-1` on NULL `w` / `type != CHART` / (`count > 0 && values == NULL`). Fail-closed: stored dataset unchanged on failure.
  - `int capy_widget_clear_chart_data(w)` — resets to NULL/0; `0`, or `-1` on NULL / wrong type.
  - `int capy_widget_chart_count(w)` — number of points (0 when none), or `-1` on NULL / wrong type.
  - `int capy_widget_chart_value(w, index, out_value)` — writes `values[index]` into `*out_value`; `0`, or `-1` on NULL `w` / wrong type / NULL `out_value` / no data / `index >= count`.
  - `int capy_widget_chart_range(w, out_min, out_max)` — single-pass signed min/max. Returns `1` with bounds written when data is present, `0` (and `*out_min = *out_max = 0`) when empty, `-1` on NULL `w` / wrong type / NULL `out_min` / NULL `out_max`.
- DL schema unchanged (`7`). `capy_widget_serialize` (1.10) does not serialize the dataset (caller-owned pointer).
- Deferred: multiple series, axes/labels/legend, chart kinds (line/bar/pie), display-list scaling/normalisation (desktop-session UX); remaining advanced-widget families.

## Advanced widget state — tree hierarchy (since 2.18)

- Fifth "advanced widget" family lifted into tail fields. The first whose read side **walks the widget hierarchy**: a tree row's visibility is derived from the fold state of its `TREE` ancestors. Total / deterministic / zero-alloc / zero-float / fail-closed by widget type.
- Tail fields on `capy_widget`, meaningful only when `type == CAPY_WIDGET_TREE`: `uint8_t tree_collapsed` (1 = children hidden; the create-time default of 0 means "expanded") + `uint16_t tree_depth` (caller-set indent level, 0 = root row — for flattened/virtualized tree models where visual nesting need not match the widget tree). `sizeof(struct capy_widget)` grows 856 → 864 bytes.
- 5 APIs:
  - `int capy_widget_set_tree_collapsed(w, collapsed)` — sets the fold flag (any nonzero `collapsed` stores 1). Returns `0`, or `-1` on NULL `w` / `type != TREE`.
  - `int capy_widget_tree_is_collapsed(w)` — `1` collapsed, `0` expanded, or `-1` on NULL / wrong type. A fresh node is expanded.
  - `int capy_widget_set_tree_depth(w, depth)` — sets the indent level; `0`, or `-1` on NULL / wrong type.
  - `int capy_widget_tree_depth(w)` — indent level (0 at root), or `-1` on NULL / wrong type.
  - `int capy_widget_tree_row_visible(w)` — walks the parent chain: returns `0` when any ancestor that is a `TREE` node is collapsed (the node's own collapse does not hide itself), `1` when none is, `-1` on NULL `w` / `type != TREE`. Deterministic; non-TREE ancestors are skipped.
- DL schema unchanged (`7`). `capy_widget_serialize` (1.10) does not serialize the fold state or depth (ephemeral UI state).
- Deferred: layout auto-indent by `tree_depth`, disclosure twisties in the display-list, row selection (desktop-session UX); remaining advanced-widget families.

## Advanced widget state — autocomplete suggestions (since 2.17)

- Fourth "advanced widget" family lifted into tail fields. Like the table it stores a **caller-owned** model (zero-alloc: the `const char *const *` array is never copied/freed — same idiom as `drop_accepted_types` and the IME candidate list), but it adds a **mutable selection cursor** with clamp / fail-closed semantics. Still total / deterministic / zero-alloc / zero-float / fail-closed by widget type.
- Tail fields on `capy_widget`, meaningful only when `type == CAPY_WIDGET_AUTOCOMPLETE`: `const char *const *autocomplete_items` (caller-owned, must outlive the widget's use; NULL = no list) + `uint16_t autocomplete_count` (0 = no list) + `int32_t autocomplete_selected` (highlighted index, -1 = none). The selection is **clamped against the live count on read**, so a stale selection never points past the list (and the zero-initialised default of a fresh widget reads as "none"). `sizeof(struct capy_widget)` grows 840 → 856 bytes.
- 6 APIs:
  - `int capy_widget_set_autocomplete(w, items, count)` — stores the list and resets the selection to -1. `count == 0` clears (items ignored). `count > 0` requires non-NULL `items`. Returns `0`, or `-1` on NULL `w` / `type != AUTOCOMPLETE` / (`count > 0 && items == NULL`). Fail-closed: stored model unchanged on failure.
  - `int capy_widget_clear_autocomplete(w)` — resets to NULL/0 and selection -1; `0`, or `-1` on NULL / wrong type.
  - `int capy_widget_autocomplete_count(w)` — number of suggestions (0 when none), or `-1` on NULL / wrong type.
  - `int capy_widget_autocomplete_item(w, index, out_item)` — writes `items[index]` into `*out_item`; `0`, or `-1` on NULL `w` / wrong type / NULL `out_item` / no list / `index >= count`.
  - `int capy_widget_set_autocomplete_selected(w, index)` — `index == -1` clears the selection; `0 <= index < count` selects. `0`, or `-1` on NULL `w` / wrong type / `index < -1` / `index >= count` (fail-closed).
  - `int capy_widget_get_autocomplete_selected(w, out_index)` — writes the live (clamped) selection (-1 when none) into `*out_index`; returns `1` when an entry is selected, `0` when none, `-1` on NULL `w` / wrong type / NULL `out_index`.
- DL schema unchanged (`7`). `capy_widget_serialize` (1.10) does not serialize the suggestion list (caller-owned strings) or the selection index.
- Deferred: incremental filtering/matching, prefix highlighting, display-list popup (desktop-session UX); remaining advanced-widget families.

## Advanced widget state — table columns (since 2.16)

- Third "advanced widget" family lifted into tail fields. Unlike the scalar date/color pickers, the `TABLE` stores a **caller-owned** column-width array plus a count, and the accessors are **bounds-checked / fail-closed**. Still total / deterministic / zero-alloc (CapyUI never copies/allocates/frees the array) / zero-float / fail-closed by widget type.
- Tail fields on `capy_widget`, meaningful only when `type == CAPY_WIDGET_TABLE`: `const uint16_t *table_column_widths` (caller-owned, must outlive the widget's use; NULL = no model) + `uint16_t table_column_count` (0 = no model). Widths are pixels. `sizeof(struct capy_widget)` grows 824 → 840 bytes (the pointer forces 8-byte alignment).
- 4 APIs:
  - `int capy_widget_set_table_columns(w, widths, count)` — stores the model. `count == 0` clears (widths ignored). `count > 0` requires non-NULL `widths`. Returns `0`, or `-1` on NULL `w` / `type != TABLE` / (`count > 0 && widths == NULL`). Fail-closed: stored model unchanged on failure.
  - `int capy_widget_clear_table_columns(w)` — resets to NULL/0; `0`, or `-1` on NULL / wrong type.
  - `int capy_widget_table_column_count(w)` — number of columns (0 when none), or `-1` on NULL / wrong type.
  - `int capy_widget_table_column_width(w, index, out_width)` — writes `widths[index]` into `*out_width`; `0`, or `-1` on NULL `w` / wrong type / NULL `out_width` / no model / `index >= count`.
- DL schema unchanged (`7`). `capy_widget_serialize` (1.10) does not serialize the column model (caller-owned pointers are not serializable).
- Deferred: column titles/alignment/sorting (desktop-session UX); remaining advanced-widget families.

## Advanced widget state — color picker (since 2.15)

- Second "advanced widget" family lifted from caller-owned `user_data` into a dedicated tail field; same additive shape as the date picker (total / deterministic / zero-alloc / zero-float / fail-closed by widget type).
- Tail fields on `capy_widget`, meaningful only when `type == CAPY_WIDGET_COLOR_PICKER`: `uint32_t picker_color` (0xAARRGGBB, same channel order as `capy_widget_style.*_color` and theme tokens) + `uint8_t picker_color_set` (presence flag; every 32-bit value is a valid colour so there is no unset sentinel). `sizeof(struct capy_widget)` grows 816 → 824 bytes.
- 4 APIs:
  - `uint32_t capy_color_pack(uint8_t r, uint8_t g, uint8_t b, uint8_t a)` — packs into 0xAARRGGBB; each channel cast to `uint32_t` before shifting (no signed-shift UB on the alpha byte). Inverse is `(argb >> 16) & 0xFFu` (R) / `>> 8` (G) / `& 0xFFu` (B) / `>> 24` (A).
  - `int capy_widget_set_color(w, argb)` — stores + marks set; `0`, or `-1` on NULL / `type != COLOR_PICKER`. No value validation.
  - `int capy_widget_clear_color(w)` — resets to colour 0 + flag 0; `0`, or `-1` on NULL / wrong type.
  - `int capy_widget_get_color(w, out)` — copies into `*out` (0 when unset); returns `1` set, `0` unset, `-1` on NULL / wrong type / NULL `out`.
- DL schema unchanged (`7`). `capy_widget_serialize` (1.10) does not serialize the colour fields.
- Deferred: remaining advanced-widget families; colour-space conversions (must stay integer/zero-float if added).

## Advanced widget state — date picker (since 2.14)

- First "advanced widget" family (enum-only since 2.1) lifted from caller-owned `user_data` into a dedicated first-class tail field. Establishes the additive pattern the remaining families (TABLE/TREE/COLOR_PICKER/CHART/AUTOCOMPLETE/RICH_TEXT/CANVAS) will follow, one focused minor each.
- `struct capy_date { uint16_t year; uint8_t month; uint8_t day; }` (any field `0` = unset).
- Tail field `struct capy_date date_value;` on `capy_widget`, meaningful only when `type == CAPY_WIDGET_DATE_PICKER`; zero-initialised by `capy_widget_create` to the unset sentinel. `sizeof(struct capy_widget)` grows 4 bytes (additive tail).
- 4 APIs, all total / deterministic / zero-alloc / zero-float:
  - `int capy_date_is_valid(uint16_t year, uint8_t month, uint8_t day)` — proleptic-Gregorian calendar predicate (month 1..12, day within month including Feb 29 on leap years `(y%4==0 && y%100!=0) || y%400==0`, year ≥ 1). Returns 1/0.
  - `int capy_widget_set_date(w, year, month, day)` — validates then stores; `0` on success, `-1` on NULL / `type != DATE_PICKER` / invalid date. Fail-closed: the stored value is unchanged on failure.
  - `int capy_widget_clear_date(w)` — resets to the unset sentinel; `0`, or `-1` on NULL / wrong type.
  - `int capy_widget_get_date(w, out)` — copies into `*out`; returns `1` when a valid date is set, `0` when unset, `-1` on NULL / wrong type / NULL `out`.
- DL schema unchanged (`7`): DATE_PICKER still emits via the generic pipeline; this slice only adds state + validation. `capy_widget_serialize` (1.10) does **not** serialize `date_value` (stays on the deferred-fields list); the binary format is unchanged.
- Deferred: remaining advanced-widget families; date formatting / locale (host / desktop-session via `capy_locale`); serializing `date_value` in widget state.

## Login screen (since 2.13)

- Plumbing primitives shared by the boot login surface, the lock screen and the user switcher. The widget core never authenticates (that lives in v2.7 `capy_user_directory`), never composes the background (v2.12 wallpaper), never resolves avatar bitmaps (v2.11 icon provider) — only the taxonomy + the deterministic layout decision so all three consumers stay in sync.
- 12 published macros:
  - 4 layout enums: `CAPY_LOGIN_LAYOUT_SINGLE` (=0), `_GRID` (=1), `_LIST` (=2), `_COUNT` (=3).
  - 2 thresholds: `CAPY_LOGIN_GRID_THRESHOLD` (=2), `CAPY_LOGIN_LIST_THRESHOLD` (=7).
  - 7 power enums: `CAPY_LOGIN_POWER_NONE/SHUTDOWN/REBOOT/SLEEP/LOCK/LOGOUT/COUNT` (= 0..6).
- 1 deterministic API: `uint8_t capy_login_choose_layout(uint32_t user_count)`.
  - `user_count ∈ {0, 1}` → `LAYOUT_SINGLE`. Zero users is the first-boot "create root account" prompt; one user is the direct password form.
  - `2 ≤ user_count ≤ 6` → `LAYOUT_GRID` (host draws a 2- to 3-column avatar grid).
  - `user_count ≥ 7` → `LAYOUT_LIST` (host shows a scrollable list with a search box on top).
- Total function: every `uint32_t` maps to a valid layout, no error return.
- Determinism: identical `user_count` always returns the same layout byte across runs and platforms.
- Power taxonomy is stable; hosts may omit any subset per policy (kiosk usually hides `SHUTDOWN` + `REBOOT`).
- Deferred: biometric / SSO challenge UI (`capy_login_challenge` enum + per-method input affordances); PIN keypad widget (desktop-session composite); locked-badge animation for `capy_user_account.is_locked`; resume-from-sleep wake screen (reuses `capy_login_choose_layout`).

## Wallpaper management (since 2.12)

- Plumbing primitives + canonical binary blob `CWLP` for the desktop session's wallpaper config. The widget core never composes pixels, never schedules the slideshow timer, never reads from disk — only the struct shapes and the round-trip serialization so the desktop session, the v2.13 login screen, the Settings → Personalization app and migration tooling share the same format.
- Blob layout (little-endian, header 24B + slides 8B):
  ```
  offset  size  field
  0       4     magic = "CWLP"
  4       2     format_version (uint16, currently 1)
  6       1     fit_mode (CAPY_WALLPAPER_FIT_*)
  7       1     flags    (CAPY_WALLPAPER_FLAG_*)
  8       4     FNV-1a 32-bit checksum over bytes [24, end)
  12      4     default_image_id (uint32; 0 = solid colour fallback)
  16      2     slideshow_interval_sec (uint16; 0 = no slideshow)
  18      2     slide_count (uint16)
  20      2     monitor_index (uint16; 0 = global, >= 1 per-monitor)
  22      2     reserved (must be 0)
  24+     ...   N slides of 8 bytes each (uint32 image_id >= 1; uint16 duration_sec >= 1; uint16 reserved == 0)
  ```
- 13 published macros: `CAPY_WALLPAPER_MAGIC0..3`, `_FORMAT_VERSION` (=1), `_HEADER_SIZE` (=24), `_ENTRY_SIZE` (=8); 5 fit modes `CAPY_WALLPAPER_FIT_STRETCH/FILL/FIT/CENTER/TILE` + `_COUNT` (=5); flags `CAPY_WALLPAPER_FLAG_RANDOM` (=0x01), `_PER_MONITOR` (=0x02).
- `struct capy_wallpaper_config { uint32_t default_image_id; uint16_t slideshow_interval_sec; uint16_t monitor_index; uint8_t fit_mode; uint8_t flags; uint16_t reserved; }`.
- `struct capy_wallpaper_slide { uint32_t image_id; uint16_t duration_sec; uint16_t reserved; }`.
- 3 APIs:
  - `int capy_wallpaper_validate(buf, len)` — checks magic, `format_version ≤ 1`, header + per-entry reserved zero, `fit_mode < FIT_COUNT`, `len == header + slide_count * entry_size`, FNV-1a body checksum, per-slide `image_id > 0` and `duration_sec > 0`.
  - `int capy_wallpaper_load(buf, len, *out_config, *out_slides, cap, *out_count)` — validates then writes the config plus copies slides up to `cap`. `*out_count` always carries declared count (callers detect short buffer via `*out_count > cap`).
  - `int capy_wallpaper_serialize(*config, *slides, count, *out, cap)` — writes a canonical blob; returns bytes written or `-1` on invalid args, missing slides, zero `image_id`/`duration_sec`, or insufficient `cap`. Byte-deterministic output.
- Deferred: live wallpaper / video (flag + callback); per-locale default factory; transition effects (cross-fade / slide / dissolve in the desktop animation policy); image format negotiation (decoding lives in the v2.11 icon provider).

## Icon & thumbnail system (since 2.11)

- Plumbing for icon resolution (sync) and async thumbnail generation. The widget core never decodes PNG/JPEG, never maintains an icon theme, never touches the disk. The host attaches a caller-owned `capy_icon_provider`; the widget core forwards calls and emits a `THUMBNAIL_READY` event when the host signals completion.
- 2 callback typedefs: `capy_icon_resolve_fn(user_data, mime_type, extension, *out_icon_id) → int`, `capy_icon_thumbnail_request_fn(user_data, path, *out_request_id) → int`.
- `struct capy_icon_provider { resolve, thumbnail_request, user_data }`. Tail field `icon_provider` em `capy_widget_context` (NULL = host-less; resolve falls back to `image_id = 0`, thumbnail_request returns `-1`).
- 1 event type aditivo: `CAPY_WIDGET_EVENT_THUMBNAIL_READY`. Payload: `event->x = (int32_t)request_id`, `event->y = (int32_t)image_id`. Dispatchers route the event to the widget that originally called `_thumbnail_request` (matching by `x`).
- 4 APIs:
  - `void capy_widget_set_icon_provider(ctx, provider)` — attach/clear. NULL ctx no-op.
  - `int capy_icon_resolve(ctx, mime_type, extension, *out_icon_id)` — forwards to `provider->resolve`. Deterministic fallback to `image_id = 0` when there is no provider, when the callback is NULL, or when the host returns `-1`. Returns `0` on success, `-1` only on NULL `ctx` or `out_icon_id`.
  - `int capy_icon_thumbnail_request(ctx, path, *out_request_id)` — forwards. Host writes a non-zero `*out_request_id`. Returns `0` on success, `-1` on NULL args, missing callback, host failure, or host returning `request_id = 0` (reserved).
  - `int capy_icon_thumbnail_ready(ctx, root, request_id, image_id)` — emits the event on `root` (when non-NULL). Rejects `request_id == 0`.
- `image_id = 0` is reserved for the deterministic default silhouette; hosts must assign `>= 1` for real icons.
- `request_id = 0` is reserved as the "unassigned" sentinel; hosts must not return it from `_thumbnail_request`.
- Determinism: identical inputs produce identical observable side effects (callback invocations, fallback image id, emitted event payload).
- Deferred: icon theme cascade with multiple providers chained (user theme → desktop default → fallback); vector (SVG/PDF) rendering pipeline; thumbnail request cancellation; stack/group folder icon descriptors (will land with the v2.9 group folders follow-up).

## File manager UX plumbing (since 2.10)

- Shared primitives + math for the file-manager app, save/open dialogs, and any future shell that needs toolbar grouping or breadcrumb truncation. The widget core never owns the file manager itself — that lives in `src/apps/` — but the taxonomy and the truncation algorithm are published here so multiple consumers stay consistent.
- 6 published macros:
  - `CAPY_FILE_MGR_COMPACT_THRESHOLD_PX` (=600) — viewport breakpoint for compact toolbar.
  - `CAPY_TOOLBAR_GROUP_NAV/VIEW/ACTION/LAYOUT` (= 0/1/2/3), `_COUNT` (=4) — stable taxonomy.
- `struct capy_breadcrumb_segment { uint32_t id; uint16_t text_offset; uint16_t text_len; uint16_t icon_image_id; uint8_t is_clickable; uint8_t reserved; }`.
- `int capy_breadcrumb_truncate(in, in_count, available_width_px, segment_avg_px, out, cap, *out_count, *out_dropdown)` — deterministic, zero-float, zero-alloc. Algorithm:
  1. `in_count == 0` → `*out_count = 0`, `*out_dropdown = 0`.
  2. `in_count * avg <= available_width` → copy all, `*out_dropdown = 0`. Fails when `cap < in_count`.
  3. Otherwise: `max_inline = max(2, available_width / avg)`, clamped to `min(cap, in_count)`. `out[0]` = root, `out[1..N-1]` = tail of `in[]`, `*out_dropdown = 1`. Hosts render the `…` between `out[0]` and `out[1]`.
  - Returns `-1` on NULL `in`/`out`/`out_count`/`out_dropdown` (when relevant), `cap == 0` with non-zero input, or `segment_avg_px == 0`.
- Determinism: identical inputs produce identical `(out[], out_count, out_dropdown)` byte-for-byte.
- Deferred: variable-width per-segment measurement via host callback; multi-line breadcrumb wrapping; locale-aware ellipsis (RTL leading `…` will piggyback on the v0.13 locale surface); per-item toolbar descriptors (icon + label + tooltip + shortcut) stay in the desktop session.

## Desktop icon arrangement (since 2.9)

- Plumbing primitives + canonical binary blob `CDLA` for the desktop session's icon layout. The widget core owns the struct shapes and the round-trip serialization; the desktop session in `src/desktop/` drives the interactive drag and the actual disk I/O.
- Blob layout (little-endian, header 24B + entries 16B):
  ```
  offset  size  field
  0       4     magic = "CDLA"
  4       2     format_version (uint16, currently 1)
  6       1     snap (0 or 1)
  7       1     auto_arrange (0 or 1)
  8       4     FNV-1a 32-bit checksum over bytes [24, end)
  12      2     cell_w (uint16, > 0)
  14      2     cell_h (uint16, > 0)
  16      1     sort_by (CAPY_DESKTOP_SORT_*)
  17      1     flags  (CAPY_DESKTOP_LAYOUT_FLAG_*)
  18      2     entry_count (uint16)
  20      4     reserved (must be 0)
  24+     ...   N entries of 16 bytes (uint32 icon_id; int16 x, y, grid_x, grid_y; uint8 pinned; uint8 reserved[3])
  ```
- 14 published macros: `CAPY_DESKTOP_LAYOUT_MAGIC0..3`, `_FORMAT_VERSION` (=1), `_HEADER_SIZE` (=24), `_ENTRY_SIZE` (=16); `CAPY_DESKTOP_SORT_MANUAL/NAME/TYPE/MTIME/SIZE` + `_COUNT` (=5); `CAPY_DESKTOP_LAYOUT_FLAG_ALIGN_RIGHT` (=0x01), `_VERTICAL_FIRST` (=0x02).
- `struct capy_desktop_layout { uint16_t cell_w, cell_h; uint8_t snap, auto_arrange, sort_by, flags; }`.
- `struct capy_desktop_icon_position { uint32_t icon_id; int16_t x, y, grid_x, grid_y; uint8_t pinned; uint8_t reserved[3]; }`.
- 3 APIs:
  - `int capy_desktop_layout_validate(buf, len)` — checks magic, `format_version ≤ 1`, reserved zero (header + per-entry), `len == header + entry_count · entry_size`, FNV-1a body checksum, `cell_w > 0 && cell_h > 0`, `snap/auto_arrange ≤ 1`, `sort_by < CAPY_DESKTOP_SORT_COUNT`.
  - `int capy_desktop_layout_load(buf, len, *out_layout, *out_positions, cap, *out_count)` — validates then writes config + copies entries (capped by `cap`). `*out_count` always carries the declared entry count, so callers detect a short buffer by `*out_count > cap`.
  - `int capy_desktop_layout_serialize(*layout, *positions, count, *out, cap)` — writes a canonical blob; returns bytes written or `-1` on invalid layout / NULL args / cap too small.
- Determinism: identical `(layout, positions[])` inputs produce identical blobs byte-for-byte across runs and platforms.
- Deferred: multi-monitor envelope (one blob holds one monitor today); group folders / icon stacks (v2.11 icon system); locale-aware `SORT_NAME` collation (v0.13 locale surface); drag hit-test in the widget core (desktop session owns the loop).

## Contrast & accessibility refinement (since 2.8)

- Quantitative contrast metrics over the colour tokens. Zero-float, deterministic integer arithmetic.
- Calculation: sRGB byte → Q16.16 linear via `lin = c * c * 65536 / 65025` (γ ≈ 2.0 approximation); weighted luminance `L = (2126*R + 7152*G + 722*B) / 10000`; contrast ratio `(L_max + 3277) * 1000 / (L_min + 3277)` (offset `0.05 ≈ 3277`); clamped `[1000, 21000]`.
- Differs slightly from the exact WCAG-2.1 transform (`((c + 0.055)/1.055)^2.4`) above the 0.03928 knee, but both agree on AA/AAA pass/fail for the canonical token pairs.
- 3 published macros: `CAPY_CONTRAST_AA_X1000` (=4500), `CAPY_CONTRAST_AAA_X1000` (=7000), `CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST` (=3).
- `struct capy_contrast_finding { uint8_t fg_token; uint8_t bg_token; uint16_t ratio_x1000; uint8_t passes_aa; uint8_t passes_aaa; uint8_t reserved[2]; }`.
- 3 APIs:
  - `uint32_t capy_theme_contrast_ratio_x1000(fg, bg)` — alpha is ignored; returns `1000` for matching colours, `21000` for white/black.
  - `int capy_theme_audit_wcag(theme, out, cap)` — walks 11 canonical pairs (FG_PRIMARY/BG_BASE, FG_PRIMARY/BG_RAISED, FG_MUTED/BG_BASE, FG_INVERSE/ACCENT, BORDER_FOCUS/BG_BASE, FOCUS_RING/BG_BASE, DANGER/BG_BASE, WARNING/BG_BASE, SUCCESS/BG_BASE, INFO/BG_BASE, DISABLED/BG_BASE). `cap == 0` is a dry-run returning the total pair count so callers can size their buffer with a single call. `-1` on NULL theme or `cap > 0 && out == NULL`.
  - `struct capy_theme capy_theme_default_dark_high_contrast(void)` — fourth built-in factory. Dark background, bright foregrounds chosen so every canonical pair lands at ≥ 7:1 (AAA).
- Determinism: identical inputs produce identical `ratio_x1000` byte-for-byte across runs and platforms.
- Deferred: WCAG 3 APCA perceptual contrast; large-text AA threshold (3.0) with per-pair size metadata; per-locale palette overrides; auto-fix suggestions for failing pairs.

## User management UI (since 2.7)

- Plumbing for user account management. CapyUI never authenticates, never hashes a password, never touches `/etc/passwd`-style backing stores. The host registers five callbacks via a caller-owned `capy_user_directory` and the widget core just routes calls.
- `struct capy_user_account { char username[CAPY_USER_NAME_MAX=32]; char display_name[CAPY_USER_DISPLAY_NAME_MAX=64]; uint32_t uid; uint8_t is_admin; uint8_t is_locked; uint16_t avatar_image_id; uint32_t last_login_ms; }`. `uid == 0` is reserved for root.
- 5 callback typedefs: `capy_user_list_fn`, `capy_user_create_fn` (password passes through verbatim), `capy_user_update_fn`, `capy_user_delete_fn`, `capy_user_set_avatar_fn`.
- `struct capy_user_directory { list, create, update, del, set_avatar, user_data }` is caller-owned. Attach via `capy_widget_set_user_directory(ctx, dir)` (NULL clears).
- 6 APIs:
  - `void capy_widget_set_user_directory(ctx, dir)` — attach/clear. NULL ctx no-op.
  - `int capy_user_list(ctx, out, cap)` — forwards. `-1` on NULL ctx, missing callback, or `cap > 0 && out == NULL`.
  - `int capy_user_create(ctx, account, password)` — forwards; rejects empty `username` before reaching host. CapyUI never stores/hashes the password.
  - `int capy_user_update(ctx, account)` — forwards. Host looks up by `uid`.
  - `int capy_user_delete(ctx, uid)` — forwards. Rejects `uid == 0` (root) before reaching host.
  - `int capy_user_set_avatar(ctx, uid, png, len)` — forwards. Rejects `len > 0 && png == NULL`; `len == 0 && png == NULL` clears the avatar.
- Determinism: identical `(directory, sequence)` inputs produce identical observable side effects (callback invocations + return codes).
- Deferred: authentication (`capy_user_authenticate_fn` for v2.13 login screen, password + biometric + SSO); groups/roles ACLs beyond the binary `is_admin`; full audit log (last_login_ms is exposed; deeper history lives outside the widget core).

## Display mode (since 2.6)

- Plumbing for runtime resolution + refresh-rate selection. CapyUI never touches the framebuffer, kernel mode-setting, or video-card registers; the host registers two callbacks via a caller-owned `capy_display_controller` and the widget core just routes calls + emits a notification event.
- `struct capy_display_mode { uint16_t width, height, refresh_hz_q8; uint8_t bpp, flags; }`. `refresh_hz_q8` is Q8.0 today (60, 144, 240). `flags` is a `CAPY_DISPLAY_MODE_FLAG_*` bitmask: `PREFERRED` (host-marked default), `INTERLACED` (legacy CRT).
- Callbacks: `capy_display_enum_modes_fn(user_data, out, cap) → int` (modes written), `capy_display_set_mode_fn(user_data, *mode) → int` (0 ok, -1 host-rejected).
- `struct capy_display_controller { enum_modes, set_mode, user_data, current_mode, has_current, reserved[3] }` is caller-owned and attached to a context via `capy_widget_set_display_controller(ctx, controller)` (NULL clears).
- Event type aditivo `CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED` (appended ao tail do enum). Payload: `event->x = (width << 16) | height`, `event->y = (bpp << 16) | refresh_hz_q8`.
- 4 APIs:
  - `int capy_widget_set_display_controller(ctx, controller)` — attach/clear. NULL ctx is a no-op.
  - `int capy_display_enum_modes(ctx, out, cap)` — forwards to `controller->enum_modes`. Returns modes written; `-1` on NULL ctx, NULL controller/callback, or `cap > 0 && out == NULL`.
  - `int capy_display_set_mode(ctx, root, mode)` — validates `width > 0 && height > 0`, forwards to `controller->set_mode`. On success: caches `*mode` into `controller->current_mode`, sets `has_current = 1`, emits `DISPLAY_MODE_CHANGED` on `root` (if non-NULL). `-1` on NULL/missing callback, zero dims, or host-rejected mode.
  - `int capy_display_current_mode(ctx, out)` — copies cached `current_mode` when `has_current != 0`; `-1` otherwise.
- Determinism: identical `(controller, mode)` inputs produce identical observable side effects (callback invocation, cached state, emitted event payload).
- Deferred: fractional refresh (Q24.8 + fractional bit for 23.976 / 59.94 / 119.88 Hz / G-SYNC); HDR + VRR metadata in `flags` reserved bits; rollback / 15-second preview UI (lives in the desktop session); per-user persistence via `capy_widget_serialize` (1.10) envelope.

## Devtools / inspector (since 2.5)

- Read-only introspection over a retained widget tree and a widget context. Zero allocation; both APIs write into caller-provided buffers and never mutate the inspected tree, context, or theme.
- `struct capy_inspector_node { uint32_t id; uint16_t type; uint16_t depth; uint32_t parent_index; uint32_t child_count; int32_t x, y, w, h; uint32_t flags; int16_t tab_index; uint16_t font_id; uint16_t text_offset; uint16_t text_len; uint32_t layout_version; uint32_t dirty_version; }`.
- `struct capy_perf_counters { uint32_t widget_count; uint32_t plugin_count; uint32_t undo_count; uint32_t undo_redo_count; uint32_t undo_capacity; uint16_t dpi_scale_x256; uint16_t reserved_dpi; uint32_t frame_budget_microseconds; uint16_t theme_variant; uint8_t theme_high_contrast; uint8_t reserved[1]; }`.
- 12 published macros: `CAPY_INSPECTOR_NO_PARENT` (= 0xFFFFFFFFu) plus 11 flag bits (`VISIBLE`, `ENABLED`, `FOCUSED`, `FOCUSABLE`, `CHECKED`, `HOVERED`, `LAYOUT_DIRTY`, `HAS_TRANSFORM`, `HAS_VIRTUAL_SRC`, `HAS_DRAG_PAYLOAD`, `HAS_DROP_TARGET`).
- 2 APIs:
  - `int capy_widget_inspect(root, out_nodes, cap, out_text_pool, text_cap, out_node_count, out_text_used)` — depth-first pre-order flatten mirroring `capy_widget_emit_recursive`. Text bytes copied without NUL into the caller's text pool, indexed by `(text_offset, text_len)`. Overflow rolls back both `*out_node_count` and `*out_text_used` to `0` and returns `-1`. NULL root/out_nodes/out_node_count/out_text_used → `-1`; `text_cap > 0` with `out_text_pool == NULL` → `-1`.
  - `int capy_perf_counters_snapshot(ctx, root, out)` — populates the snapshot from `ctx` (plugin_count, undo_stack, dpi_scale_x256, frame_budget_microseconds, theme.variant, theme.high_contrast) plus a recursive widget count from `root`. NULL `root` is tolerated (`widget_count = 0`). NULL `ctx`/`out` → `-1`.
- Determinism: same `(root, ctx)` → same flatten + same snapshot byte-for-byte. Read-only over the tree.
- Deferred for follow-up additive minors: textual `capy_devtools_dump_tree`/`dump_display_list` canonical formats; event recording / replay ring (caller-buffer-backed `(timestamp_ms, capy_widget_event)` records + `_replay`); headless harness CLI tooling (lives outside the portable core); IDE wire protocol.

## Theme packs (since 2.4)

- Binary canonical little-endian format for offline distribution of theme tokens. Layout:
  ```
  offset  size  field
  0       4     magic = "CTHM"
  4       2     format_version (uint16, currently 1)
  6       1     variant (CAPY_THEME_VARIANT_*; rejected if >= 16)
  7       1     high_contrast flag (0 or 1)
  8       4     FNV-1a 32-bit checksum over bytes [16, end)
  12      2     entry_count (uint16)
  14      2     reserved (must be 0)
  16+     ...   N entries of 6 bytes each:
                  uint8  token_id (>=1, <CAPY_TOKEN_COUNT)
                  uint8  reserved (must be 0)
                  uint32 colour_le (0xAARRGGBB little-endian)
  ```
- 2 APIs:
  - `int capy_theme_pack_validate(const void *buf, uint32_t len)` — checks magic, format_version ≤ `CAPY_THEME_PACK_FORMAT_VERSION`, reserved zero, declared length matches `len`, FNV-1a body checksum matches. Returns `0` on success, `-1` otherwise (also `-1` on NULL/too-short buffer).
  - `int capy_theme_pack_load(const void *buf, uint32_t len, struct capy_theme *out)` — validates then applies the pack to `*out`. Tokens absent from the pack stay untouched (callers seed with `capy_theme_default_light/dark/high_contrast` first). `out->variant` and `out->high_contrast` are overwritten; `out->version` is preserved for caller-managed per-instance versioning. Returns `0` on success, `-1` on validation failure or NULL `out`.
- 7 published macros: `CAPY_THEME_PACK_MAGIC0..3`, `CAPY_THEME_PACK_FORMAT_VERSION` (=1), `CAPY_THEME_PACK_HEADER_SIZE` (=16), `CAPY_THEME_PACK_ENTRY_SIZE` (=6).
- Forward-compat: entries with `token_id == 0` or `token_id >= CAPY_TOKEN_COUNT` are silently skipped during load, so packs authored against a newer token schema still load on older hosts.
- Determinism: same `buf`/`len` → same `*out`. Endianness explicit. Zero allocation in core; pack stays in caller buffer.
- Deferred: hot reload (host observes file and re-calls `_load`); Ed25519 signature (host validates externally; a future `format_version = 2` may append a trailing signature block); font / icon refs (live on the v2.11 icon descriptor surface); textual metadata (name/author/locale) carried by host envelope, not by the binary pack.
- Coexists with v0.14 `capy_theme_serialize` textual key=value (diff-friendly for git); the 2.4 binary pack is the distribution format.

## Undo/redo per-context (since 2.3)

- `struct capy_undo_entry { const char *action_id; const void *redo_data; uint32_t redo_len; const void *undo_data; uint32_t undo_len; uint32_t timestamp_ms; }`. All pointers caller-owned; widget core never copies or interprets the bytes.
- `struct capy_undo_stack { struct capy_undo_entry *entries; uint32_t capacity; uint32_t count; uint32_t redo_count; uint32_t coalesce_window_ms; }`. Lives at offset 0 of the caller-provided buffer; `entries` points immediately past the header.
- Tail field `struct capy_undo_stack *undo_stack` on `capy_widget_context`. NULL until `_init`; pre-2.3 ctxs zero-init the field by default.
- 6 APIs:
  - `int capy_undo_stack_init(ctx, buf, buf_size)` — partitions `buf`, computes capacity, attaches to `ctx->undo_stack`. Returns `0` on success, `-1` on NULL args / too-small buffer.
  - `int capy_undo_push(ctx, action_id, redo, rlen, undo, ulen)` — appends an entry on top, discards pending redo entries (branch cut), FIFO-evicts the oldest entry when full.
  - `int capy_undo_undo(ctx, *out)` — pops top to redo stack; copies the entry into `*out` (optional). Returns `0` / `-1` on empty.
  - `int capy_undo_redo(ctx, *out)` — pops redo back to top; copies entry into `*out`. Returns `0` / `-1` on empty.
  - `int capy_undo_can_undo(ctx)` / `int capy_undo_can_redo(ctx)` — `1` if non-empty respective stack, `0` otherwise (also `0` for NULL/no-init).
- Deterministic: identical buffer + same sequence of `(push/undo/redo)` over two contexts produce identical `(count, redo_count, top.action_id)`.
- Deferred: automatic coalescing inside `coalesce_window_ms` (default 500 ms; field is informational only in 2.3); TEXTBOX / RICH_TEXT auto-registering of insert/delete actions (hosts call `_push` themselves today).

## Virtualization data source (since 2.2)

- `typedef int (*capy_virtual_get_count_fn)(void *user_data)` and `typedef int (*capy_virtual_get_item_fn)(void *user_data, uint32_t index, char *out_text, uint16_t cap)`. Both caller-owned.
- `struct capy_virtual_data_source { get_count, get_item, user_data; }`. Stored verbatim on the widget; widget core never copies or validates beyond NULL checks.
- Tail field `const struct capy_virtual_data_source *virtual_source` on `capy_widget`. NULL = eager rendering (pre-2.2 behaviour).
- `capy_widget_set_virtual_source(w, src)` — stores the pointer; NULL clears; NULL widget is a no-op.
- `capy_widget_virtual_count(w)` — forwards to `src->get_count(user_data)`. Returns `-1` on NULL widget / NULL source / NULL `get_count`.
- `capy_widget_virtual_get_item(w, index, out_text, cap)` — forwards to `src->get_item(...)`. Returns `-1` on NULL widget / NULL source / NULL `get_item` / NULL `out_text` / `cap == 0`; otherwise returns the callback's result (0 on success).
- Event `CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE` appended to `capy_widget_event_type`. The host renderer emits this when a virtualized LIST/TREE/TABLE needs to prefetch the `[event->x, event->y)` index range; `capy_widget_handle_event` returns `0` (pass-through). Hosts listen via their own dispatcher loop.
- **Viewport-only emit + row recycling deferred.** The plumbing is in place; the actual rewiring of `capy_widget_emit_recursive` to consult the data source instead of `children[]` lives in a follow-up fatia together with the recycle pool. Existing widget trees continue to render eagerly with byte-equivalent output to pre-2.2.
- Determinism: every call goes straight through to the caller-provided function pointers. No caching, no allocation in the widget core.

## Advanced widget types (since 2.1)

- 8 new entries appended to `enum capy_widget_type`: `CAPY_WIDGET_TREE`, `CAPY_WIDGET_TABLE`, `CAPY_WIDGET_RICH_TEXT`, `CAPY_WIDGET_CANVAS`, `CAPY_WIDGET_CHART`, `CAPY_WIDGET_COLOR_PICKER`, `CAPY_WIDGET_DATE_PICKER`, `CAPY_WIDGET_AUTOCOMPLETE`. All slot reservations; per-widget state and dedicated APIs are deferred to subsequent 2.x minors.
- Each new type is created with `capy_widget_create(ctx, type)` and exercises the generic pipeline: bounds, text, visible/enabled flags, focus traversal, display-list emit, accessibility snapshot, state serialization. Hosts that need richer semantics today should attach their per-widget data via the existing `user_data` slot until the dedicated minor lands.
- `capy_widget_serialize` / `capy_widget_deserialize` (since 1.10): the deserialize type-byte ceiling moved from `CAPY_WIDGET_TABS` to `CAPY_WIDGET_AUTOCOMPLETE`; bytes above the ceiling are still rejected fail-closed.
- Display-list schema remains **7** — the new widgets emit via the generic CLIP_PUSH/RECT/...CLIP_POP sequence; no new ops are required.
- Deferred (each becomes its own 2.x minor): TREE hierarchy + lazy load, TABLE columns/sort/selection, RICH_TEXT ranges, CANVAS draw callback, CHART datasets, COLOR_PICKER HSV/hex/recent, DATE_PICKER calendar grid, AUTOCOMPLETE suggestion callback, and role mapping for `capy_widget_a11y_snapshot` (currently the new types fall through to the default role).

## Plugin system (since 2.0)

- Major bump 1.10 → 2.0 is required by SemVer because (a) a new display-list op `CAPY_DL_PLUGIN_OP` appears in schema 7 and may surprise consumers that fail-closed on unknown ops, and (b) the major bump opens the deprecation-policy door for future 2.x minors to remove APIs after ≥2-minor windows. **No actual API removal happens in 2.0.0** — the bump is conservative.
- `struct capy_plugin_descriptor { const char *id; const char *version; uint32_t capy_ui_abi_required; struct capy_widget_allocator allocator; uint32_t timeout_microseconds; init/destroy/on_event/emit callbacks; }`. `capy_ui_abi_required` encodes `(major << 16) | (minor << 8) | patch`; the host rejects descriptors with a value greater than `CAPYUI_API_VERSION_TAG`.
- `struct capy_plugin_context { const capy_plugin_descriptor *descriptor; void *user_data; }`. Caller-managed; the widget core never looks inside.
- `int capy_plugin_register(ctx, &desc)` validates (non-NULL ctx and desc, `capy_ui_abi_required <= CAPYUI_API_VERSION_TAG`, descriptor not already registered, `plugin_count < CAPY_MAX_PLUGINS`) and stores the pointer in `ctx->plugins[ctx->plugin_count++]`. Returns the slot index on success, `-1` on validation/capacity failure.
- `void capy_plugin_unregister_all(ctx)` invokes each non-NULL `destroy` callback once and resets `ctx->plugin_count = 0`. Idempotent. NULL-safe on `ctx`.
- `CAPYUI_API_VERSION_TAG` macro lets plugins compile-time gate their required host version. `CAPY_MAX_PLUGINS` (`8`) is the static per-context registry capacity; hosts that need more fork the constant.
- Sandbox: the widget core **never invokes** plugin callbacks. The host dispatcher iterates `ctx->plugins[0..plugin_count)` and applies its own timeout / signal-handling discipline. Plugin crashes never reach the core because the core does not call into the callbacks. The descriptor's `allocator` pointer is stored separately from the host context's `allocator` so audit tools can confirm the plugin uses its own arena.
- Display-list op `CAPY_DL_PLUGIN_OP` (schema 7) carries an opaque 32-byte payload reusing the existing `capy_dl_cmd` fields. `image_id` is the canonical plugin-id channel. `capy_widget_emit` never produces this op; host-side plugin code appends it via the descriptor's `emit` callback (invoked by the host dispatcher).
- GPU translator (`capy_dl_to_quads`, since 1.2) recognises `CAPY_DL_PLUGIN_OP` and skips it (no quad emitted) so unprepared backends degrade gracefully.
- SDK seed in `sdk/`: `README.md` (curator note + plugin authoring overview) and `plugin-template.c` (compilable no-op descriptor). Future iterations may copy a curated header subset under `sdk/include/capyui/` for plugin pin-points.
- LTS commitment: 1.x continues to receive security and bug-fix patches for **≥12 months post-2.0** per `docs/roadmap/contracts/deprecation-policy.md`.

## State serialization (since 1.10)

- New source `src/widget/capy_widget_serialize.c` added to `SRC_WIDGET` in the Makefile (no new public header — the APIs live in `capy_widget.h`).
- 3 public APIs:
  - `int capy_widget_serialize(root, out, cap, &out_len)` writes the canonical binary blob; returns `0` on success with `*out_len` set, `-1` on NULL args / capacity overflow / count failure (with `*out_len` reflecting the portion written before failure when applicable).
  - `int capy_widget_deserialize(buf, len, ctx, &out_root)` validates magic, version, reserved, checksum and node_count; rebuilds the tree using `ctx->allocator`. Returns `0` on success, `-1` on any validation/allocation failure (all partial allocations destroyed before returning).
  - `int capy_widget_serialize_text(root, out, cap)` writes a debug-friendly indented dump (`[type=N bounds=(x,y,w,h) flags=ve text="..." children=K]`). Returns the byte length written (excluding NUL) on success, `-1` on overflow / NULL.
- Public constants in `capy_widget.h`: `CAPY_STATE_MAGIC0..3`, `CAPY_STATE_FORMAT_VERSION` (currently `1`), `CAPY_STATE_HEADER_SIZE` (`16`), and 5 flag bits (`CAPY_STATE_FLAG_VISIBLE/ENABLED/FOCUSED/FOCUSABLE/CHECKED`).
- **Canonical binary format** (little-endian throughout for cross-host portability):
  - Header (16 bytes): `"CUIS"` magic, `uint16` format_version, `uint16` reserved (must be `0`), `uint32` FNV-1a checksum of body bytes, `uint32` node_count.
  - Pre-order body: per node `uint8 type`, `uint8 flags`, `int16 tab_index`, `int32 x`, `int32 y`, `uint32 w`, `uint32 h`, `uint16 text_len`, `text_len` UTF-8 bytes, `uint16 child_count`. `child_count` is bounded by `CAPY_WIDGET_MAX_CHILDREN` (32) and `text_len` by `CAPY_WIDGET_MAX_TEXT - 1` (255) on deserialize.
- **Preserved** in 1.10: type, bounds, visible/enabled/focused/focusable/checked, tab_index, text, child tree structure.
- **Deferred** (future additive minor): text_edit state, active animations, theme tokens, locale, 2D transform pointer, drag-and-drop config, IME state, layout cache fields (`layout_dirty`/`layout_version`/`dirty_version`), pool/slab state, callbacks (`on_click`/`on_change`/`on_submit`/`on_drop`), `user_data`, `font_id`, `image_id` and any host-owned resource handle. Hosts re-wire callbacks and re-bind resources after deserialize.
- Determinism: same widget tree → same blob byte-for-byte. Cross-platform thanks to explicit little-endian byte primitives.
- Foundation: undo/redo (v2.3) and devtools time-travel debugging (v2.5) build on this format.

## 2D transforms (since 1.9)

- `struct capy_dl_transform { int32_t m00, m01, m02, m10, m11, m12; }`. 2×3 affine matrix. `m00`/`m01`/`m10`/`m11` are Q16.16 (`0x10000` = 1.0); `m02`/`m12` are integer pixels for translation. Identity is `{0x10000, 0, 0, 0, 0x10000, 0}`. Declared in `capy_display_list.h` so producers and consumers share the type.
- New field `struct capy_dl_transform transform` on `capy_dl_cmd` (adds 24 bytes to `sizeof(capy_dl_cmd)`). Zero-filled for every op except `TRANSFORM_PUSH`.
- Widget tail field `const struct capy_dl_transform *transform` on `capy_widget`. NULL by default = identity (no PUSH/POP emitted; output byte-equivalent to pre-1.9 for the same widget tree). When non-NULL, `capy_widget_emit_recursive` prepends `CAPY_DL_TRANSFORM_PUSH` (carrying `*w->transform`) and appends `CAPY_DL_TRANSFORM_POP` around the widget's clip block.
- Factories (pure, no allocation):
  - `capy_transform_identity()` — diagonal 1s.
  - `capy_transform_scale(sx_q16, sy_q16)`.
  - `capy_transform_translate(tx, ty)` — `tx`/`ty` are integer pixels.
  - `capy_transform_rotate_quadrants(n)` — exact 0°/90°/180°/270°; `n` reduced modulo 4.
- `capy_transform_multiply(a, b)` returns `a*b`. Q16.16 multiply with `int64_t` intermediates and saturating clamp to `INT32_MAX/MIN`. NULL args degrade to identity.
- `capy_widget_set_transform(w, t)` stores the caller-owned matrix pointer; NULL clears.
- Display-list ops `CAPY_DL_TRANSFORM_PUSH`/`POP` are appended to `capy_dl_op` (DL schema **5 → 6** — second real schema bump since 0.9). Consumers reading `dl->version < 6` must treat both ops as opaque and ignore them.
- Arbitrary-angle rotation is **not provided in 1.9** — the widget core stays float-free and free of any `sin`/`cos` LUT. Hosts compute `(cos, sin)` externally (CORDIC, host LUT, etc.) and populate the matrix directly.
- Hit-test compensation is **deferred**: `capy_widget_handle_event` continues to compare against axis-aligned bounds. Hosts that route events into rotated widgets pre-apply the inverse transform to event coordinates themselves.

## Rich IME (since 1.8)

- `struct capy_ime_state { const char *preedit; uint16_t preedit_len, cursor_pos; const char *const *candidates; uint16_t candidate_count, selected_index; uint8_t composition_phase; uint8_t reserved[3]; }`. Snapshotting struct for hosts that render a candidate window; the live state lives on the textbox widget as tail fields.
- Widget tail fields (valid only when `type == CAPY_WIDGET_TEXTBOX`; all NULL/zero by default): `ime_preedit`, `ime_preedit_len`, `ime_cursor_pos`, `ime_candidates`, `ime_candidate_count`, `ime_selected_index`, `ime_composition_phase` (0=idle / 1=composing / 2=selecting), `ime_reserved[3]`.
- Setters (all no-ops on non-TEXTBOX): `capy_ime_set_preedit(w, text, len, cursor)`, `capy_ime_set_candidates(w, list, count, selected)`. CapyUI stores caller-owned pointers; the host engine must keep them alive until replaced.
- `int capy_ime_commit(w, final_text, len)` invokes `capy_textbox_insert(w, final_text, len)`, propagates its return value, and clears the IME state regardless of whether the insert was accepted.
- `void capy_ime_cancel(w)` clears the IME state without inserting; the textbox content is untouched.
- `void capy_ime_get_state(w, &out)` zero-fills `out` and (for TEXTBOX widgets) copies the tail fields into it. NULL-safe on both arguments.
- 4 event types appended to `capy_widget_event_type`: `CAPY_WIDGET_EVENT_IME_COMPOSE_START`, `PREEDIT_UPDATE`, `COMMIT`, `CANCEL`. All four are pass-through in `capy_widget_handle_event` (return 0). Host dispatcher uses them to broadcast phase transitions to overlays/notifications.
- Coexists with the pre-1.8 stub `capy_widget_ime_compose` (since 0.4) for minimal preedit forwarding without candidate handling.
- Forward compatibility: a future minor may add `CAPY_DL_CANDIDATE_WINDOW` to render the popup directly via the display-list. In 1.8 the host desktop session renders it externally using `capy_ime_get_state`.

## Slab allocator (since 1.7)

- `struct capy_slab { void *buffer; uint32_t size, element_size, high_water; void *free_list; }`. Caller-owned `buffer` partitioned into `floor(size / element_size)` slots. `element_size >= sizeof(void *)` so the free-list can live inline in freed slots.
- `capy_slab_init(s, buf, size, element_size)` zeros `high_water` and `free_list`. Degenerate inputs (NULL buffer, zero size, element_size below `sizeof(void *)`) clamp `s->size = 0` so all subsequent `_alloc` calls return `NULL` (fail-closed).
- `capy_slab_alloc(s)` first pops LIFO from the free-list; on empty, advances `high_water` to a fresh slot; on exhaustion returns `NULL`. O(1).
- `capy_slab_free(s, ptr)` pushes the slot onto the free-list. Caller is responsible for passing pointers previously returned by `_alloc` from the same slab; CapyUI does not validate the range. O(1).
- Determinism: identical `(init, alloc/free sequence)` over identically-laid-out buffers produces identical slot indices step-for-step.
- Companion to the bump allocator `capy_widget_pool` (since 0.15): pool for monotonically-growing arenas (no per-element free), slab for tight loops with frequent free+reuse.
- Forward compatibility: display-list compression and hash measure-cache from the original v1.7 plan are deferred to a future additive minor; the existing `capy_widget_diff` (since 0.8), `capy_display_list_diff_incremental` (since 1.1) and the `layout_dirty`/`layout_version` measure cache (since 0.15) already cover most of the equivalent use cases without growing the ABI further.

## Drag and drop (since 1.6)

- New event types appended at the tail of `capy_widget_event_type`: `CAPY_WIDGET_EVENT_DRAG_BEGIN`, `DRAG_MOVE`, `DRAG_END`, `DROP`. DRAG_BEGIN/MOVE/END are no-ops in the widget core — the host dispatcher owns the visual preview, cursor tracking and cancel (ESC) lifecycle. DROP is the only event the core acts on.
- `struct capy_dnd_payload { char type[32]; const void *data; uint32_t len; }`. Payloads are always passed by `const struct capy_dnd_payload *`; CapyUI never copies, allocates or frees them. The `data` buffer is host-owned and must outlive the drag.
- Widget tail fields (all NULL/zero by default, preserving pre-1.6 behaviour): `drag_payload` (turns `w` into a drag source), `drop_accepted_types` + `drop_types_count` (caller-owned filter array), `on_drop` (callback) + `drop_user_data`.
- Setters: `capy_widget_set_draggable(w, payload)`, `capy_widget_set_drop_target(w, types, count)` (NULL array clamps count to zero), `capy_widget_set_on_drop(w, fn, user_data)`. All NULL-safe on the widget argument.
- Matchers: `capy_dnd_type_matches(accepted, type)` accepts `"*"` (any), `"prefix/*"` (prefix match), and exact case-insensitive matches; intermediate stars or character classes are intentionally unsupported. `capy_widget_dnd_accepts(w, payload)` walks the widget's filter array and returns the OR of the per-pattern matches.
- Routing: `capy_widget_handle_event` with `event->type == CAPY_WIDGET_EVENT_DROP` and `event->payload` as a `const struct capy_dnd_payload *` walks children deepest-first; the first widget under `(event->x, event->y)` that has `on_drop` set AND whose filter accepts the payload type wins. Its callback returns `1` to stop propagation or `0` to keep bubbling.
- Determinism: same `(widget tree, event sequence)` → same callback dispatched with the same `(target, payload, user_data)` triple.

## Multi-display & DPI scaling (since 1.5)

- `capy_widget_context.dpi_scale_x256` and `capy_display_list.dpi_scale_x256` are Q8.8 fixed-point fields (`256` = 1.0×, `384` = 1.5×, `512` = 2.0×). Both default to `256` via the respective `_init` functions, preserving pre-1.5 behaviour.
- `void capy_widget_set_dpi_scale(ctx, scale_x256)` clamps `0` up to `1` (no upper clamp) and stores the value. `uint16_t capy_widget_get_dpi_scale(const ctx)` returns the stored value or `256` for NULL / zero-initialised contexts.
- `int32_t capy_widget_dpi_scale_dim(scale_x256, value)` is a pure helper: `value * scale_x256 / 256` in `int64_t`, truncated toward zero, saturating-clamped to `INT32_MAX/MIN`. Hosts use this to scale their own padding/gap/min/max constants deterministically. CapyUI does **not** auto-scale internal measure/arrange in 1.5.
- Display-list schema `5` adds `CAPY_DL_DPI_SCOPE`. When the host sets `dl->dpi_scale_x256 != 256` before `capy_widget_emit`, the emit prepends a `DPI_SCOPE` op with `rect = root.bounds` and `image_id = dpi_scale_x256`. When the scale is `256` (default), no scope op is emitted and the byte sequence is identical to pre-1.5 emit for the same widget tree.
- The GPU translator (`capy_dl_to_quads`, since 1.2) recognises `DPI_SCOPE` and skips it (no quad emitted). Host backends read the scope op separately to choose the rasterizer / viewport.
- Multi-display: hosts keep one `capy_widget_context` per display so each tree carries its own scale independently. There is no cross-context coupling.
- Forward compatibility: `capy_display_list` ships a `reserved_dpi` 16-bit tail slot so a future minor can introduce a separate `font_size_scale_x256` without growing the struct.

## Gesture engine (since 1.4)

- `enum capy_gesture_kind`: `NONE=0`, `TAP`, `DOUBLE_TAP`, `LONG_PRESS`, `SWIPE_LEFT/RIGHT/UP/DOWN`, `PINCH_IN/OUT` (reserved, not emitted in 1.4), `ROTATE_CW/CCW` (reserved, not emitted in 1.4), `DRAG`.
- `struct capy_gesture { uint8_t kind; uint8_t reserved[3]; capy_ui_point start, end; int32_t magnitude; uint32_t duration_ms; }`. `magnitude` carries the dominant-axis pixel delta for SWIPE/DRAG, percent for future PINCH, deg×256 for future ROTATE.
- `struct capy_gesture_recognizer` is caller-embedded (≈80 bytes). Thresholds are tunable after `capy_gesture_recognizer_init` seeds the defaults: tap 200 ms / 10 px, double-tap gap 300 ms, long-press 500 ms, swipe 50 px. Distance comparisons use Chebyshev (max-abs) for a fast integer L∞ norm; no `sqrt`, no float.
- `capy_gesture_feed(r, ev, now)`: returns `1` if a gesture was queued in `r->pending`, `0` if the event only updated state, `-1` on NULL. Only TOUCH_BEGIN/MOVE/END are acted upon; POINTER/KEY/WHEEL/GAMEPAD (and future event types) are ignored. A second TOUCH_BEGIN with a different `touch_id` while a touch is already active is **ignored** (state machine remains anchored on the first touch).
- `capy_gesture_tick(r, now)`: lets the host drive LONG_PRESS detection without a new touch event. Emits LONG_PRESS once when elapsed >= threshold and the finger has not moved beyond `tap_max_distance_px`. Returns `1` on emit, `0` otherwise, `-1` on NULL.
- `capy_gesture_poll(r, &out)`: consumes the pending gesture (depth-1 queue). Returns `1` and copies into `out` on success, `0` if nothing pending, `-1` on NULL.
- Determinism: same `(events, now)` sequence yields the same gestures with the same field values across runs and platforms.

## Rich animation (since 1.3)

- `struct capy_anim_keyframe { uint32_t tick; int32_t value; uint8_t easing; uint8_t reserved; uint16_t reserved2; }` and `struct capy_anim_track { capy_anim_keyframe *keyframes; uint32_t count, capacity; int32_t loop; }`. Keyframes are caller-owned and assumed sorted ascending by `tick`. `loop = 0` plays once and clamps to first/last value outside the range; `loop = -1` loops infinitely (modulo the track duration); `loop > 0` repeats N times then clamps to the last value. The easing of the **earlier** keyframe in each segment is applied via the existing `capy_anim_easing` enum.
- `int capy_anim_track_sample(const struct capy_anim_track *track, uint32_t now, int32_t *out)` returns `0` on success, `-1` on NULL args / `count == 0`. Deterministic: same `(now, track)` → same `*out`.
- `struct capy_spring { int32_t target, velocity, position; uint16_t stiffness, damping; }`. `velocity` is Q24.8; `stiffness`/`damping` are Q8.8 (e.g. `256 = 1.0`); `target` and `position` are plain integer units.
- `void capy_spring_step(struct capy_spring *s, uint32_t dt_ms)` advances the spring by `dt_ms` milliseconds using 1 ms symplectic-Euler substeps. With `damping > 0` the system converges; with `damping == 0` it oscillates indefinitely. `dt_ms == 0` is a no-op. All arithmetic uses `int64_t` intermediates and saturating clamps to `int32_t` for both `velocity` and `position`.
- `int32_t capy_anim_bezier_eval(int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t t_q16)` evaluates a 1D cubic Bézier via de Casteljau. `t_q16` is Q16.16 in `[0, 1]` (clamped). Endpoints exact (`t=0 → p0`, `t=0x10000 → p3`). Monotonic for monotonically ordered control points. Hosts wanting CSS-style easing (P0=(0,0), P3=(1,1) with x→t solve) can wrap this with their own Newton-Raphson.
- All signed shifts inside the 1.3 implementations are replaced by `/` and `*` (truncation toward zero) to avoid C11 implementation-defined right-shift on negative signed values and the undefined behaviour of left-shift on negative signed values. Zero float in the widget core remains an invariant.

## GPU translator (since 1.2)

- New header `src/widget/capy_dl_gpu.h` exposing `struct capy_gpu_quad { int32_t x0,y0,x1,y1; uint32_t color; uint32_t texture_id; int32_t uv_x0,uv_y0,uv_x1,uv_y1; }` and:
  - `int capy_dl_to_quads(const struct capy_display_list *dl, struct capy_gpu_quad *out, uint32_t cap, uint32_t *out_count);`
- Translator walks the schema-4 display-list and decomposes ops into quads:
  - `CAPY_DL_RECT` → 1 solid-colour quad (texture_id=0).
  - `CAPY_DL_BORDER` → 4 quads (top, bottom, left, right strips using `cmd->border_width`).
  - `CAPY_DL_TEXT` → 1 placeholder quad with texture_id=0; the host re-translates with its own glyph atlas keyed by `font_id`+`text_offset`+`text_len`.
  - `CAPY_DL_FOCUS_RING` → 4 quads (same shape as BORDER).
  - `CAPY_DL_IMAGE_REF` → 1 quad carrying `texture_id = cmd->image_id` and full-texture UVs (0,0)-(rect.w, rect.h).
  - `CAPY_DL_CLIP_PUSH/POP` → internal scissor stack (depth bounded to `CAPY_DL_GPU_CLIP_STACK_MAX = 16`). Quads after a CLIP_PUSH are intersected against the active clip; zero-area clips silently drop contained quads.
  - `CAPY_DL_DIRTY_HINT` / `CAPY_DL_NONE` → ignored.
- Returns `0` on success with `*out_count` set, or `-1` on overflow (`*out_count` partial), unbalanced clip stack (CLIP_POP without push, or CLIP_PUSH still on stack at end), unknown op, NULL `dl`/`out_count`, or NULL `out` with `cap > 0`.
- The translator does not invoke any GPU API — CapyOS picks the backend (Mesa software rasterizer, lavapipe, VirGL, hardware GL/Vulkan, etc.) and consumes `capy_gpu_quad[]`.
- DL schema **remains 4** in 1.2. The original v1.2 plan reserved schema 5 for GPU primitives, but the translator output lives in a separate buffer type so no new DL op is introduced; the schema bump is deferred until a real op is added.

## Damage tracking (since 1.1)

- `capy_widget` gains an additive `uint32_t dirty_version` tail field. `capy_widget_invalidate` (since 0.15) now **also** bumps `dirty_version` on every widget it visits along the parent chain. `capy_widget_invalidate_subtree(widget)` is the downward sibling — it walks `widget` and every descendant, setting `layout_dirty = 1` and bumping `dirty_version`.
- `uint32_t capy_widget_dirty_version(const struct capy_widget *widget)` returns the current monotonic counter (NULL → 0). Hosts compare snapshots between frames; overflow wraps silently because hosts observe the *delta*, not the absolute value.
- `capy_widget_context` gains an additive `uint32_t frame_budget_microseconds` tail field. `capy_widget_frame_budget(ctx, microseconds)` stores the value; CapyUI never enforces it — the field is published for cooperative host-side degradation. NULL `ctx` is a no-op.
- `capy_display_list_diff_incremental(prev, next, out_dirty, out_cap)` is byte-equivalent to `capy_widget_diff` (since 0.8) but skips the common leading prefix before running the positional inner diff. Suffix skipping is deliberately not applied because it would re-align the two lists and drop trailing rects that the positional algorithm emits when the lists differ in length.
- `capy_widget_emit` does **not** consult `dirty_version` in 1.1; the field is published so future minors can layer emit caching without another ABI bump.

## Performance tier 1 (since 0.15)

- New struct `capy_widget_pool { void *buffer; uint32_t size; uint32_t used; uint32_t high_water; }`.
- New APIs:
  - `void capy_widget_pool_init(p, buf, size)` — binds a caller-owned buffer (NULL `buf` clamps `size` to 0).
  - `void capy_widget_pool_reset(p)` — sets `used = 0`; **preserves** `high_water` for diagnostics.
  - `struct capy_widget_allocator capy_widget_allocator_from_pool(p)` — returns an allocator wrapper. `alloc` is a 16-byte aligned bump; `free` is a no-op (pool is bulk-managed).
  - `void capy_widget_invalidate(widget)` — walks from `widget` up through `parent` pointers, setting `layout_dirty = 1` on every ancestor (including `widget`). NULL is a no-op.
- Pool `alloc` returns NULL when: pool/buffer NULL, `size == 0`, request larger than remaining capacity, request larger than `UINT32_MAX`. Pool state stays consistent across failures.
- `capy_widget` gains additive tail fields `uint8_t layout_dirty` + `uint32_t layout_version`. `capy_widget_create` sets `layout_dirty = 1` so first measure always computes.
- `capy_widget_measure` short-circuits when `layout_dirty == 0` AND `bounds.width/height` already equal the clamped inputs (cache hit — no version bump). Otherwise it writes bounds, clears `layout_dirty` and increments `layout_version` (monotonic).
- Benchmarks and a formal performance budget remain optional follow-ups for external CI; CapyUI exposes the cache infrastructure but does not enforce timing here.

## Theme serialization (since 0.14)

- New constant `CAPY_THEME_FORMAT_VERSION = 2u`. Default themes (`capy_theme_default_light/dark/high_contrast`) now ship with `version = 2` (bumped from `1` to align with the published format).
- New APIs:
  - `int capy_theme_serialize(const struct capy_theme *t, char *out, uint32_t cap)` — emits canonical text: `version=2`, `variant=light|dark|high_contrast`, `high_contrast=0|1`, then each token (`bg_base`, `bg_raised`, ..., `disabled`) as `name=0xAARRGGBB` uppercase hex. Returns bytes written (excl. NUL), `-1` on overflow / NULL / cap=0 / invalid variant. NUL-terminated on success. Deterministic: same `theme` → same bytes.
  - `int capy_theme_deserialize(struct capy_theme *out, const char *in, uint32_t len)` — line-oriented parser tolerating leading/trailing whitespace, `#` comments and blank lines. Optional `0x`/`0X` prefix in hex values. Returns `0` on success, `-1` on NULL args, `version > CAPY_THEME_FORMAT_VERSION` or `version == 0`, missing `version=` line, malformed line (no `=`, bad hex, unknown variant, bad boolean, empty key).
- Unknown keys (tokens added by future format versions) are **silently ignored** on deserialize for forward-compat. Missing tokens stay zero in `out`; callers wanting defaults should seed `out` with `capy_theme_default_*()` before deserializing.
- The format itself is auditable plain text. A binary theme-pack format is planned for v2.4 separately.

## Locale + RTL (since 0.13)

- New struct `capy_locale { char tag[16]; uint8_t rtl; uint8_t plural_rule; uint16_t reserved; }` and `enum capy_plural_rule { EN, PT, AR, RU, OTHER }`.
- `capy_widget_context` gains an additive `locale` field at the tail. `capy_widget_context_init` seeds it with `capy_locale_default()` = `("en-US", LTR, EN)`.
- `capy_layout_constraints` gains an additive `uint8_t rtl` at the tail. When non-zero, `capy_widget_arrange` mirrors the x-coordinate of every direct child within the content rect (`new_x = 2*ix + iw - old_x - width`) **after** the layout kind-specific pass and **before** recursing into descendants — so grandchildren are arranged relative to the final parent x.
- New APIs:
  - `struct capy_locale capy_locale_default(void)`.
  - `void capy_context_set_locale(ctx, l)` (NULL restores default).
  - `int capy_locale_plural(l, n)` (NULL locale falls back to EN). Returns the index of the plural form under the locale rule.
  - `int capy_locale_format(l, out, out_cap, fmt, ...)`: tiny printf subset covering `%d` (int32_t), `%u` (uint32_t), `%x` (uint32_t lowercase hex), `%s` (const char *), `%%` (literal). No floats. Fail-closed on overflow, NULL out/fmt, zero cap, trailing lone `%`, or unknown specifier; always NUL-terminated in success. The locale argument is currently unused but reserved for future locale-aware digit shaping.
- Plural rules: EN/PT (one for n=1, else other); AR (zero/one/two/few/many/other by CLDR); RU (one/few/many by CLDR); OTHER (catch-all).
- Refactor: `capy_widget_arrange` dispatcher now owns the recursion into children (previously each layout kind recursed internally). Behavior is preserved for LTR trees; mirror runs uniformly when `rtl=1`.

## Accessibility tree (since 0.11)

- New struct `capy_a11y_node` carries `widget_id`, `role`, `label`, `description`, `state_flags`, `parent_id`.
- New API `int capy_widget_a11y_snapshot(struct capy_widget *root, struct capy_a11y_node *out, uint32_t cap);` walks the tree in pre-order and fills `out[]` with one node per visited widget. Returns the count on success, `-1` on overflow (partial fill, no rollback), `NULL root`, `cap == 0`, or `out == NULL && cap > 0`.
- `widget_id` is a deterministic FNV-1a 32-bit hash of the path: root uses `0x811C9DC5`; each child mixes `(parent_id, child_index)` through 4 byte-steps of the prime `0x01000193`. Same widget at the same position → same `widget_id` across snapshots.
- `parent_id` is the `widget_id` of the parent; root nodes have `parent_id = 0`.
- `label` resolution: `widget->text` when non-empty (points directly into the widget buffer), else the literal `"Untitled"`. `description` is always `NULL` in 0.11.
- Role mapping covers all 12 widget types: `LABEL→"label"`, `BUTTON→"button"`, `TEXTBOX→"textbox"`, `CHECKBOX→"checkbox"`, `LIST→"list"`, `PANEL→"panel"`, `SCROLLBAR→"scrollbar"`, `MENUBAR→"menu"`, `MENU_ITEM→"menuitem"`, `DIALOG→"dialog"`, `PROGRESS→"progressbar"`, `TABS→"tab"`. Strings are static literals — CapyUI never allocates.
- State flags: `focused→FOCUSED`, `checked→CHECKED`, `!enabled→DISABLED`, `!visible→HIDDEN`, `TEXTBOX && readonly→READONLY`. `PRESSED`/`EXPANDED`/`SELECTED` reserved for later slices.
- Snapshot never mutates the widget tree. Deterministic across runs.

## Input plumbing (since 0.10)

- New abstract event types appended to `capy_widget_event_type`: `CAPY_WIDGET_EVENT_WHEEL`, `CAPY_WIDGET_EVENT_TOUCH_BEGIN`, `CAPY_WIDGET_EVENT_TOUCH_MOVE`, `CAPY_WIDGET_EVENT_TOUCH_END`, `CAPY_WIDGET_EVENT_GAMEPAD`.
- `capy_widget_event` gains an additive tail field `const void *payload` whose concrete type is determined by `type`: `capy_wheel_payload` (`delta_x`/`delta_y`), `capy_touch_payload` (`touch_id`/`pos`/`pressure_x256`), `capy_gamepad_payload` (`button_mask`/`axis[4]`). NULL for legacy POINTER/KEY events.
- New helper struct `capy_ui_point` (`int32_t x`, `int32_t y`) shared by the touch payload.
- Modifier bitmask expanded with `CAPY_MOD_CAPSLOCK` (`0x10`) and `CAPY_MOD_NUMLOCK` (`0x20`), both disjoint from the existing `CTRL/ALT/SHIFT/META` flags.
- Routing (CapyUI side):
  - `WHEEL`: routed via `capy_widget_find_at` + nearest `LIST` ancestor; adjusts `list->value` by `payload->delta_y` clamped to `[min_value, max_value]` and fires `on_change` when the value changes. Returns `1` if a list absorbed the event, `0` otherwise.
  - `TOUCH_BEGIN/MOVE/END` (single touch): translated in-memory to `POINTER_DOWN/MOVE/UP` and dispatched through the existing pointer path. When the payload is present, `payload->pos.x/y` overrides `event->x/y`. Multi-touch gestures land in v1.4.
  - `GAMEPAD`: accepted silently (`return 0`); configurable button bindings arrive in a later slice.
  - Unknown event types (future ABI): ignored without crash.
- Hosts not yet emitting WHEEL/TOUCH/GAMEPAD remain fully functional — POINTER/KEY remain the minimum required surface.

## Text metrics hook (since 0.9)

- New typedef `int (*capy_text_metrics_fn)(uint16_t font_id, uint8_t font_size, const char *text, uint16_t len, uint32_t *out_width, uint32_t *out_height, void *user_data);`.
- `capy_widget_context` gains additive `text_metrics_fn` + `text_metrics_user` fields at the tail. `capy_widget` gains additive `uint16_t font_id` at the tail.
- New API `void capy_widget_set_text_metrics_hook(struct capy_widget_context *ctx, capy_text_metrics_fn fn, void *user_data);` (passing `fn == NULL` clears the hook).
- New API `int capy_widget_measure_text(const struct capy_widget_context *ctx, uint16_t font_id, uint8_t font_size, const char *text, uint16_t len, uint32_t *out_width, uint32_t *out_height);`.
- Hook set and returning `0`: outputs from the hook.
- Hook NULL or hook returning `-1`: deterministic fallback `width = (effective_size / 2) * len`, `height = effective_size`, with `effective_size = (font_size > 0 ? font_size : 16)`.
- `text == NULL` or `len == 0`: short-circuits to `width = 0`, `height = effective_size` (hook not called).
- `out_width == NULL` or `out_height == NULL`: returns `-1` without invoking the hook.
- Hook is read-only on `text` and must not allocate on the hot path.
- Display-list `CAPY_DL_TEXT` carries `font_id` for the host font loader; the resolved glyph rendering stays out of CapyUI.

## Compositor diff (since 0.8)

- New API `int capy_widget_diff(const struct capy_display_list *prev, const struct capy_display_list *next, struct capy_ui_rect *out_dirty, uint32_t out_cap);` walks both lists in parallel by command index, emits the rect of every differing command (and, when the rects themselves differ, both rects), then the rects of any trailing commands present only in one of the two lists.
- Stable coalescing: a rect identical to the last appended one is suppressed. Two independent calls on the same `(prev, next)` produce the same sequence of rects byte-for-byte.
- Returns the number of rects written (≥0), or `-1` on overflow of `out_cap` (partially filled, no rollback) or `out_dirty == NULL && out_cap > 0`.
- `prev == NULL` or `next == NULL` is treated as an empty list (the other list is fully dirty).
- O(max(prev->count, next->count)). Zero allocation.
- `CAPY_DL_DIRTY_HINT` op (schema 3) is reserved for higher-level producers — `capy_widget_emit` never emits it. Consumers running an older schema (≤2) must ignore this op when reading newer producers.

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

## Desktop session contract (since 0.7, delivered in `alpha.241`)

- Desktop runtime owns taskbar, launcher (`Super` shortcut), system
  tray, desktop icons, wallpaper, notifications.
- Window manager owns focus, lifecycle (`open`/`close`/`minimize`/
  `maximize`/`restore`), drag/drop and snap.
- Dispatcher routes keyboard, mouse, scroll, hover, click, drag,
  right-click/context menu to focused window via
  `gui_window_dispatch_event()`; preserves overlays/taskbar/desktop
  icons priority order.
- Apps must be host-isolated: a single-app crash must not bring the
  desktop down. The desktop session contains the failure via the
  dispatcher and logs via the CapyOS klog audit channel.
- Activation gate: CapyOS only starts the desktop session when
  `kernel_module_desktop_session_available()` returns 1, which is
  determined by the presence of `/var/capypkg/org.capyos.ui.desktop-session/installed`.
- Build modes:
  - `PROFILE=full` (default) + `../CapyUI` sibling present → sources
    compiled from `CapyUI/src/desktop`, `CapyUI/src/window`,
    `CapyUI/src/apps`;
  - `PROFILE=full` + sibling absent → CapyOS in-tree fallback under
    `src/gui/desktop/`, `src/gui/window/`, `src/apps/` is compiled
    (for migration parity; not the owner of feature);
  - `PROFILE=core-only` → desktop session is excluded entirely and
    `kernel_module_desktop_session_available()` returns 0 by static
    build flag (`CAPYOS_PROFILE_CORE_ONLY`).

## Install/update boundary

CapyUI ships modules consumed by the CapyOS `services/capypkg`
adapter. CapyOS owns:

- compositor;
- windows surfaces (framebuffer scanout, layering, damage);
- input device plumbing (keyboard, mouse, USB HID Etapa 3);
- fonts;
- drawing surfaces;
- theme provider host;
- accessibility and shell integration;
- staging, activation and rollback;
- the `kernel/module_gate.c` decision that gates the desktop session
  by `/var/capypkg/<canonical-name>/installed`;
- the first-boot wizard step that calls
  `capypkg_bootstrap_run_with_progress()` and lets the user select
  modules.

## Dependency rules

UI components may depend on:

- `capy-ui-widget` (always);
- `capy-ui-desktop-session` (only when targeting CapyUI-managed
  desktop apps; not for kernel-side adapters);
- CapyOS UI adapter ABIs through documented headers.

They must not depend on raw GUI structs from CapyOS source.

The `org.capyos.ui.desktop-session` manifest **must** declare
`depends=org.capyos.ui.widget-core`. The CapyOS in-tree adapter
enforces the dependency at install time (max depth 8, deterministic
ordering).

## Validation before CapyOS integration

Before CapyOS consumes a CapyUI release, externally validate:

- widget tree construction (host-side fixtures in `tests/`);
- event routing (host-side dispatcher tests);
- focus traversal when applicable;
- display-list fixtures (golden tests);
- deterministic error handling;
- no direct CapyOS runtime includes (use `make validate`);
- `make package` produces canonical `<name>.bin` + `<name>.manifest`
  in line-oriented `key=value` format for both modules;
- `make modules-index` (host-side) aggregates both manifests with
  `---` separator into `modules-index.txt`.

CapyUI integration is gated by Etapas 4 (widget model formal
contract) and 6 (desktop apps using the model). The
`org.capyos.ui.desktop-session` v1 module is already installable via
capypkg in alpha mode (publisher pinned to current `VERSION`).

## Publishing as a Capy package

When CapyUI is delivered as a remote module to the CapyOS
`services/capypkg` adapter, the publisher must follow
[`CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`](../../CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md).
The key requirements that affect CapyUI are:

- `payload_url` must be HTTPS only;
- `payload_sha256` must be lowercase 64 hex of the published artifact;
- `payload_size` ≤ 1 MiB during the alpha streaming-buffer window
  (`CAPYPKG_PAYLOAD_MAX = 8 MiB` will become reachable when the
  streaming writer lands);
- `name` must match `[a-zA-Z0-9._-]`; canonical names are
  `org.capyos.ui.widget-core` and `org.capyos.ui.desktop-session`;
- `install_root` must live under `/var/capypkg` or `/opt/`;
- `signature_ed25519` must cover the canonical descriptor
  `name=N|version=V|payload_sha256=H|payload_url=U\n` (literal `|`
  separators, single `\n` terminator);
- the CapyUI ABI version pinned in `docs/roadmap/contracts/abi-versions.md`
  must be mirrored in `required_abis[]` on the high-level JSON index;
- `depends=org.capyos.ui.widget-core` must be set in the
  `org.capyos.ui.desktop-session` manifest.

Until CapyAgent publishes its Ed25519 signer, CapyUI cannot be
installed from a `signed` repository in production; `--unsigned`
labs are possible but never promote to user-facing release.

## Continuous delivery

`.github/workflows/release-artifacts.yml` runs `make validate` +
`make package` on every push to `main`, republishes the rolling
`latest` GitHub Release with the `.bin/.manifest/modules-index.txt`
assets and marks it as the latest release. The CapyOS first-boot
wizard pins
`https://github.com/<owner>/CapyUI/releases/download/v<X.Y.Z>/modules-index.txt`
by semver tag to avoid the `latest` redirect during kernel-space
module installation.

To freeze a semver release without affecting the rolling channel,
push a `v*` tag (e.g. `v0.7.3`). The same workflow handles
tag-triggered runs as standalone frozen releases.
