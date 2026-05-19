# CapyUI — Changelog

Mudanças por release tag, da mais recente para a mais antiga. Cada entrada é imutável após release.

## [0.6.0] — 2026-05-19

**Marco:** fase de curto prazo (núcleo retido portátil) completa. Próximo trabalho (v0.7.0) aguarda gate CapyOS Etapa 4.

### Adicionado

- Sistema de tokens de tema v2 em `src/widget/capy_widget.c` e `.h`.
- `enum capy_token` com 17 entries: `CAPY_TOKEN_NONE` (reservado para "use literal"), `CAPY_TOKEN_BG_BASE/RAISED/SUNKEN`, `CAPY_TOKEN_FG_PRIMARY/MUTED/INVERSE`, `CAPY_TOKEN_ACCENT/ACCENT_HOVER`, `CAPY_TOKEN_BORDER/BORDER_FOCUS`, `CAPY_TOKEN_FOCUS_RING`, `CAPY_TOKEN_DANGER/WARNING/SUCCESS/INFO`, `CAPY_TOKEN_DISABLED`, `CAPY_TOKEN_COUNT`.
- Constantes de variant: `CAPY_THEME_VARIANT_LIGHT/DARK/HIGH_CONTRAST`.
- `struct capy_theme` (standalone): `tokens[CAPY_TOKEN_COUNT]`, `variant`, `high_contrast`, `version`.
- Campo aditivo `theme` em `struct capy_widget_context` (embutido por valor).
- Campos aditivos `bg_token`, `fg_token`, `border_token`, `reserved` em `struct capy_widget_style` (`uint8_t` cada).
- Ponteiro aditivo `const struct capy_theme *theme` em `struct capy_display_list` (NULL por default, set explícito para opt-in de theming).
- APIs públicas: `capy_theme_default_light`, `capy_theme_default_dark`, `capy_theme_high_contrast`, `capy_theme_apply`, `capy_theme_resolve`.
- Default themes documentados: light/dark com paleta moderna; high-contrast com BG=#000000 e FG=#FFFFFF (contraste 21:1, trivial WCAG AA).
- `capy_widget_emit_recursive` resolve cores via `capy_theme_resolve(dl->theme, w->style.*_token, w->style.*_color)` para RECT, BORDER, TEXT e caret RECT. FOCUS_RING mantém `style.active_color` literal.
- Backward compat: callers pré-0.6 (sem tokens setados, sem theme em dl) produzem display-list byte-idêntico ao pré-0.6.
- 7 novos testes em `tests/test_widget_contracts.c` cobrindo snapshot dos 3 defaults, apply em contexto, resolver edge cases (NULL/0/out-of-range), WCAG AA high-contrast, uso de theme quando token setado, backward compat com literal, troca de tema preservando estrutura de ops.

### ABI

- `capy-ui-widget` 0.5 → **0.6** (aditivo).
- Display-list schema permanece **2**. Theme é ponteiro opcional, não introduz nova op.

### Docs

- `docs/compatibility.md` ganhou bloco "Theme tokens v2 (since 0.6)".
- `docs/README.md` marca fatia 6 como entregue e curto prazo completo.
- `docs/roadmap/short-term/v0.6-theme-tokens-v2.md` marcado como `delivered`.

## [0.5.0] — 2026-05-19

### Adicionado

- Modelo de animação determinístico em `src/widget/capy_widget.c` e `.h`.
- `enum capy_anim_easing` com 4 valores: `CAPY_ANIM_LINEAR`, `CAPY_ANIM_EASE_IN`, `CAPY_ANIM_EASE_OUT`, `CAPY_ANIM_EASE_IN_OUT`.
- `struct capy_anim` standalone (não embutida em `capy_widget` em 0.5): campos `start_tick`, `duration_ticks`, `from`, `to`, `easing`, `active`, `reserved`.
- APIs: `capy_anim_start`, `capy_anim_sample`, `capy_widget_tick`.
- Easings implementadas em fixed-point 16.16 com intermediários `uint64_t`; **zero `float`/`double`** em qualquer hot path.
- Defesa contra clock retrocedido (`now <= start_tick` → retorna `from`) e duração zero (retorna `from`).
- `capy_widget_tick(root, now)` faz DFS walk determinístico; em 0.5 é no-op por widget (hook para futuras integrações).
- 10 novos testes em `tests/test_widget_contracts.c` cobrindo endpoints (start/end), inativo, duração zero, midpoint linear, monotonicidade das 4 easings, clock retrocedido, múltiplas animações independentes, determinismo e walk de árvore.

