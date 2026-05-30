# CapyUI — Estado Absoluto do Projeto

Documento vivo, atualizado a cada release tag.

**Última atualização:** 2026-05-29
**Pacote atual:** `2.19.0` — advanced widget state: chart dataset (6ª fatia de _estado_ dos advanced widgets; date picker 2.14, color picker 2.15, table columns 2.16, autocomplete 2.17, tree hierarchy 2.18). ABI sobe para 2.19.
**ABI `capy-ui-widget`:** `2.19`. 14 minors 0.0–0.15 + 1.0 freeze + 1.1–1.10 (toda a fase 1.x linear, em LTS ≥12m) + 2.0 plugin system + SDK público + 2.1 advanced widgets + 2.2 virtualization plumbing + 2.3 undo/redo + 2.4 theme packs + 2.5 devtools/inspector + 2.6 display mode + 2.7 user management + 2.8 contrast/WCAG + 2.9 desktop icon arrangement + 2.10 file manager UX + 2.11 icon & thumbnail system + 2.12 wallpaper management + **2.13 login screen** + **2.14 date picker** + **2.15 color picker** + **2.16 table columns** + **2.17 autocomplete** + **2.18 tree hierarchy** + **2.19 chart dataset** (estado dos advanced widgets) compõem o surface ativo.
**ABI `capy-ui-desktop-session`:** `1` (entregue em `alpha.241`)
**Release gate:** `make validate` precisa rerodar externamente (lint + security + check-decoupling + **321 testes** + version-check; não executado neste workspace por política local). Gates de release adicionais para as tags `v1.0.0`–`v1.10.0` + `v2.0.0`–`v2.13.0`: 30d CI contínuo, soak 1000×, audit cross-repo CapyOS (rastreado em `tracking/cross-repo-sync-pending.md`; v1.5 1º bump DL schema; v1.6 DRAG_*; v1.7 slab; v1.8 IME_*; v1.9 2º bump DL schema TRANSFORM_*; v1.10 state serialization; v2.0 **major bump conservador** + 3º bump DL schema + plugin surface; v2.1 8 advanced widget enums; v2.2 virtualization plumbing + VIRTUAL_REQUEST_RANGE event; v2.3 undo/redo per-context; v2.4 theme packs binário + FNV-1a; v2.5 devtools/inspector + perf counters; v2.6 display mode plumbing + DISPLAY_MODE_CHANGED event; v2.7 user management plumbing + capy_user_account/directory; v2.8 contrast/WCAG + capy_contrast_finding + default_dark_high_contrast factory; v2.9 desktop icon arrangement + CDLA blob + FNV-1a; v2.10 file manager UX plumbing + capy_breadcrumb_truncate + toolbar taxonomy; v2.11 icon & thumbnail system + capy_icon_provider + THUMBNAIL_READY event; v2.12 wallpaper management + CWLP blob + slideshow + per-monitor; v2.13 login screen plumbing + capy_login_choose_layout + power taxonomy), benchmarks 100k widgets/16 ms (v1.7), engine IME real (v1.8), backend affine-aware (v1.9), crash recovery (v1.10), backend PLUGIN_OP-aware (v2.0), CapyOS a11y mapper para advanced widget types (v2.1), data source bridge para LIST/TREE/TABLE viewports (v2.2), text-edit auto-hook do undo stack (v2.3), hot reload + Ed25519 signature dos theme packs (v2.4), dump textual + event recording/replay + headless harness para devtools (v2.5), backend mode-setting CapyOS (DRM/KMS adapter) + Settings→Display app + rollback UI 15s + persistência per-user para display mode (v2.6), backend de autenticação CapyOS + Settings→Users app + groups/roles + audit log para user management (v2.7), WCAG 3 APCA + large-text threshold + per-locale palettes + Settings→Appearance audit panel para contrast (v2.8), drag + snap + auto-arrange + sort + pinned + multi-monitor envelope no desktop-session para desktop layout (v2.9), variable-width per-segment measurement + multi-line breadcrumb + locale-aware ellipsis + per-item toolbar descriptors para file manager (v2.10), icon theme cascade + vector pipeline + cancellation + stack/group folder descriptors para icon system (v2.11), live wallpaper + per-locale defaults + transition effects + decode/scaling pipeline para wallpaper (v2.12), biometric/SSO + PIN keypad + locked-badge animation + wake-screen para login (v2.13), tags assinadas, comunicado público de major release. CD via `.github/workflows/release-artifacts.yml`.
**Checkpoint consolidado:** `CHECKPOINT-v0.6.0.md` (curto prazo); v0.7.x–v0.15.0, v1.0.0–v1.10.0, v2.0.0–v2.13.0 documentados em `tracking/CHANGELOG.md`.

