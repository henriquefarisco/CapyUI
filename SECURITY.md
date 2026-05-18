# Security Policy

CapyUI 0.0.1 is an early service release. Report security issues privately to the repository owner before opening public issues.

## Release gate

- `make validate` must pass before release tags.
- Widget input must respect visibility and enabled state.
- Build gates use strict C warnings and hardened compile flags.
