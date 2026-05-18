# CapyUI compatibility and integration contract

CapyUI modules must remain portable retained UI model logic and must not call CapyOS compositor internals directly.

Authoritative CapyOS references:

- `CapyOS/docs/reference/integration/modular-installation-architecture.md`
- `CapyOS/docs/reference/integration/capyui-widget-integration-contract.md`

## Owned ABI

CapyUI owns the `capy-ui-widget` ABI.

This ABI covers:

- widget tree model;
- abstract events;
- retained state;
- style token contracts;
- future layout and display-list output.

## Compatibility rules

- Widget model changes must be additive until the relevant CapyOS stage permits migration.
- Event names and payloads must be versioned once serialized or exposed across modules.
- Display-list output must be deterministic and independent from CapyOS surface pointers.
- CapyUI must not depend on fonts, framebuffer, compositor, input drivers or shell internals.
- Host adapters must convert CapyOS events/surfaces into CapyUI abstractions.

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