## Pipeline

- **Em produção (commit-side):** `2.15.0` (tags externas pendentes). **Fase 1.x linear CONCLUÍDA + fase 2.x major CONCLUÍDA (14/14)** + **trilha 2.14+ de estado dos advanced widgets** (date picker + color picker entregues).
- **Em validação:** gates externos para `v1.0.0–v1.10.0` + `v2.0.0`–`v2.13.0` (30d CI verde, soak 1000×, audit cross-repo, tags assinadas, comunicado público de major release com política de LTS 1.x).
- **Trilha aditiva ativa:** estado dos advanced widgets (2.14 date picker + 2.15 color picker entregues; próximas: TABLE/TREE/CHART/AUTOCOMPLETE/RICH_TEXT/CANVAS).
- **Próximo grande marco:** **fase 3.x (Vision platform)** — quebra controlada via novo major bump 3.0 (DL schema 9, sketch surface, GPU integration, cross-device handoff). **Gated por ADR-0006:** qualquer remoção exige deprecação ≥2 minors antes (hoje nada deprecado).
- **Extensões aditivas pendentes:** multi-touch (1.4), DPI layout (1.5), `CAPY_DL_DRAG_PREVIEW` (1.6), compression + hash-cache (1.7), `CAPY_DL_CANDIDATE_WINDOW` (1.8), arbitrary-angle rotation + hit-test (1.9), state serialization rico (1.10), plugin callback dispatch (2.0), per-widget state APIs (2.1), viewport emit + row recycling (2.2), undo coalescing + text-edit auto-hook (2.3), hot reload + Ed25519 signature + font/icon refs em theme packs (2.4), dump textual + event recording/replay + headless harness para devtools (2.5), fractional refresh + HDR/VRR + rollback UI 15s + persistência per-user para display mode (2.6), authentication + groups/roles + audit log para user management (2.7), WCAG 3 APCA + large-text threshold + per-locale palettes para contrast (2.8), multi-monitor envelope + group folders + locale collation + drag hit-test in core para desktop layout (2.9), variable-width measurement + multi-line + locale-aware ellipsis + per-item descriptors para file manager (2.10), icon theme cascade + vector + cancellation + group folder stacks para icon system (2.11), live wallpaper + per-locale defaults + transition effects para wallpaper (2.12), biometric/SSO + PIN keypad + locked-badge animation + wake-screen para login (2.13).
- **Bloqueios externos:** Etapa 4 (só afeta adapter CapyOS, v0.7), Etapa 6 (só afeta shell, v0.12). Núcleo retido livre.

## Cobertura por fase

| Fase | Faixa | Concluído | Em andamento | Pendente |
|------|-------|-----------|--------------|----------|
| Baseline | v0.0.1 | 1 | 0 | 0 |
| Curto | v0.1–v0.6 | **6 (TODAS)** | 0 | 0 |
| Packaging / desktop migration | v0.7.0–v0.7.3 | **4** | 0 | 0 |
| Médio | v0.8–v1.0 | **8** (v0.8.0, v0.9.0, v0.10.0, v0.11.0, v0.13.0, v0.14.0, v0.15.0, **v1.0.0**) | 0 | 0 (v0.7 + v0.12 reservadas externamente sobre 1.x já congelado) |
| Longo 1.x linear | v1.1–v1.10 | **10 (TODAS)** (v1.1.0–v1.10.0; em LTS ≥12m) | 0 | 0 |
| 2.x major | v2.0–v2.13 | **14 (TODAS)** (v2.0.0 major bump, v2.1.0 advanced widgets, v2.2.0 virtualization plumbing, v2.3.0 undo/redo, v2.4.0 theme packs, v2.5.0 devtools/inspector, v2.6.0 display mode plumbing, v2.7.0 user management plumbing, v2.8.0 contrast/WCAG refinement, v2.9.0 desktop icon arrangement, v2.10.0 file manager UX plumbing, v2.11.0 icon & thumbnail system, v2.12.0 wallpaper management, **v2.13.0 login screen plumbing**) | 0 | **0 (fase completa)** |

