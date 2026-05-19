# Contrato: Input plumbing

**Since:** 0.10.0 (planned)
**Status:** planned
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
uint32_t modifiers;       /* bitmask CTRL/ALT/SHIFT/META */
void *payload;            /* depende do tipo de evento */
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

## Roteamento

- **WHEEL:** roteado para widget sob o cursor; LIST consome para scroll, propaga caso não consuma.
- **TOUCH_***: traduzido para POINTER_* quando único toque; gestos resolvidos em v1.4.
- **GAMEPAD:** dispatch para widget focado; mapeamento de botões configurável.

## Invariantes

- Eventos desconhecidos (futuros) ignorados sem crash.
- `modifiers` é bitmask aditivo; bits novos sempre no final.
- Touch IDs estáveis durante drag (BEGIN→MOVE→END com mesmo ID).
- POINTER continua sendo o mínimo obrigatório para todos hosts.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.10.0 (planned) | Introdução de WHEEL/TOUCH/GAMEPAD/modifiers |
