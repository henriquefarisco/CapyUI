# CapyUI — Métricas

**Última atualização:** 2026-05-29 (snapshot pós `v2.14.0` — advanced widget state: date picker).

> Antes desta atualização o arquivo estava congelado no snapshot `v0.6.0`
> (47 testes / ABI 0.6 / schema 2). Os números abaixo refletem o estado real.

## Testes

- **Total:** **321** funções de teste em `tests/test_widget_contracts.c`.
- **Guard:** `make version-check` pina o total e exige `defs == call-sites em main()`,
  então drift silencioso (teste definido mas não registrado, ou contagem fora do esperado) falha o CI.
- **Breakdown por fase (resumo):** 97 (1.0 baseline) + 7×(1.1–1.5) + 8 (1.6) + 7×(1.7–1.10) + 8 (2.0) + 8 (2.1) + 7 (2.2) + 7 (2.3) + 8 (2.4) + 7×(2.5–2.7) + 8×(2.8–2.11) + 7 (2.12) + 6 (2.13) + 8 (2.14 date picker) + 8 (2.15 color picker) + 8 (2.16 table columns) + 8 (2.17 autocomplete) + 8 (2.18 tree hierarchy) + **8 (2.19 chart dataset)** = 321.
- **v2.14 (date picker):** `test_date_set_valid_and_get`, `test_date_unset_get_returns_zero`,
  `test_date_invalid_rejected_fail_closed`, `test_date_clear_resets`,
  `test_date_wrong_widget_type_rejected`, `test_date_is_valid_calendar_predicate`,
  `test_date_null_guards`, `test_date_determinism`.
- **v2.15 (color picker):** `test_color_pack_channel_order`, `test_color_pack_extremes`,
  `test_color_set_and_get`, `test_color_unset_get_returns_zero`, `test_color_clear_resets`,
  `test_color_wrong_widget_type_rejected`, `test_color_null_guards`, `test_color_determinism`.
- **v2.16 (table columns):** `test_table_set_and_query`, `test_table_index_out_of_range`,
  `test_table_set_zero_count_clears`, `test_table_set_null_widths_rejected`, `test_table_clear`,
  `test_table_wrong_widget_type_rejected`, `test_table_null_guards`, `test_table_default_no_columns`.
- **v2.17 (autocomplete):** `test_autocomplete_set_and_query`, `test_autocomplete_item_out_of_range`,
  `test_autocomplete_set_zero_count_clears`, `test_autocomplete_set_null_items_rejected`,
  `test_autocomplete_clear`, `test_autocomplete_selection_clamp`,
  `test_autocomplete_wrong_widget_type_rejected`, `test_autocomplete_null_guards`.
- **v2.18 (tree hierarchy):** `test_tree_collapsed_default_expanded`, `test_tree_set_collapsed_toggle`,
  `test_tree_depth_set_and_get`, `test_tree_row_visible_no_ancestors`,
  `test_tree_row_visible_collapsed_ancestor`, `test_tree_row_visible_nested`,
  `test_tree_wrong_widget_type_rejected`, `test_tree_null_guards`.
- **v2.19 (chart dataset):** `test_chart_set_and_query`, `test_chart_value_out_of_range`,
  `test_chart_set_zero_count_clears`, `test_chart_set_null_values_rejected`, `test_chart_clear`,
  `test_chart_range_minmax`, `test_chart_wrong_widget_type_rejected`, `test_chart_null_guards`.

## Linhas de código (snapshot)

- **Core portátil (`src/widget/*.c` + `*.h`):** ~7.9k LOC (5 fontes + 3 headers).
- **Desktop-session (`src/desktop/` + `src/window/` + `src/apps/`, inc. internos):** ~12.5k LOC.
  - **Cobertura local: 0** — só compila pelo caminho cross-repo CapyOS (ver `contracts/desktop-session-coupling.md` + ADR-0007).
- **Tests (`tests/`):** ~7.6k LOC.

## ABI

- **`capy-ui-widget`:** **2.19** (aditivo sobre 2.18). Macros `2/19/0`; `CAPYUI_API_VERSION_TAG = 0x00021300`.
- **`capy-ui-desktop-session`:** 1.
- **Display-list schema:** **7** (sem mudança desde 2.0; bumps 4→5 em 1.5, 5→6 em 1.9, 6→7 em 2.0).
- **Versões ativas:** 0.0–0.15 (0.7/0.12 reservadas), 1.0–1.10 (LTS ≥12m), 2.0–2.19.
- **Deprecações ativas:** 0 (macro `CAPYUI_API_DEPRECATED` publicada, nenhum símbolo anotado).

## Gates de CI

- `ci.yml` → `make validate` (lint + security + check-decoupling + 321 testes + version-check), ubuntu-latest.
- `security.yml` → CodeQL (`make test`) + hardened compile (`make security`).
- `abi-guard.yml` → `tools/abi_guard.sh` (remoção de símbolo público dentro do major = falha).
- `release-artifacts.yml` → `make validate` + `make package` + publica rolling `latest` / tags `v*`.

## Performance budget

- Alvo (placeholder histórico de 0.15): 10k widgets `measure+arrange+emit` < 16 ms em host x86_64.
- Benchmarks 100k widgets/16 ms (v1.7) ficam como gate externo (workspace não executa).

## Métricas pendentes (a coletar externamente)

- Tempo de `make validate` (alvo < 10s) e tamanho do binário de teste (< 100KB).
- Cobertura de linhas/branches do core (alvo > 80%).
- **Cobertura de build do desktop-session** (hoje zero local; medir via CapyOS `PROFILE=full`).

## Política de atualização

Atualizar a cada release tag (`v*`), ao adicionar suíte de testes, e ao mudar o performance budget.
Histórico preservado em commits Git; este arquivo reflete o snapshot atual.
