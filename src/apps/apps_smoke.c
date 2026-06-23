/*
 * src/apps/apps_smoke.c — Etapa 6 / Slice 6.6 apps-basic-roundtrip aggregator
 * (CapyUI side).
 *
 * Implements the CapyOS-owned contract `apps/apps_smoke.h`: runs each basic
 * app's headless primary-function smoke roundtrip and reports pass/fail per
 * app, so the CapyOS orchestrator (under CAPYOS_APPS_ROUNDTRIP_SMOKE) can count
 * clean passes and emit `[smoke] apps-basic-roundtrip ready`.
 *
 * The apps are in-kernel functions (compiled into the kernel ELF as part of the
 * desktop session), NOT ring-3 processes, so there is no process exit code; the
 * per-app `*_smoke_roundtrip()` functions exercise primary logic only (no
 * window/compositor) and return 0 on success.
 *
 * The roundtrip set grows app-by-app. The first milestone covers the calculator
 * (primary function fully separable). REQUIRED_APPS on the CapyOS side must
 * match apps_smoke_roundtrip_total().
 */
#include "apps/apps_smoke.h"
#include "apps/calculator.h"

unsigned apps_smoke_roundtrip_total(void) {
  return 1u; /* calculator */
}

int apps_smoke_roundtrip_run(unsigned index) {
  switch (index) {
  case 0u:
    return calculator_smoke_roundtrip();
  default:
    return -1; /* out of range -> failure */
  }
}
