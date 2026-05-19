# Contrato: Display-list

**Since:** 0.2.0
**Status:** stable (aditivo)
**Schema atual:** 2

## Definição

Sequência determinística de comandos de desenho emitida a partir de uma árvore de widgets, independente de surfaces CapyOS.

## Schema versioning

- Macro `CAPY_DISPLAY_LIST_SCHEMA_VERSION` (atual = 2).
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
  CAPY_DL_FOCUS_RING       /* since 0.3 */
  /* aditivo: DIRTY_HINT em 0.8; TRANSFORM_PUSH/POP em 1.9; DPI_SCOPE em 1.5; LAYER_3D_PUSH/POP em 3.0 */
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
  uint16_t reserved;     /* aditivo: font_id em 0.9 */
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
- Campos novos em `capy_dl_cmd`: usar campo `reserved` (16 bits) primeiro; depois append.
- Schema bump apenas quando ops novas são adicionadas.
- Payload novo em op existente só via campos aditivos.

## Schema history

| Schema | Ops | Introduzido em |
|--------|-----|----------------|
| 1 | NONE, RECT, BORDER, TEXT, CLIP_PUSH, CLIP_POP, IMAGE_REF | 0.2.0 |
| 2 | + FOCUS_RING | 0.3.0 |
| 3 | + DIRTY_HINT | 0.8.0 (planned) |
| 4 | + font_id em TEXT | 0.9.0 (planned) |
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

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.2.0 | Schema 1 — introdução do módulo |
| 0.3.0 | Schema 2 — op aditiva `FOCUS_RING` |
| 0.4.0 | Schema 2 (sem mudança) — password substitui bytes em TEXT (`*` por char UTF-8); caret RECT após TEXT para TEXTBOX focado |
| 0.5.0 | Schema 2 (sem mudança) — animação opera fora do display-list |
| 0.6.0 | Schema 2 (sem mudança) — ponteiro aditivo `const struct capy_theme *theme` em `capy_display_list` (NULL por default); cores resolvidas via `capy_theme_resolve` quando token de style != 0 |
