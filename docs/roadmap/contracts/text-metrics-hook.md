# Contrato: Text metrics hook

**Since:** 0.9.0 (planned)
**Status:** planned
**Fatia:** `medium-term/v0.9-text-fonts-host.md`

## Definição

Callback opcional fornecido pelo host para medição de texto durante `capy_widget_measure`. Sem o hook, CapyUI usa fallback determinístico.

## Tipos

```c
typedef int (*capy_text_metrics_fn)(
    uint16_t font_id, uint8_t font_size,
    const char *text, uint16_t len,
    uint32_t *out_width, uint32_t *out_height,
    void *user_data);
```

## API

```c
void capy_widget_set_text_metrics_hook(
    struct capy_widget_context *ctx,
    capy_text_metrics_fn fn,
    void *user_data);
```

## Semântica

- Hook setado: `measure` chama hook em widgets com texto não-vazio.
- Hook NULL: fallback determinístico:
  - `width = len * font_size / 2`
  - `height = font_size`
- Hook deve retornar `0` em sucesso, `-1` em erro (`measure` então usa fallback).
- Hook **não deve alocar** (em contexto de hot path).
- Hook é **read-only** sobre `text` (caller fornece, callee não modifica).

## Display-list

Op `TEXT` ganha campo aditivo `uint16_t font_id` no payload (schema → 4). Resolução do glifo é responsabilidade do compositor/font loader; CapyUI apenas emite a referência.

## Invariantes

- Fallback sempre determinístico (mesma string + mesmo size → mesma largura).
- Hook do host pode ser não-determinístico (caches dinâmicos), mas isso é tradeoff do host.
- Layout reflui quando `font_id` muda (handled em `measure`).

## Regras de evolução

- Assinatura do callback é estável; campos novos via wrapper ou struct payload em versões futuras.
- `font_id = 0` sempre válido (= default font, host escolhe).
