# CapyUI

Version: 2.19.0

CapyUI owns portable widget primitives and interaction contracts for CapyOS services. As of `1.0.0` the `capy-ui-widget` ABI is **frozen**: minor releases stay additive, breaking changes require a major bump per `docs/roadmap/contracts/deprecation-policy.md`.

## CapyOS reference version

Pinned for this release: `0.8.0-alpha.261+20260529` (authoritative pin: `docs/compatibility.md`). Since 0.7.0 CapyUI also publishes the `org.capyos.ui.desktop-session` capypkg module (compositor session, window manager, apps) in addition to `org.capyos.ui.widget-core`. Update this together with `docs/compatibility.md` whenever the CapyOS core version, ABI or canonical manifest format changes.

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

`make package` emits the assets consumed by the CapyOS first-boot wizard under `build/capypkg/`:

- `org.capyos.ui.widget-core.bin`
- `org.capyos.ui.widget-core.manifest`
- `org.capyos.ui.desktop-session.bin`
- `org.capyos.ui.desktop-session.manifest`
- `modules-index.txt`

Asset filenames are intentionally version-less inside each GitHub Release. The published `modules-index.txt` uses tag-pinned payload URLs:

```
https://github.com/<owner>/CapyUI/releases/download/v2.19.0/modules-index.txt
```

The semantic `version=` field inside each `.manifest` still comes from the `VERSION` file in this repo, so `capypkg` knows when to upgrade.

This Makefile **does not touch CapyOS sources**: the desktop session lives entirely in this repo after the alpha.241 migration.

## Automatic deploy

Every push to `main` triggers `.github/workflows/release-artifacts.yml`, which:

1. runs `make validate` and `make package`,
2. moves the rolling `latest` git tag to the current commit,
3. republishes the GitHub Release `latest` with the new `.bin/.manifest/modules-index.txt` assets and marks it as the latest release.

CapyOS first-boot releases should pin the index URL to a semver release tag, for example `https://github.com/<owner>/CapyUI/releases/download/v2.19.0/modules-index.txt`. This avoids an extra GitHub `latest` redirect during kernel-space module installation.

To freeze a semver release without affecting the rolling channel, push a `v*` tag (for example `v0.7.1`). The same workflow handles tag-triggered runs as standalone frozen releases.
