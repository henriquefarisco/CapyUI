# Contrato: Display-list

**Since:** 0.2.0
**Status:** stable (aditivo)
**Schema atual:** 4

## Definição

Sequência determinística de comandos de desenho emitida a partir de uma árvore de widgets, independente de surfaces CapyOS.

## Schema versioning

- Macro `CAPY_DISPLAY_LIST_SCHEMA_VERSION` (atual = 4).
- Campo `dl->version` armazena a versão ao init.
- Consumidores devem:
  - Aceitar versão **maior** que esperada e ignorar ops desconhecidas.
  - Falhar limpo (sem render parcial) se versão **menor** que mínima.

## Ops

```c
enum capy_dl_op {
  CAPY_DL_NONE = 0,
  CAPY_DL_RECT,
  CAPY_DL_BORDER,
  CAPY_DL_TEXT,
  CAPY_DL_CLIP_PUSH,
  CAPY_DL_CLIP_POP,
  CAPY_DL_IMAGE_REF,
  CAPY_DL_FOCUS_RING,      /* since 0.3 */
  CAPY_DL_DIRTY_HINT       /* since 0.8 — inline dirty hint (never emitted by capy_widget_emit) */
  /* aditivo planejado: GPU primitives em 1.2; DPI_SCOPE em 1.5; TRANSFORM_PUSH/POP em 1.9; LAYER_3D_PUSH/POP em 3.0 */
};
```

## Structs

```c
struct capy_dl_cmd {
  enum capy_dl_op op;
  struct capy_ui_rect rect;
  uint32_t color;
  uint16_t text_offset;
  uint16_t text_len;
  uint8_t border_width;
  uint8_t font_size;
  uint16_t font_id;      /* since 0.9 — layout-compatível rename de `reserved`; default 0 */
  uint32_t image_id;
};

struct capy_display_list {
  struct capy_dl_cmd *cmds;
  uint32_t count;
  uint32_t capacity;
  char *text_pool;
  uint32_t text_used;
  uint32_t text_capacity;
  uint32_t version;
};
```

## APIs

- `void capy_display_list_init(struct capy_display_list *dl, void *cmd_buf, uint32_t cmd_cap, char *text_buf, uint32_t text_cap);`
- `void capy_display_list_reset(struct capy_display_list *dl);`
- `int  capy_widget_emit(struct capy_widget *root, struct capy_display_list *dl);`
- `int  capy_widget_diff(const struct capy_display_list *prev, const struct capy_display_list *next, struct capy_ui_rect *out_dirty, uint32_t out_cap);` (since 0.8)

### `capy_widget_diff` semantics (since 0.8)

- Percorre `prev->cmds[]` e `next->cmds[]` em paralelo por índice.
- Comandos byte-iguais (struct toda comparada byte-a-byte; `capy_dl_push` zera padding antes de preencher — `memcmp` é seguro) → skip.
- Comandos diferentes → emite `next->cmds[i].rect`. Se `prev->cmds[i].rect != next->cmds[i].rect`, emite também `prev->cmds[i].rect`.
- Comandos remanescentes em `next` (extras) → emite seus rects.
- Comandos remanescentes em `prev` (removidos) → emite seus rects.
- Coalescing estável: rect idêntico ao último escrito é suprimido (deduplicação sequencial).
- Retorna número de rects escritos (≥0), ou `-1` em overflow de `out_cap` (parcialmente preenchido, sem rollback) ou `out_dirty == NULL && out_cap > 0`.
- `prev == NULL` ou `next == NULL` é tratado como lista vazia (toda a outra lista é dirty).
- O(max(prev->count, next->count)), zero allocação.

## Semântica de emit

Para cada widget visível, em ordem fixa:

1. `CLIP_PUSH` com `bounds`.
2. `RECT` com `style.bg_color`.
3. `BORDER` (apenas se `style.border_width > 0`).
4. `FOCUS_RING` (since 0.3, apenas se `widget->focused != 0`) com `style.active_color`.
5. `TEXT` (apenas se `text[0] != '\0'`). Copia bytes para `text_pool` sem NUL. **Password mode (since 0.4):** se `type == TEXTBOX && text_edit.password == 1`, escreve `*` em `text_pool` (um por caractere UTF-8) em vez dos bytes originais.
6. Caret `RECT` (since 0.4, apenas se `type == TEXTBOX && focused && !text_edit.readonly && !text_edit.password`). Largura 1 px, altura `font_size`, posição horizontal = `bounds.x + caret * font_size / 2`.
7. Recurse em filhos (na ordem do array `children`).
8. `CLIP_POP`.

Widgets `visible=0` não emitem nada (nem CLIP_PUSH).

## Invariantes

