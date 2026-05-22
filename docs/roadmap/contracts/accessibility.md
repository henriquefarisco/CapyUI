# Contrato: Accessibility tree

**Since:** 0.11.0 (delivered)
**Status:** delivered (CapyUI side); CapyOS bridge para AT-SPI/MSAA/NSAccessibility segue trilha Etapa 4+
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
- `widget_id` é hash determinístico FNV-1a 32-bit: root usa `0x811C9DC5`, e cada filho mistura `(parent_id, child_index)` via 4 step-bytes do prime `0x01000193`.
- Mesmo widget em mesma posição → mesmo `widget_id` entre snapshots.
- `parent_id` aponta para `widget_id` do pai; root tem `parent_id = 0`.
- `label` resolution:
  1. `widget->text` se não-vazio (aponta diretamente para o buffer do widget).
  2. `aria-label` (futuro, v3.0).
  3. `"Untitled"` fallback (literal estático).
- `description` sempre `NULL` em 0.11; reservado para integração futura com `aria-describedby`.
- Mapeamento `widget_type → role` (cobre todos os 12 valores conhecidos do enum, incluindo PANEL `"panel"`, SCROLLBAR `"scrollbar"`, MENUBAR `"menu"`):
  - `LABEL→"label"`, `BUTTON→"button"`, `TEXTBOX→"textbox"`, `CHECKBOX→"checkbox"`, `LIST→"list"`, `PANEL→"panel"`, `SCROLLBAR→"scrollbar"`, `MENUBAR→"menu"`, `MENU_ITEM→"menuitem"`, `DIALOG→"dialog"`, `PROGRESS→"progressbar"`, `TABS→"tab"`, default → `"none"`.
- Mapeamento `widget state → flags`: `focused → FOCUSED`, `checked → CHECKED`, `!enabled → DISABLED`, `!visible → HIDDEN`, `TEXTBOX && text_edit.readonly → READONLY`. `PRESSED`/`EXPANDED`/`SELECTED` reservados para fatias futuras.
- `NULL root`, `cap == 0`, ou `out == NULL && cap > 0` → retorna `-1` sem escrita.

## Invariantes

- **Estabilidade:** árvore não muda → snapshot não muda (CapyUI nunca mutá o widget tree durante o snapshot).
- **Determinístico:** mesma árvore → mesmo snapshot byte-a-byte (hash FNV-1a fixo, pre-order fixo, role/label/flags determinísticos).
- **Overflow:** retorna `-1`, `out` parcialmente preenchido até `cap` entradas (sem rollback).
- **Roles cobrem 100%** dos tipos focáveis e mais (todos os 12 valores do enum mapeados).

## Testes que cobrem o contrato

- `test_a11y_snapshot_basic` — widget isolado: 1 nó, `parent_id=0`, `widget_id!=0`, role "button", label de `widget->text`, `description=NULL`.
- `test_a11y_snapshot_hierarchy` — panel+2 filhos: pre-order, `parent_id` aponta para `widget_id` do panel, ids dos irmãos divergem.
- `test_a11y_roles_cover_focusable_types` — BUTTON/TEXTBOX/CHECKBOX/LIST/MENU_ITEM/TABS recebem role não-NULL e correta.
- `test_a11y_widget_id_stable_across_snapshots` — dois snapshots da mesma árvore dão ids iguais campo-a-campo.
- `test_a11y_state_flags_match_widget_state` — FOCUSED+CHECKED no checkbox, READONLY no textbox, DISABLED+HIDDEN no button refletem o estado do widget.
- `test_a11y_overflow_partial_fill_returns_minus_one` — cap=1 com 2-node tree → `-1`, root preenchido; `cap=0` → `-1`; `out=NULL` com cap>0 → `-1`; `root=NULL` → `-1`.
- `test_a11y_label_untitled_fallback` — widget sem texto → label `"Untitled"`; setar texto → label de `widget->text`.

## Bridge CapyOS

Lado CapyOS:
- Converte `capy_a11y_node` → API a11y do SO (AT-SPI, MSAA, NSAccessibility, etc.).
- Atalhos globais (Win+U para central, lupa, etc.).
- Anúncio de mudanças via eventos do bridge.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.11.0 | Introdução do `capy_a11y_node` (6 campos), `capy_widget_a11y_snapshot`, 8 state flags, role mapping cobrindo os 12 widget types, hash FNV-1a 32-bit por path. Determinístico, sem alocação, sem mutação do tree. Lado CapyUI entregue; bridge CapyOS para AT-SPI/MSAA/NSAccessibility segue trilha Etapa 4+. |
| 3.0.0 (planned) | WCAG 2.2 AA certificada |
