# Security Policy

CapyUI is the portable widget core and desktop session for CapyOS. Since `v1.0.0` the `capy-ui-widget` ABI is frozen (additive-only minors; major bump required for breaking changes). The `capy-ui-desktop-session` ABI is `v1`. Vulnerabilities in either ABI are treated as release-gate blockers.

## Reporting a vulnerability

- Report privately to the repository owner before opening any public issue, pull request, or release note.
- Include a minimal reproduction (widget tree, input sequence, or display-list state) when possible.
- Allow up to 90 days for coordinated disclosure unless the issue is being actively exploited; emergency fixes follow the compressed deprecation window documented in `docs/roadmap/contracts/deprecation-policy.md` (security exception).
- Embargoed fixes ship as point releases (`1.x.y+1`) with the security advisory referenced in `docs/roadmap/tracking/CHANGELOG.md`.

## Supported versions

- `2.x` is the current additive line.
- `1.x` remains LTS and receives security fixes for **≥12 months** after `2.0.0` (commitment per `docs/roadmap/contracts/deprecation-policy.md`).
- `0.x` pre-1.0 lines are unsupported except via the CapyOS host-side compatibility adapter; consumers should upgrade to a supported line.
- `capy-ui-desktop-session v1` follows the same LTS window aligned to its host CapyOS Etapa.

## Release gate

- `make validate` must pass (lint + `-D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE` security build + full host test suite + `version-check`) before any tag is pushed.
- Widget input must respect `visible` and `enabled` state — every event handler ignores widgets that fail either flag.
- The display-list, focus, animation, text edit, locale, theme serialize, a11y snapshot, input plumbing and pool/cache subsystems must all remain **deterministic byte-for-byte** under identical inputs. Determinism regressions are treated as security issues because they break audit trails and reproducible smoke smokes downstream.
- No `float` / `double` is allowed in `src/widget/`; introduction of platform-dependent rounding would be a release blocker.
- No CapyOS runtime header may be included from `src/widget/` (cross-repo decoupling discipline).

## Build hardening

The CapyUI Makefile compiles the widget core with `-Wall -Wextra -Werror -pedantic -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE`. Downstream consumers (the desktop-session binaries shipped through `org.capyos.ui.desktop-session`) are expected to inherit the same flags from CapyOS.

## Supply chain

The CapyAgent Ed25519 signer and the CapyOS verifier adapter exist, but production trust remains fail-closed until the publisher public key is pinned and the external known-answer test passes. At that promotion point, `org.capyos.ui.widget-core` and `org.capyos.ui.desktop-session` manifests will publish `signature_ed25519` over the canonical descriptor `name=N|version=V|payload_sha256=H|payload_url=U\n`. Until then, unsigned installation is restricted to lab environments.
