# CapyUI — Métricas

**Última atualização:** 2026-05-19 (snapshot pós `v0.6.0` — **fase curto prazo completa**)

## Testes

- **Total:** 47 funções de teste
- **Arquivo:** `tests/test_widget_contracts.c`
- **Cobertura por área:**
  - Widget tree + eventos: 2 (`test_create_find_and_click`, `test_disabled_widget_ignores_input`)
  - Layout: 4 (`test_layout_stack_equal_grow`, `test_layout_grid_2x2`, `test_layout_idempotent`, `test_layout_min_max_constraints`)
  - Display-list: 4 (`test_displaylist_emit_simple_tree`, `test_displaylist_clip_balance`, `test_displaylist_overflow_rollback`, `test_displaylist_reset_and_text`)
  - Focus traversal (since 0.3): 5 (`test_focus_defaults_and_set`, `test_focus_traversal_order`, `test_focus_tab_index_priority`, `test_focus_skip_non_traversable`, `test_focus_prev_wrap`)
  - Dispatch key (since 0.3): 4 (`test_dispatch_key_tab_cycles`, `test_dispatch_key_enter_button`, `test_dispatch_key_space_checkbox`, `test_dispatch_key_escape_dialog`)
  - Focus ring (display-list, since 0.3): 1 (`test_displaylist_focus_ring`)
  - Text editing (since 0.4): 9 (`test_text_edit_insert_basic`, `test_text_edit_insert_middle_utf8`, `test_text_edit_insert_overflow`, `test_text_edit_delete_selection`, `test_text_edit_backspace_utf8`, `test_text_edit_readonly_blocks_insert`, `test_text_edit_ime_stub`, `test_text_edit_utf8_invalid_rejected`, `test_text_edit_determinism`)
  - Password rendering (display-list, since 0.4): 1 (`test_displaylist_password_mode`)
  - Animation (since 0.5): 9 (`test_anim_sample_start_returns_from`, `test_anim_sample_end_returns_to`, `test_anim_sample_inactive_returns_from`, `test_anim_linear_midpoint`, `test_anim_easings_monotonic_and_endpoints`, `test_anim_now_rewound_stable`, `test_anim_zero_duration_returns_from`, `test_anim_multiple_independent`, `test_anim_determinism`)
  - Widget tick (since 0.5): 1 (`test_widget_tick_walks_tree`)
  - Theme tokens (since 0.6): 4 (`test_theme_defaults_snapshot`, `test_theme_apply_stores_in_context`, `test_theme_resolve`, `test_theme_high_contrast_meets_wcag_aa`)
  - Theme rendering (display-list, since 0.6): 3 (`test_displaylist_uses_theme_when_token_set`, `test_displaylist_backward_compat_with_literal`, `test_displaylist_theme_change_preserves_structure`)

## Linhas de código

- **Core (`src/widget/`):** estimado ~2.3k LOC
  - `capy_widget.h` + `.c`
  - `capy_layout.h` + `.c`
  - `capy_display_list.h` + `.c`
- **Tests (`tests/`):** ~1280 LOC

## ABI

- **Master `capy-ui-widget`:** 0.6 (aditivo)
- **Display-list schema:** 2 (sem mudança em v0.6; theme é ponteiro opcional na struct)
- **Versions ativas:** 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6
- **Próxima:** 0.7 (adapter CapyOS — gate Etapa 4)

## Performance budget (placeholder)

Será definido em `v0.15.0` com baseline mensurável. Alvos preliminares:

- 10000 widgets — `measure + arrange + emit` < 16ms em host x86_64 desktop.
- Display-list overflow rollback em O(1).
- Sem alocação dinâmica em hot path após init.

## Métricas pendentes (a coletar)

- Tempo de `make validate` (target: < 10s).
- Tamanho do binário de teste (target: < 100KB).
- Cobertura de linhas (target: > 80% para core).
- Cobertura de branches.
- Latência p99 de emit em árvore de 1000 widgets.

## Política de atualização

Atualizar:
- A cada release tag (`v*`).
- Quando adicionar nova suite de testes.
- Quando mudar performance budget em release dedicada (`v0.15`, `v1.7`).

Histórico preservado em commits Git; este arquivo reflete o **snapshot atual**.