- **Determinístico:** mesma árvore + mesmos bounds → mesma sequência de bytes em `cmds[]` e `text_pool[]`.
- **Clip balanceado:** count(CLIP_PUSH) == count(CLIP_POP) em emit completo.
- **Rollback em overflow:** `emit` retorna `-1` e restaura `count` e `text_used` aos valores pré-call.
- **Sem alocação interna:** buffers fornecidos pelo caller; zero malloc.
- **Reset:** zera `count` e `text_used`, mas preserva `capacity`, `text_capacity` e `version`.

## Regras de evolução

- Ops novas: append no final do enum.
- Campos novos em `capy_dl_cmd`: o slot reservado de 16 bits foi consumido por `font_id` em 0.9; novos campos devem ser **appended at the tail** (após `image_id`), preservando a regra aditiva.
- Schema bump apenas quando ops novas são adicionadas.
- Payload novo em op existente só via campos aditivos.

## Schema history

| Schema | Ops | Introduzido em |
|--------|-----|----------------|
| 1 | NONE, RECT, BORDER, TEXT, CLIP_PUSH, CLIP_POP, IMAGE_REF | 0.2.0 |
| 2 | + FOCUS_RING | 0.3.0 |
| 3 | + DIRTY_HINT | 0.8.0 |
| **4** | + font_id em TEXT (rename layout-compatível de `reserved`) | **0.9.0 (atual)** |
| 5 | + GPU primitives opcionais | 1.2.0 (planned) |
| 6 | + DPI_SCOPE | 1.5.0 (planned) |
| 7 | + TRANSFORM_PUSH/POP | 1.9.0 (planned) |
| 8 | + plugin custom ops | 2.0.0 (planned) |
| 9 | + LAYER_3D_PUSH/POP | 3.0.0 (planned) |

## Testes que cobrem o contrato

- `test_displaylist_emit_simple_tree` — sequência correta e count.
- `test_displaylist_clip_balance` — push/pop balanceados.
- `test_displaylist_overflow_rollback` — overflow restaura state.
- `test_displaylist_reset_and_text` — reset + copy de texto.
- `test_displaylist_diff_identical_zero_rects` (0.8) — emit duas vezes da mesma árvore → 0 rects.
- `test_displaylist_diff_button_color_change` (0.8) — trocar bg_color de botão → rect pequeno (não tela inteira).
- `test_displaylist_diff_coalesces_adjacent` (0.8) — cmds consecutivos com mesmo rect coalescem.
- `test_displaylist_diff_overflow_returns_minus_one` (0.8) — cap insuficiente → `-1`; out NULL com cap>0 → `-1`.
- `test_displaylist_diff_determinism` (0.8) — duas execuções independentes → mesmos rects byte-a-byte.
- `test_displaylist_dirty_hint_op_not_emitted` (0.8) — `emit` nunca produz `DIRTY_HINT`; reservado para produtores de mais alto nível.
- `test_displaylist_emits_font_id` (0.9) — schema 4; `widget->font_id` propaga para `CAPY_DL_TEXT`.
- `test_displaylist_font_id_default_zero` (0.9) — `font_id` default 0 após `capy_widget_create`.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.2.0 | Schema 1 — introdução do módulo |
| 0.3.0 | Schema 2 — op aditiva `FOCUS_RING` |
| 0.4.0 | Schema 2 (sem mudança) — password substitui bytes em TEXT (`*` por char UTF-8); caret RECT após TEXT para TEXTBOX focado |
| 0.5.0 | Schema 2 (sem mudança) — animação opera fora do display-list |
| 0.6.0 | Schema 2 (sem mudança) — ponteiro aditivo `const struct capy_theme *theme` em `capy_display_list` (NULL por default); cores resolvidas via `capy_theme_resolve` quando token de style != 0 |
| 0.7.x | Schema 2 (sem mudança) — packaging-only; migração desktop-session do fallback in-tree CapyOS, sem mudança no núcleo retido |
| 0.8.0 | Schema **3** — op aditiva `CAPY_DL_DIRTY_HINT` (não emitida por `capy_widget_emit`; reservada para produtores higher-level); API nova `capy_widget_diff` determinística O(n) que escreve rects em buffer caller-provided com coalescing estável e overflow sem rollback |
| 0.9.0 | Schema **4** — rename layout-compatível `reserved` → `font_id` em `capy_dl_cmd`; `capy_widget_emit` propaga `widget->font_id` (campo aditivo no tail de `capy_widget`) para cada `CAPY_DL_TEXT`; introdução do `capy_text_metrics_fn` hook + `capy_widget_set_text_metrics_hook` + `capy_widget_measure_text` (fallback determinístico quando hook ausente) |
