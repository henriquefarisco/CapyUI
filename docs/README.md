# CapyUI documentation

CapyUI owns portable retained UI model code that can evolve outside CapyOS.

## Current migration

Migrated from CapyOS concepts:

- `CapyOS/include/gui/widget.h`
- `CapyOS/src/gui/widgets/widget.c`

Created here:

- `src/widget/capy_widget.h`
- `src/widget/capy_widget.c`
- `src/widget/capy_layout.h`
- `src/widget/capy_layout.c`
- `src/widget/capy_display_list.h`
- `src/widget/capy_display_list.c`
- `docs/compatibility.md`
- `docs/roadmap/` (roadmap segregado por fase + contratos versionados + tracking)

## Roadmap

O roadmap completo da plataforma vive em `docs/roadmap/` e cobre v0.0.1 → v3.0.0:

- **Estado atual:** `docs/roadmap/STATUS.md`.
- **Curto prazo (v0.3 → v0.6):** `docs/roadmap/short-term/` — foco, edição de texto, animação, theme tokens v2.
- **Médio prazo (v0.7 → v1.0):** `docs/roadmap/medium-term/` — adapter CapyOS, compositor, fontes, input, a11y, shell, i18n, theming UX, performance, ABI 1.0 congelado.
- **Longo prazo (v1.1 → v3.0):** `docs/roadmap/long-term/` — damage tracking, GPU path, animação rica, gesture engine, multi-display, drag&drop, IME rico, transforms, serialização, plugin system + SDK, widgets avançados, virtualização, undo/redo, theme packs, devtools, vision platform.
- **Contratos:** `docs/roadmap/contracts/` — 19 contratos versionados por área de ABI.
- **Dependências:** `docs/roadmap/dependencies/` — gates CapyOS, grafo interno, política de breaking change, matriz de compatibilidade.
- **Tracking absoluto:** `docs/roadmap/tracking/` — `ABSOLUTE.md`, `BLOCKERS.md`, `METRICS.md`, `DECISIONS.md`, `CHANGELOG.md`.

## Boundary

CapyUI owns:

- widget tree data model;
- abstract event routing;
- style tokens;
- retained state;
- layout/display-list contracts in future slices.

CapyOS owns:

- compositor;
- windows;
- input device plumbing;
- fonts;
- drawing surfaces;
- theme provider;
- accessibility and shell integration.

## Migration note

The extracted model intentionally does not include CapyOS compositor, font or rendering calls. CapyOS should integrate it later through a small adapter that maps `gui_event` and `gui_surface` to abstract CapyUI events/display lists.

## Tag-release compatibility model

Early alpha releases use GitHub release tags plus a compatibility index without certificate/signature enforcement. Required metadata:

- component id;
- tag;
- artifact name;
- sha256;
- required CapyOS/CapyUI ABI versions;
- dependencies;
- requested permissions.

## Next slices

1. ~~Add layout primitives.~~ (delivered in 0.1.0)
2. ~~Add display-list output.~~ (delivered in 0.2.0)
3. ~~Add keyboard focus traversal.~~ (delivered in 0.3.0)
4. ~~Add text editing model.~~ (delivered in 0.4.0)
5. ~~Add animation and timing.~~ (delivered in 0.5.0)
6. ~~Add theme tokens v2.~~ (delivered in 0.6.0 — **short-term phase complete**)
7. Add CapyOS adapter only when Etapas 4/6 permit integration — `docs/roadmap/medium-term/v0.7-host-adapter-gate-etapa4.md`.

Ver plano completo em `docs/roadmap/README.md`.
