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
 * The roundtrip set covers the five basic desktop apps, each exercising its
 * primary logic headlessly (no window/compositor): calculator (calc_eval),
 * task_manager (task_iter/process_iter enumeration), file_manager (path
 * join/compare/containment helpers), text_editor (handle_key buffer edits) and
 * settings (username-policy validator). REQUIRED_APPS on the CapyOS side must
 * match apps_smoke_roundtrip_total().
 */
#include "apps/apps_smoke.h"
#include "apps/calculator.h"
#include "apps/task_manager.h"
#include "apps/file_manager.h"
#include "apps/text_editor.h"
#include "apps/settings.h"

unsigned apps_smoke_roundtrip_total(void) {
  return 5u; /* calculator, task_manager, file_manager, text_editor, settings */
}

int apps_smoke_roundtrip_run(unsigned index) {
  switch (index) {
  case 0u:
    return calculator_smoke_roundtrip();
  case 1u:
    return task_manager_smoke_roundtrip();
  case 2u:
    return file_manager_smoke_roundtrip();
  case 3u:
    return text_editor_smoke_roundtrip();
  case 4u:
    return settings_smoke_roundtrip();
  default:
    return -1; /* out of range -> failure */
  }
}
