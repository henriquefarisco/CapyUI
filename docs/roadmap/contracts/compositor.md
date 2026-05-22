# Contrato: Compositor + dirty rects

**Since:** 0.8.0 (delivered)
**Status:** delivered (CapyUI side); CapyOS compositor consumer ainda evolui sob Etapa 4
**Fatia:** `medium-term/v0.8-compositor-host.md`

## Definição

Compositor CapyOS consome display-list de CapyUI; CapyUI fornece diff e dirty hints opcionais.

## Op aditiva no display-list

```c
CAPY_DL_DIRTY_HINT  /* schema → 3 */
```

Payload: `rect` define região suja explicitada. Compositor pode honrar ou ignorar.

## API

```c
int capy_widget_diff(struct capy_display_list *prev,
                     struct capy_display_list *next,
                     struct capy_ui_rect *out_dirty,
                     uint32_t out_cap);
```

## Semântica

- `diff` percorre `prev` e `next` simultaneamente.
- Comandos idênticos em mesma posição: skip.
- Comandos diferentes: emite `rect` afetado em `out_dirty`.
- Comandos extras em `next`: emite seu `rect`.
- Comandos removidos em `prev`: emite seu `rect`.
- Rects adjacentes podem coalescer (decisão do impl, mas determinística).

## Invariantes

- **Determinístico:** mesma `(prev, next)` → mesma lista de rects byte-a-byte.
- **Coalescing estável:** mesma entrada não fragmenta rects de forma diferente entre chamadas.
- **Overflow:** retorna `-1` quando `out_cap` excedido; `out_dirty` parcial.
- **Performance:** O(max(count_prev, count_next)).

## Regras para compositor

1. Ler `dl->version`.
2. Se `>= 3`, pode usar `DIRTY_HINT` ops.
3. Caso contrário, fazer diff próprio via `capy_widget_diff`.
4. Aplicar dirty rects ao backbuffer; flush apenas regiões afetadas.
5. Double-buffer + vsync ficam a critério do host.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.8.0 | Introdução de `capy_widget_diff` + op aditiva `CAPY_DL_DIRTY_HINT` (DL schema 2 → 3). Lado CapyUI entregue; consumidor compositor CapyOS segue trilha Etapa 4. |
