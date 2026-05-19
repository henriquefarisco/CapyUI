# Contrato: Focus traversal

**Since:** 0.3.0
**Status:** delivered
**Fatia:** `short-term/v0.3-focus-traversal.md`

## Definição

Modelo determinístico de foco com Tab/Shift+Tab e dispatch de teclas para o widget focado.

## Campos aditivos em `capy_widget`

```c
uint8_t focusable;
int16_t tab_index;
```

## APIs

```c
void capy_widget_set_focusable(struct capy_widget *w, int focusable, int16_t tab_index);
struct capy_widget *capy_widget_focus_next(struct capy_widget *root, struct capy_widget *current);
struct capy_widget *capy_widget_focus_prev(struct capy_widget *root, struct capy_widget *current);
int capy_widget_dispatch_key(struct capy_widget *root, const struct capy_widget_event *ev);
```

## Defaults por tipo

- **Focáveis por default:** BUTTON, TEXTBOX, CHECKBOX, LIST, MENU_ITEM, TABS.
- **Não focáveis:** LABEL, PANEL, PROGRESS, SCROLLBAR.

## Semântica

- Ordem de travessia: ascendente por `tab_index`; empate por ordem de inserção.
- Skip: widgets `enabled=0`, `visible=0`, `focusable=0`.
- Wrap-around: último → primeiro (next); primeiro → último (prev).
- `dispatch_key` consome: TAB, SHIFT+TAB, ENTER, SPACE, ESC, setas.
- ENTER em BUTTON dispara `on_click`.
- SPACE em CHECKBOX alterna `checked` + dispara `on_change`.

## Op aditiva no display-list

`CAPY_DL_FOCUS_RING` no schema 2 — emitido após `BORDER` e antes de `TEXT`, quando `widget->focused != 0`. Cor: `style.active_color`. `border_width` = (existente + 1) ou 2 quando widget não tem border.

## Invariantes

- Determinístico: mesma árvore + mesma sequência de keys → mesmo foco final.
- Apenas um widget focado por contexto.
- Foco persistente entre frames (não reseta entre emits).

## Regras de evolução

- `tab_index` é `int16_t`; range `-32768..32767`.
- `focusable=0` é default para tipos não-interativos; tipos interativos (BUTTON/TEXTBOX/CHECKBOX/LIST/MENU_ITEM/TABS) ganham `focusable=1` em `capy_widget_create`.
- `capy_widget_focus(widget)` (since 0.0.1) continua válida; não checa `focusable` (callers podem marcar widgets não-focaláveis se necessário).

## Testes que cobrem o contrato

- `test_focus_defaults_and_set` — defaults por tipo + `set_focusable` override.
- `test_focus_traversal_order` — ordem DFS quando todos tab_index são iguais.
- `test_focus_tab_index_priority` — tab_index sobrepõe ordem de inserção.
- `test_focus_skip_non_traversable` — disabled/invisible/non-focusable são ignorados.
- `test_focus_prev_wrap` — prev wrap-around.
- `test_dispatch_key_tab_cycles` — TAB e SHIFT+TAB ciclam corretamente.
- `test_dispatch_key_enter_button` — ENTER e SPACE em BUTTON disparam `on_click`.
- `test_dispatch_key_space_checkbox` — SPACE em CHECKBOX alterna `checked` + `on_change`.
- `test_dispatch_key_escape_dialog` — ESC em DIALOG dispara `on_click` (cancel).
- `test_displaylist_focus_ring` — emit emite `FOCUS_RING` quando widget focado.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.3.0 | Introdução do módulo |
