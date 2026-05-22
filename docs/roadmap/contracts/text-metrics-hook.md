# Contrato: Text metrics hook

**Since:** 0.9.0 (delivered)
**Status:** delivered (CapyUI side); host font loader CapyOS segue trilha Etapa 4
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

- API pública: `int capy_widget_measure_text(const struct capy_widget_context *ctx, uint16_t font_id, uint8_t font_size, const char *text, uint16_t len, uint32_t *out_width, uint32_t *out_height);`
- Setup: `void capy_widget_set_text_metrics_hook(struct capy_widget_context *ctx, capy_text_metrics_fn fn, void *user_data);` (passar `fn == NULL` limpa).
- Hook setado e `rc == 0`: `*out_width`/`*out_height` vêm do hook.
- Hook NULL ou hook setado retornando `-1`: fallback determinístico
  - `effective_size = (font_size > 0 ? font_size : 16)`
  - `*out_width = (effective_size / 2) * len`
  - `*out_height = effective_size`
- `text == NULL` ou `len == 0`: `*out_width = 0`, `*out_height = effective_size`. Hook **não é** chamado neste caso (curto-circuito determinístico).
- `out_width == NULL` ou `out_height == NULL`: `capy_widget_measure_text` retorna `-1` sem chamar o hook.
- Hook deve retornar `0` em sucesso, `-1` em erro.
- Hook **não deve alocar** (hot path).
- Hook é **read-only** sobre `text`.

## Display-list

Op `TEXT` ganha campo aditivo `uint16_t font_id` no payload (DL schema 3 → 4 — layout-compatível: rename do antigo `uint16_t reserved`). `capy_widget_emit` copia `widget->font_id` (campo aditivo no tail de `capy_widget`) para cada `CAPY_DL_TEXT`. Resolução do glifo é responsabilidade do compositor/font loader; CapyUI apenas emite a referência.

## Invariantes

- Fallback sempre determinístico (mesma string + mesmo size → mesma largura).
- Hook do host pode ser não-determinístico (caches dinâmicos), mas isso é tradeoff do host.
- Layout reflui quando `font_id` muda (handled em `measure`).

## Regras de evolução

- Assinatura do callback é estável; campos novos via wrapper ou struct payload em versões futuras.
- `font_id = 0` sempre válido (= default font, host escolhe).

## Testes que cobrem o contrato

- `test_text_metrics_fallback_deterministic` — fallback exato, edge cases (font_size 0, len 0, NULL out args).
- `test_text_metrics_hook_called_when_set` — hook recebe argumentos corretos e seus outputs são devolvidos.
- `test_text_metrics_hook_cleared_restores_fallback` — setar fn=NULL volta ao fallback sem chamar o hook anterior.
- `test_text_metrics_hook_failure_falls_back` — hook que retorna `-1` força fallback determinístico.
- `test_displaylist_emits_font_id` — `widget->font_id` propaga para `CAPY_DL_TEXT` (schema 4).
- `test_displaylist_font_id_default_zero` — `font_id` default é 0.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.9.0 | Introdução do hook (`capy_text_metrics_fn`, `capy_widget_set_text_metrics_hook`, `capy_widget_measure_text`), campo `font_id` em `capy_widget` (tail aditivo) e em `capy_dl_cmd` (layout-compatível rename de `reserved`). DL schema 3 → 4. |