### ABI

- `capy-ui-widget` 0.4 → **0.5** (aditivo).
- Display-list schema permanece **2** (animação opera no modelo retido, fora do display-list).

### Docs

- `docs/compatibility.md` ganhou bloco "Animation and timing (since 0.5)".
- `docs/README.md` marca fatia 5 como entregue.
- `docs/roadmap/short-term/v0.5-animation-timing.md` marcado como `delivered`.

## [0.4.0] — 2026-05-19

### Adicionado

- Modelo de edição de texto em `src/widget/capy_widget.c` e `.h`.
- `struct capy_text_edit` (aditiva, embutida no final de `capy_widget`) com campos `caret`, `sel_start`, `sel_end`, `multiline`, `readonly`, `password`, `reserved`. Válida quando `type == TEXTBOX`.
- APIs: `capy_textbox_insert`, `capy_textbox_delete` (positivo = forward, negativo = backspace, ambos em UTF-8 chars), `capy_textbox_set_selection`, `capy_textbox_copy`, `capy_widget_ime_compose` (stub).
- Helpers internos: validador UTF-8 mínimo, `capy_utf8_prev_char_size`, `capy_utf8_char_size_at`, `capy_text_shift_left/right`, `capy_textbox_clamp_caret`.
- Selection ativa é substituída ao inserir; ao deletar com seleção, conteúdo selecionado é removido (count ignorado).
- `readonly = 1` bloqueia `insert` e `delete` (retornam `-1`); `copy` continua funcionando.
- `password = 1` emite `*` no display-list TEXT (um por caractere UTF-8); bytes originais nunca tocam o text_pool.
- Caret rendering: pequeno RECT (1 px de largura, altura = `font_size`) após `TEXT` para TEXTBOX `focused && !readonly && !password`. Posição usa fallback `caret * font_size / 2`.
- 10 novos testes em `tests/test_widget_contracts.c` cobrindo insert ASCII/UTF-8, overflow, delete de seleção, backspace UTF-8, password rendering, readonly, IME stub, UTF-8 inválido e determinismo.

### ABI

- `capy-ui-widget` 0.3 → **0.4** (aditivo).
- Display-list schema permanece **2** (password e caret usam `TEXT` e `RECT` existentes; sem op nova).

### Docs

- `docs/compatibility.md` ganhou bloco "Text editing (since 0.4)".
- `docs/README.md` marca fatia 4 como entregue.
- `docs/roadmap/short-term/v0.4-text-editing.md` marcado como `delivered`.

## [0.3.0] — 2026-05-19

### Adicionado

- Focus traversal em `src/widget/capy_widget.c` e `.h`.
- Campos aditivos em `struct capy_widget`: `uint8_t focusable`, `int16_t tab_index`.
- Campo aditivo em `struct capy_widget_event`: `uint32_t modifiers`.
- Constantes: `CAPY_KEY_TAB`, `CAPY_KEY_ENTER`, `CAPY_KEY_ESCAPE`, `CAPY_KEY_SPACE`, `CAPY_KEY_ARROW_UP/DOWN/LEFT/RIGHT`.
- Constantes de modificadores: `CAPY_MOD_NONE`, `CAPY_MOD_CTRL`, `CAPY_MOD_ALT`, `CAPY_MOD_SHIFT`, `CAPY_MOD_META`.
- APIs: `capy_widget_set_focusable`, `capy_widget_focus_next`, `capy_widget_focus_prev`, `capy_widget_clear_focus`, `capy_widget_get_focused`, `capy_widget_dispatch_key`.
- Algoritmo 2-pass DFS preorder com ordenação lexicográfica `(tab_index, dfs_order)` para travessia determinística.
- Defaults de focusable por tipo: `BUTTON`, `TEXTBOX`, `CHECKBOX`, `LIST`, `MENU_ITEM`, `TABS`.
- Op aditiva `CAPY_DL_FOCUS_RING` no display-list (schema 1 → 2). Emit após `BORDER` e antes de `TEXT` quando `widget->focused != 0`.
- 9 novos testes em `tests/test_widget_contracts.c` cobrindo defaults, traversal order, tab_index priority, skip non-traversable, prev wrap, dispatch TAB/SHIFT+TAB/ENTER/SPACE/ESC, e emissão de focus ring.

