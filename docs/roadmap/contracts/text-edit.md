# Contrato: Text editing

**Since:** 0.4.0
**Status:** delivered
**Fatia:** `short-term/v0.4-text-editing.md`

## Struct aditiva em `capy_widget` (válida quando `type == TEXTBOX`)

```c
struct capy_text_edit {
  uint16_t caret;
  uint16_t sel_start;
  uint16_t sel_end;
  uint8_t multiline;
  uint8_t readonly;
  uint8_t password;
  uint8_t reserved;
};
```

## APIs

```c
int  capy_textbox_insert(struct capy_widget *w, const char *utf8, uint16_t len);
int  capy_textbox_delete(struct capy_widget *w, int16_t count);  /* negativo = backspace */
void capy_textbox_set_selection(struct capy_widget *w, uint16_t start, uint16_t end);
int  capy_textbox_copy(struct capy_widget *w, char *out, uint16_t out_cap);
int  capy_widget_ime_compose(struct capy_widget *w, const char *preedit, uint16_t len);
```

## Semântica

- UTF-8 validation mínima em `insert`/`ime_compose`; sequências mal-formadas (leading byte inválido, truncada, ou continuation byte inválido) rejeitadas com retorno `-1`.
- `caret` em **bytes**, não caracteres; mantém boundary UTF-8 válido após mutações.
- Selection: `sel_start <= sel_end` (garantido por `capy_textbox_set_selection`, que faz swap se necessário). `sel_start == sel_end` significa sem seleção.
- `insert` com seleção ativa **substitui** a região selecionada antes de inserir.
- `delete` com seleção ativa remove a seleção (count ignorado).
- `delete` negativo (`count < 0`) faz backspace de `|count|` **caracteres UTF-8** (via `capy_utf8_prev_char_size`); positivo faz forward delete de `count` caracteres UTF-8.
- Password mode (`text_edit.password == 1`) emite `*` no display-list TEXT (um por caractere UTF-8). Bytes originais nunca entram no `text_pool`.
- Readonly (`text_edit.readonly == 1`) bloqueia `insert`/`delete` (retornam `-1`); `copy` e `set_selection` continuam funcionando.
- IME stub: `capy_widget_ime_compose` valida UTF-8 do preedit e aceita a chamada, mas não modifica `widget->text`. Estado real de composição (preedit visible, candidatos) chega em v1.8.
- `copy` sem seleção copia o texto inteiro; com seleção, copia `[sel_start, sel_end)`.
- Caret rendering: pequeno RECT (1 px de largura, altura = `font_size`) emitido no display-list após `TEXT` quando widget é TEXTBOX `focused && !readonly && !password`. Posição horizontal usa fallback determinístico `caret * font_size / 2` até a v0.9 (text metrics hook) conectar.

## Invariantes

- `caret <= strlen(text) <= CAPY_WIDGET_MAX_TEXT - 1` sempre (mantém sala para NUL).
- Caret sempre em boundary UTF-8 válido após mutações feitas pelas APIs do contrato.
- Mutação respeita `CAPY_WIDGET_MAX_TEXT`; insert que excederia retorna `-1` sem corromper estado.
- `capy_textbox_clamp_caret` defende contra `caret`/`sel_*` setados externamente além da string.
- Determinístico: mesma sequência de chamadas → mesma string final byte-a-byte.
- Sem `malloc` interno; mutações operam in-place no buffer fixo de `CAPY_WIDGET_MAX_TEXT` bytes.

## Testes que cobrem o contrato

- `test_text_edit_insert_basic` — insert ASCII em textbox vazio.
- `test_text_edit_insert_middle_utf8` — insert de 2 bytes UTF-8 no meio preserva chars existentes.
- `test_text_edit_insert_overflow` — insert de 280 bytes em buffer de 256 retorna -1 sem corromper.
- `test_text_edit_delete_selection` — selection + delete(0) remove região e move caret.
- `test_text_edit_backspace_utf8` — delete(-1) remove caractere UTF-8 completo (2 bytes de á).
- `test_displaylist_password_mode` — TEXT op emite `***` para texto `abc`.
- `test_text_edit_readonly_blocks_insert` — readonly bloqueia mutate; copy funciona.
- `test_text_edit_ime_stub` — ime_compose aceita UTF-8 válido, rejeita inválido, não muta texto.
- `test_text_edit_utf8_invalid_rejected` — 3 sequências inválidas (leading errado, truncada, continuation errada).
- `test_text_edit_determinism` — dois textboxes com mesma sequência produzem mesma string.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.4.0 | Introdução do módulo (insert/delete/select/copy, IME stub, password render, caret RECT) |
| 1.8.0 (planned) | IME rico substitui stub (candidate window, preedit state, commit) |
