# CapyUI — Status atual

Última atualização: 2026-05-19

## Versão

- **Entregue (tagged):** `0.6.0`
- **ABI atual:** `capy-ui-widget 0.6`
- **Display-list schema:** `2` (sem mudança em v0.6; theme é ponteiro opcional)
- **Release gate:** `make validate` **VERDE** em 2026-05-19 (lint + security + 47 testes + version-check).
- **Checkpoint consolidado:** `CHECKPOINT-v0.6.0.md`.
- **Próxima fatia:** `v0.7.0 — Adapter CapyOS` (ver `medium-term/v0.7-host-adapter-gate-etapa4.md`) — **bloqueada por Etapa 4 do CapyOS**

## Fase atual

**Curto prazo — COMPLETO**. Toda a fase de núcleo retido portátil (v0.0.1 → v0.6.0) está entregue: widget tree, layout, display-list, focus, text editing, animation, theme tokens.

**Médio prazo — aguardando gate**. v0.7.0 (adapter CapyOS) depende da conclusão da Etapa 4 do roadmap CapyOS. Trabalho de médio prazo só destrava após esse gate.

## Bloqueios

Nenhum. Ver `tracking/BLOCKERS.md`.

## Cobertura por fase

| Fase | Faixa | Entregue | Próxima |
|------|-------|----------|---------|
| Curto | v0.3–v0.6 | v0.3.0, v0.4.0, v0.5.0, v0.6.0 | — (fase completa) |
| Médio | v0.7–v1.0 | — | — |
| Longo | v1.1–v3.0 | — | — |

## Métricas (snapshot)

- Testes de contrato: **47** (em `tests/test_widget_contracts.c`)
- Linhas de código (core): ~`2.3k` (estimado)
- ABI versions ativas: `0.0`, `0.1`, `0.2`, `0.3`, `0.4`, `0.5`, `0.6`
- Etapa CapyOS bloqueante: **Etapa 4** (apenas para v0.7+; núcleo não bloqueado)

## Fatias já entregues

- `v0.0.1` — Baseline widget tree
- `v0.1.0` — Layout primitives (CAPY_LAYOUT_STACK/GRID/FLOW + measure/arrange)
- `v0.2.0` — Display-list output (7 ops, schema v1, emit determinístico)
- `v0.3.0` — Focus traversal (tab_index, focus_next/prev, dispatch_key TAB/ENTER/SPACE/ESC, FOCUS_RING op, schema v2)
- `v0.4.0` — Text editing (struct capy_text_edit, insert/delete/set_selection/copy, ime_compose stub, UTF-8 validator, password render, caret RECT)
- `v0.5.0` — Animation and timing (struct capy_anim + 4 easings em fixed-point 16.16, sample/start, widget_tick walker, zero float)
- `v0.6.0` — Theme tokens v2 (enum capy_token + struct capy_theme, default_light/dark/high_contrast, apply/resolve, tokens em style, theme opcional em display_list)

## Próximos passos imediatos

1. **Curto prazo COMPLETO.** Próximo trabalho (v0.7.0) aguarda gate CapyOS Etapa 4.
2. Enquanto o gate não liberar, opções paralelas válidas:
   - Antecipar trabalho em fatias longo-prazo independentes (v1.1 damage tracking, v1.3 rich animation, v1.5 multi-display, v1.10 serialization — todas operam apenas no núcleo retido).
   - Auditar e endurecer testes existentes (cobertura de edge cases, fixtures binarias).
   - Documentar guias de uso (exemplos de adapter, templates de app).
3. Quando Etapa 4 fechar: implementar `v0.7.0` (adapter CapyOS) e atualizar `tracking/CHANGELOG.md`.
4. Atualizar `STATUS.md` (este arquivo) com nova versão entregue após cada fatia.
