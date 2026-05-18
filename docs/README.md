# CapyUI documentation

CapyUI owns portable retained UI model code that can evolve outside CapyOS.

## Current migration

Migrated from CapyOS concepts:

- `CapyOS/include/gui/widget.h`
- `CapyOS/src/gui/widgets/widget.c`

Created here:

- `src/widget/capy_widget.h`
- `src/widget/capy_widget.c`
- `docs/compatibility.md`

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

1. Add layout primitives.
2. Add display-list output.
3. Add keyboard focus traversal.
4. Add host-side widget tree tests.
5. Add CapyOS adapter only when Etapas 4/6 permit integration.
