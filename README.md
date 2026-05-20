# CapyUI

Version: 0.7.0

CapyUI owns portable widget primitives and interaction contracts for CapyOS services.

## CapyOS reference version

Pinned for this release: `0.8.0-alpha.241+20260519`. Since 0.7.0 CapyUI also publishes the `org.capyos.ui.desktop-session` capypkg module (compositor session, window manager, apps) in addition to `org.capyos.ui.widget-core`. Update this together with `docs/compatibility.md` whenever the CapyOS core version, ABI or canonical manifest format changes.

Cross-repo authoritative references:

- `CapyOS/docs/reference/integration/compatibility-matrix.md`
- `CapyOS/docs/reference/integration/capypkg-publisher-manifest-format.md`
- `CapyOS/docs/operations/manual-module-deploy-runbook.md`

## Validation

```sh
make validate
```

The release gate compiles with strict C warnings, runs widget contract tests, checks release metadata and verifies hardened compile flags.

## capypkg release assets

```sh
make package
```

The package target emits the assets consumed by CapyOS first-boot module bootstrap under `build/capypkg/`:

- `org.capyos.ui.widget-core-<version>.bin`
- `org.capyos.ui.widget-core.manifest`
- `org.capyos.ui.desktop-session-<version>.bin` (from migrated sources, or the `CAPYOS_DIR` fallback during transition)
- `org.capyos.ui.desktop-session.manifest` (from migrated sources, or the `CAPYOS_DIR` fallback during transition)
- `modules-index.txt`

Use the `modules-index.txt` asset as the CapyOS wizard/profile repository URL. The individual `.manifest` files are publisher inputs and GitHub source archives are not capypkg payloads.
