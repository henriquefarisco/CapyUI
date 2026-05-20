# CapyUI

Version: 0.7.2

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
