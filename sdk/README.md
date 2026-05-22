# CapyUI SDK — public API surface for plugins and embedders (since 2.0)

This directory is the **public contract** between CapyUI and downstream
plugins / app authors. It curates the headers, examples and tutorials
that make it possible to build a CapyUI plugin without spelunking the
internal sources.

## Status

- **Seed published in `v2.0.0`.** APIs come from `src/widget/capy_widget.h`
  and `src/widget/capy_display_list.h`. The SDK does not yet vendor those
  headers; consumers `#include "capy_widget.h"` from the publisher's
  include path. A future patch may copy a curated subset under
  `sdk/include/capyui/` so downstreams can pin a smaller header set.
- **What is stable in 2.0.0:**
  - The plugin descriptor and context structs (`capy_plugin_descriptor`,
    `capy_plugin_context`).
  - `capy_plugin_register` / `capy_plugin_unregister_all`.
  - `CAPYUI_API_VERSION_TAG` macro for compile-time ABI gating.
  - `CAPY_MAX_PLUGINS` for the static per-context registry capacity (8).
  - `CAPY_DL_PLUGIN_OP` as the canonical channel through which plugins
    emit custom commands into the display-list (display-list schema 7).

## How a plugin is structured

1. Declare a `capy_plugin_descriptor` with your reverse-DNS `id`, a
   human-readable `version`, and the host ABI you require via
   `capy_ui_abi_required` (use `CAPYUI_API_VERSION_TAG` to target the
   exact host you compile against, or hand-roll an older value for
   forward-compatibility).
2. Provide a caller-owned `capy_widget_allocator` so the host can audit
   that your plugin allocates from its own arena rather than reusing
   the host allocator.
3. Implement the lifecycle callbacks you need (`init`, `destroy`,
   `on_event`, `emit`). NULL is allowed — the host treats them as no-ops.
4. Set `timeout_microseconds` to advertise a soft deadline; the host
   dispatcher is responsible for enforcing it under its own watchdog.
5. Call `capy_plugin_register(ctx, &descriptor)` once per `capy_widget_
   context` you want to plug into. The function returns the 0-based slot
   index on success or `-1` on validation/capacity failure.

## How plugins emit into the display-list

- The widget core itself does **not** emit `CAPY_DL_PLUGIN_OP`. Hosts
  that want plugin output append commands of this kind through their
  own dispatcher (typically inside the `emit` callback of the descriptor).
- The op carries an opaque 32-byte payload reusing the existing
  `capy_dl_cmd` fields (`rect`, `color`, `text_offset`, `text_len`,
  `border_width`, `font_size`, `font_id`, `image_id`) which together
  sum to 32 bytes. `image_id` is the canonical "plugin id" slot —
  hosts map it back to a registered descriptor.
- Pre-2.0 consumers reading `dl->version < 7` must treat the op as
  opaque and ignore it.

## Sandbox guarantees

- The widget core **never invokes** plugin callbacks. The host
  dispatcher iterates the registry and decides when to call. This keeps
  the core float-free, deterministic and crash-isolated by construction.
- The plugin allocator pointer is stored separately from the host
  context's allocator. The core never mixes them.
- `capy_plugin_unregister_all` is the single cleanup entry point. It
  walks the registry, invokes `destroy` for each non-NULL slot, and
  resets the count.

## Template plugin

See `sdk/plugin-template.c` for a minimal compilable example that:

- Declares a descriptor with `capy_ui_abi_required = CAPYUI_API_VERSION_TAG`.
- Implements no-op `init` / `destroy` callbacks.
- Demonstrates registering with a `capy_widget_context` and cleaning up
  on shutdown.

## Migration from 1.x

There are **no symbol removals in 2.0.0** despite the major bump. The
bump signals that the deprecation-policy door is now open for 2.x
minors: future bumps can remove APIs after the standard ≥2 minor
deprecation window. 1.x continues in LTS for at least 12 months
post-2.0 per `docs/roadmap/contracts/deprecation-policy.md`.

Any host that compiles cleanly against `v1.10.0` compiles cleanly
against `v2.0.0` — the only ABI surface change visible to non-plugin
hosts is the addition of `plugin_count`/`plugins[]` tail fields in
`capy_widget_context` (zero-initialised by `capy_widget_context_init`).