> **Marco:** **FASE 2.x COMPLETA — ABI `capy-ui-widget` v2.13 entregue (décima quarta e última fatia 2.x)**. Curto prazo (v0.0.1→v0.6.0), migração de ownership do desktop (v0.7.0→v0.7.3), médio prazo completo (v0.8–v0.15), congelamento (v1.0.0), **dez minors pós-freeze (v1.1–1.10)**, **v2.0–2.13** (plugin system + SDK, advanced widgets, virtualization plumbing, undo/redo, theme packs, devtools/inspector, display mode plumbing, user management plumbing, contrast/WCAG refinement, desktop icon arrangement, file manager UX plumbing, icon & thumbnail system, wallpaper management, login screen plumbing) entregues — **fase 2.x major completa 14/14**. Política de deprecation ARMADA para 2.x minors; 1.x em LTS ≥12m.

## Métricas-chave (snapshot)

- Testes: **321** funções em `tests/test_widget_contracts.c` (… + 6 v2.13 + 8 v2.14 + 8 v2.15 + 8 v2.16 + 8 v2.17 + 8 v2.18 + 8 v2.19). Contagem pinada em `make version-check` (defs == call-sites).
- LOC core (`src/widget/*.c` + `*.h`): ~7.9k.
- LOC desktop-session (`src/desktop/` + `src/window/` + `src/apps/`, inc. internos): ~12.5k (cobertura local 0 — só compila via cross-repo CapyOS; ver ADR-0007).
- LOC tests: ~7.6k.
- ABI `capy-ui-widget` ativas: 0.0–0.15 (com 0.7 e 0.12 reservadas), 1.0–1.10 (toda fase 1.x linear, em LTS ≥12m), 2.0–2.18, **2.19 (atual, aditivo)**.
- ABI `capy-ui-desktop-session` ativas: 1.
- Display-list schema: **7** (sem mudança em 2.1–2.13; bumps 4→5 em 1.5, 5→6 em 1.9, 6→7 em 2.0).
- Módulos publicáveis: 2 (`org.capyos.ui.widget-core`, `org.capyos.ui.desktop-session`).
- Etapa CapyOS bloqueante: **Etapa 4** só para v0.7 adapter (CapyOS-side); **Etapa 6** só para v0.12 shell. Núcleo retido e diff de compositor não bloqueados.
- CI green streak: contínuo desde `v0.0.1`.

## Diretrizes inquebáveis

- Núcleo portátil C11 estrito, zero malloc interno após init, zero I/O.
- Adições ABI aditivas até 1.0; deprecação ≥2 minor releases pós-1.0.
- `make validate` verde obrigatório em qualquer merge.
- Nenhum include de runtime CapyOS dentro de `src/widget/`.
- Zero float em hot paths.
- Determinismo: mesma entrada → mesma saída byte-a-byte.
- Manifesto `org.capyos.ui.desktop-session` **deve** declarar `depends=org.capyos.ui.widget-core`.

## Pendências críticas

Nenhuma. **Curto + migração do desktop + fase média completa + freeze 1.0 + 1.1–1.10 + v2.0–v2.13** entregues. **Fase 1.x linear CONCLUÍDA + fase 2.x major CONCLUÍDA (14/14)**. Gates externos rodam no pipeline de release; sync cross-repo CapyOS rastreado em `tracking/cross-repo-sync-pending.md` (v1.5 DL schema 4→5; v1.6 DRAG_*; v1.7 slab; v1.8 IME_*; v1.9 DL schema 5→6 + TRANSFORM_*; v1.10 state serialization; v2.0 **DL schema 6→7 + plugin system + LTS 1.x**; v2.1 8 advanced widget enum slots; v2.2 virtualization plumbing + VIRTUAL_REQUEST_RANGE; v2.3 undo/redo per-context; v2.4 theme packs binário + FNV-1a; v2.5 devtools/inspector read-only; v2.6 display mode plumbing + DISPLAY_MODE_CHANGED event; v2.7 user management plumbing + capy_user_account/directory; v2.8 contrast/WCAG refinement + capy_contrast_finding + 4ª factory; v2.9 desktop icon arrangement + CDLA blob + FNV-1a; v2.10 file manager UX plumbing + capy_breadcrumb_truncate + toolbar taxonomy; v2.11 icon & thumbnail system + capy_icon_provider + THUMBNAIL_READY event).

## Histórico de releases

Ver `CHANGELOG.md`.

## Política de atualização deste arquivo

Atualizar a cada tag de release com:
1. Versão atual e ABI.
2. Tabela de cobertura.
3. Métricas-chave atualizadas (rodar `wc -l` sobre `src/`, `tests/`).
4. Status dos bloqueios.
5. Próxima fatia.

Não substituir histórico — preservar como `# Release 0.X.Y` blocos quando relevante.