### ABI

- `capy-ui-widget` 0.2 → **0.3** (aditivo).
- Display-list schema 1 → **2** (aditivo).

### Docs

- `docs/compatibility.md` ganhou bloco "Focus traversal (since 0.3)".
- `docs/README.md` marca fatia 3 como entregue.
- `docs/roadmap/short-term/v0.3-focus-traversal.md` marcado como `delivered`.

## [0.2.0] — 2026-05-19

### Adicionado

- Display-list output em `src/widget/capy_display_list.h` e `.c`.
- `enum capy_dl_op` com 7 entries: `NONE`, `RECT`, `BORDER`, `TEXT`, `CLIP_PUSH`, `CLIP_POP`, `IMAGE_REF`.
- `struct capy_dl_cmd` e `struct capy_display_list` com schema versionado (`CAPY_DISPLAY_LIST_SCHEMA_VERSION = 1`).
- APIs `capy_display_list_init`, `capy_display_list_reset`, `capy_widget_emit`.
- Buffers de comandos e texto fornecidos pelo chamador (zero malloc interno).
- Emit determinístico com rollback em overflow.
- 4 novos testes em `tests/test_widget_contracts.c` cobrindo emit de árvore simples, balanço de clip push/pop, rollback em overflow e reset/texto.

### ABI

- `capy-ui-widget` 0.1 → **0.2** (aditivo).

### Docs

- `docs/compatibility.md` ganhou bloco "Display-list schema (since 0.2)".
- `docs/README.md` marca fatia 2 como entregue.

## [0.1.0] — 2026

### Adicionado

- Layout primitives em `src/widget/capy_layout.h` e `.c`.
- `enum capy_layout_kind`: `NONE`, `STACK`, `GRID`, `FLOW`.
- `struct capy_layout_constraints` e `capy_layout_node` embutidos em `capy_widget`.
- APIs `capy_widget_set_layout`, `capy_widget_measure`, `capy_widget_arrange`.
- 4 testes cobrindo stack vertical, grid 2x2, idempotência e constraints min/max.

### ABI

- `capy-ui-widget` 0.0 → **0.1** (aditivo).

### Docs

- `docs/compatibility.md` ganhou bloco "Layout contract (since 0.1)".

## [0.0.1] — 2026

### Adicionado

- Baseline do widget tree em `src/widget/capy_widget.h` e `.c`.
- Tipos de widget: `LABEL`, `BUTTON`, `TEXTBOX`, `CHECKBOX`, `LIST`, `PANEL`, `SCROLLBAR`, `MENUBAR`, `MENU_ITEM`, `DIALOG`, `PROGRESS`, `TABS`.
- Eventos abstratos: `POINTER_MOVE/DOWN/UP`, `KEY_DOWN/UP`.
- `capy_widget_style` com presets default/button/textbox.
- Allocator injetável `capy_widget_allocator`.
- Hit-testing `capy_widget_find_at` + roteamento `capy_widget_handle_event`.
- 2 testes baseline em `tests/test_widget_contracts.c`.
- `make validate` (lint + security + test + version-check) no Makefile.
- CI: `ci.yml` e `security.yml`.
- Compatibility doc: `docs/compatibility.md`, `docs/README.md`.

### ABI

- `capy-ui-widget` **0.0** (estabelecida).

## Formato

Seguir [Keep a Changelog](https://keepachangelog.com/) com seções `Adicionado`, `Mudado`, `Deprecado`, `Removido`, `Corrigido`, `Segurança` e bloco `ABI` ao final.

## Política

- Entradas imutáveis após release tag.
- Apenas a release de cabeçalho mais novo pode ser "Unreleased"; remover ao tagar.
- Cada entrada cita arquivos novos/modificados quando relevante.
