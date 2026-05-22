# Contrato: Input plumbing

**Since:** 0.10.0 (delivered)
**Status:** delivered (CapyUI side); host drivers e gesture recognizer CapyOS seguem trilha Etapa 3/4
**Fatia:** `medium-term/v0.10-input-plumbing.md`

## Eventos novos no enum `capy_widget_event_type`

```c
CAPY_WIDGET_EVENT_WHEEL
CAPY_WIDGET_EVENT_TOUCH_BEGIN
CAPY_WIDGET_EVENT_TOUCH_MOVE
CAPY_WIDGET_EVENT_TOUCH_END
CAPY_WIDGET_EVENT_GAMEPAD
```

## Campos aditivos em `capy_widget_event`

```c
uint32_t modifiers;       /* bitmask, since 0.3; CAPSLOCK/NUMLOCK since 0.10 */
const void *payload;      /* since 0.10; tipo concreto depende de event->type */
```

## Modificadores

```c
#define CAPY_MOD_NONE     0x0u
#define CAPY_MOD_CTRL     0x1u
#define CAPY_MOD_ALT      0x2u
#define CAPY_MOD_SHIFT    0x4u
#define CAPY_MOD_META     0x8u
#define CAPY_MOD_CAPSLOCK 0x10u
#define CAPY_MOD_NUMLOCK  0x20u
```

## Payloads

### WHEEL
```c
struct capy_wheel_payload {
  int16_t delta_x;
  int16_t delta_y;
};
```

### TOUCH_*
```c
struct capy_touch_payload {
  uint32_t touch_id;
  struct capy_ui_point pos;
  uint32_t pressure_x256;
};
```

### GAMEPAD
```c
struct capy_gamepad_payload {
  uint16_t button_mask;
  int16_t axis[4];
};
```

## Roteamento (CapyUI side desde 0.10)

- **WHEEL:** `capy_widget_handle_event` chama `capy_widget_find_at(root, event->x, event->y)` e, ao encontrar o widget sob o cursor, sobe pela cadeia de pais via `capy_widget_find_list_ancestor` até o primeiro `CAPY_WIDGET_LIST`; ajusta `list->value` por `payload->delta_y` clampado em `[min_value, max_value]` e dispara `on_change` quando o valor muda. Retorna `1` se uma lista absorveu o evento, `0` caso contrário. Sem lista ancestral, sem payload, ou lista invisible/disabled → `0` (não consumido).
- **TOUCH_BEGIN/MOVE/END:** traduzidos em-memória para `POINTER_DOWN`/`POINTER_MOVE`/`POINTER_UP` (touch único). Quando o `payload` contém `capy_touch_payload`, a posição do toque (`payload->pos.x/y`) **sobrescreve** `event->x/y` da chamada original — hosts podem deixar `event->x/y` zerados. O payload é zerado na versão sintetizada para não re-interpretar como POINTER. Gestos multi-touch chegam em v1.4.
- **GAMEPAD:** aceito silenciosamente (`return 0`). Mapeamento configurável é trabalho futuro; o caminho de entrega já existe para que hosts possam emitir eventos sem crash antes da ligação.
- **Tipos desconhecidos** (futuros, > `CAPY_WIDGET_EVENT_GAMEPAD`): ignorados via cascata padrão do dispatcher (retorna 0 sem crash).

## Invariantes

- Eventos desconhecidos (futuros) ignorados sem crash.
- `modifiers` é bitmask aditivo; bits novos sempre no final. Em 0.10, `CAPY_MOD_CAPSLOCK` (`0x10`) e `CAPY_MOD_NUMLOCK` (`0x20`) somam-se aos pré-existentes `CTRL/ALT/SHIFT/META` sem conflito.
- Touch IDs estáveis durante drag (BEGIN→MOVE→END com mesmo ID).
- POINTER continua sendo o mínimo obrigatório para todos hosts.
- `payload` é `const void *` — CapyUI nunca escreve através dele. Lifetime: hostchamador garante validade do payload durante a chamada de `capy_widget_handle_event`.

## Testes que cobrem o contrato

- `test_event_wheel_on_list_scrolls` — WHEEL em LIST ajusta `value` e dispara `on_change`.
- `test_event_wheel_clamps_to_range` — WHEEL clampa em `[min_value, max_value]`.
- `test_event_wheel_on_panel_returns_unhandled` — WHEEL fora de LIST retorna 0 sem efeito colateral.
- `test_event_touch_single_routes_as_pointer` — TOUCH_BEGIN sintético aciona `on_click` do botão.
- `test_event_touch_uses_payload_position_over_event_xy` — payload pos sobrescreve `event->x/y`.
- `test_event_gamepad_no_crash` — GAMEPAD com payload e botões retorna 0 silenciosamente.
- `test_event_unknown_type_ignored` — tipo fora do enum conhecido retorna 0.
- `test_event_modifier_flags_distinct` — todos os 6 bits de modifier são não-overlapping e cobrem `0x3F`.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.10.0 | Introdução de WHEEL/TOUCH/GAMEPAD/modifiers (CAPSLOCK/NUMLOCK), payloads tipados, `capy_ui_point`, campo `const void *payload` em `capy_widget_event`. Lado CapyUI entregue; drivers e gesture engine CapyOS seguem trilha Etapa 3/4. |
