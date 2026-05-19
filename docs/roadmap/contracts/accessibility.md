# Contrato: Accessibility tree

**Since:** 0.11.0 (planned)
**Status:** planned
**Fatia:** `medium-term/v0.11-accessibility.md`

## Struct

```c
struct capy_a11y_node {
  uint32_t widget_id;       /* hash estável */
  const char *role;
  const char *label;
  const char *description;
  uint32_t state_flags;
  uint32_t parent_id;
};
```

## API

```c
int capy_widget_a11y_snapshot(struct capy_widget *root,
                              struct capy_a11y_node *out,
                              uint32_t cap);
```

## Roles obrigatórios

`button`, `checkbox`, `textbox`, `menu`, `menuitem`, `list`, `listitem`, `tab`, `tabpanel`, `progressbar`, `dialog`, `label`, `panel`.

Strings constantes (literal pool); CapyUI não duplica.

## State flags (bitmask)

```c
#define CAPY_A11Y_FOCUSED   0x1u
#define CAPY_A11Y_CHECKED   0x2u
#define CAPY_A11Y_DISABLED  0x4u
#define CAPY_A11Y_HIDDEN    0x8u
#define CAPY_A11Y_PRESSED   0x10u
#define CAPY_A11Y_EXPANDED  0x20u
#define CAPY_A11Y_SELECTED  0x40u
#define CAPY_A11Y_READONLY  0x80u
```

## Semântica

- `snapshot` percorre árvore em pre-order, escreve nós em `out[]` até `cap`.
- `widget_id` é hash determinístico do path do widget (índices na árvore).
- Mesmo widget em mesma posição → mesmo `widget_id` entre snapshots.
- `parent_id` aponta para `widget_id` do pai; root tem `parent_id = 0`.
- `label` resolution:
  1. `widget->text` se não-vazio.
  2. `aria-label` (futuro, v3.0).
  3. `"Untitled"` fallback.

## Invariantes

- **Estabilidade:** árvore não muda → snapshot não muda.
- **Determinístico:** mesma árvore → mesmo snapshot byte-a-byte.
- **Overflow:** retorna `-1`, `out` parcialmente preenchido até `cap`.
- **Roles cobrem 100%** dos tipos focáveis e mais.

## Bridge CapyOS

Lado CapyOS:
- Converte `capy_a11y_node` → API a11y do SO (AT-SPI, MSAA, NSAccessibility, etc.).
- Atalhos globais (Win+U para central, lupa, etc.).
- Anúncio de mudanças via eventos do bridge.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.11.0 (planned) | Introdução do snapshot |
| 3.0.0 (planned) | WCAG 2.2 AA certificada |
