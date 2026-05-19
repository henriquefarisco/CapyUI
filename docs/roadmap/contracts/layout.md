# Contrato: Layout primitives

**Since:** 0.1.0
**Status:** stable (aditivo)

## Definição

Sistema de layout determinístico que computa `bounds` de widgets a partir de regras declarativas (STACK/GRID/FLOW), sem depender do compositor.

## Tipos

```c
enum capy_layout_kind {
  CAPY_LAYOUT_NONE = 0,
  CAPY_LAYOUT_STACK,
  CAPY_LAYOUT_GRID,
  CAPY_LAYOUT_FLOW
};

struct capy_layout_constraints {
  uint32_t min_w;
  uint32_t min_h;
  uint32_t max_w;   /* 0 = unbounded */
  uint32_t max_h;   /* 0 = unbounded */
  uint8_t axis;     /* 0 = horizontal, 1 = vertical */
  uint8_t gap;
  uint8_t pad_l;
  uint8_t pad_t;
  uint8_t pad_r;
  uint8_t pad_b;
  uint8_t cols;     /* para GRID */
  uint8_t reserved;
};

struct capy_layout_node {
  enum capy_layout_kind kind;
  struct capy_layout_constraints c;
  uint8_t grow;
  uint8_t shrink;
  uint16_t reserved;
};
```

## APIs

- `void capy_widget_set_layout(struct capy_widget *w, const struct capy_layout_node *node);`
- `int  capy_widget_measure(struct capy_widget *root, uint32_t avail_w, uint32_t avail_h);`
- `int  capy_widget_arrange(struct capy_widget *root);`

## Semântica

### measure

Clamp `root->bounds.width/height` por `min_*`/`max_*`:

- `width = max(min_w, min(avail_w, max_w ? max_w : avail_w))`
- `height = max(min_h, min(avail_h, max_h ? max_h : avail_h))`

`max_* == 0` significa unbounded (`avail_*` é o limite real).

### arrange

Para cada layout kind:

- **NONE:** filhos mantêm bounds existentes.
- **STACK:** distribui filhos ao longo de `axis`, respeitando `gap` e `padding`. Filhos com `grow > 0` dividem espaço restante proporcionalmente.
- **GRID:** distribui em `cols` colunas, com gap entre células.
- **FLOW:** filhos fluem em linha; quebram para próxima quando excedem width.

### Invariantes

- `measure + arrange` é **determinístico**: mesma árvore + mesmos `avail_*` → mesmo `bounds`.
- `measure + arrange` é **idempotente**: chamar duas vezes produz mesmos `bounds`.
- Filhos sempre dentro de `root->bounds` minus padding.
- `bounds.width/height >= 0` sempre.

## Regras de evolução

- `enum capy_layout_kind` cresce no final (novos kinds aditivos).
- `struct capy_layout_constraints` ganha campos no final (campo `reserved` será dividido).
- `struct capy_layout_node` ganha campos no final.
- Embed em `capy_widget`: campo `layout` permanece no offset estável.

## Testes que cobrem o contrato

- `test_layout_stack_equal_grow` — 3 filhos com grow=1 dividem altura igualmente.
- `test_layout_grid_2x2` — grid 2×2 com gap=4 e pad=8.
- `test_layout_idempotent` — chamar measure+arrange duas vezes.
- `test_layout_min_max_constraints` — clamp por min/max.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.1.0 | Introdução do módulo |
