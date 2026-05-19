# Contrato: Theme tokens v2

**Since:** 0.6.0
**Status:** delivered
**Fatia:** `short-term/v0.6-theme-tokens-v2.md`

## Enum

```c
enum capy_token {
  CAPY_TOKEN_NONE = 0,            /* reservado: "use literal *_color" */
  CAPY_TOKEN_BG_BASE,
  CAPY_TOKEN_BG_RAISED,
  CAPY_TOKEN_BG_SUNKEN,
  CAPY_TOKEN_FG_PRIMARY,
  CAPY_TOKEN_FG_MUTED,
  CAPY_TOKEN_FG_INVERSE,
  CAPY_TOKEN_ACCENT,
  CAPY_TOKEN_ACCENT_HOVER,
  CAPY_TOKEN_BORDER,
  CAPY_TOKEN_BORDER_FOCUS,
  CAPY_TOKEN_FOCUS_RING,
  CAPY_TOKEN_DANGER,
  CAPY_TOKEN_WARNING,
  CAPY_TOKEN_SUCCESS,
  CAPY_TOKEN_INFO,
  CAPY_TOKEN_DISABLED,
  CAPY_TOKEN_COUNT                /* upper bound, 17 */
};
```

> **Nota:** `CAPY_TOKEN_NONE = 0` foi adicionado durante a entrega para garantir backward-compat sem ambiguidade. Style com `*_token == 0` (zero-init padrão) usa o literal `*_color`, exatamente como pré-0.6.

## Struct

```c
struct capy_theme {
  uint32_t tokens[CAPY_TOKEN_COUNT];
  uint8_t variant;          /* 0=light, 1=dark, 2=high_contrast */
  uint8_t high_contrast;
  uint16_t version;
};
```

## APIs

```c
struct capy_theme capy_theme_default_light(void);
struct capy_theme capy_theme_default_dark(void);
struct capy_theme capy_theme_high_contrast(void);
void capy_theme_apply(struct capy_widget_context *ctx,
                      const struct capy_theme *theme);
uint32_t capy_theme_resolve(const struct capy_theme *theme, uint8_t token,
                            uint32_t fallback);
```

## Style aditivo

`capy_widget_style` ganha (no final, com `reserved` para alinhamento):

```c
uint8_t bg_token;
uint8_t fg_token;
uint8_t border_token;
uint8_t reserved;
```

## Resolução no display-list

Display-list `struct capy_display_list` ganha campo aditivo no final:

```c
const struct capy_theme *theme;  /* NULL após init; opt-in */
```

`capy_widget_emit` resolve cores via `capy_theme_resolve(dl->theme, style.*_token, style.*_color)`:

- NULL theme → fallback (literal).
- `token == 0` → fallback.
- `token >= CAPY_TOKEN_COUNT` → fallback.
- Outros valores → `theme->tokens[token]`.

Mapeamento op → token:

- `RECT` (bg): `bg_token` / `bg_color`.
- `BORDER`: `border_token` / `border_color`.
- `TEXT`: `fg_token` / `text_color`.
- Caret `RECT` (TEXTBOX focado): `fg_token` / `text_color`.
- `FOCUS_RING`: mantém `active_color` literal em v0.6 (sem token dedicado até nova slice).

## Invariantes

- `apply` é determinístico: mesma `theme` → mesmo estado em `ctx->theme`.
- Trocar tema reemite display-list com cores diferentes mas **mesma estrutura de comandos** (mesma sequência de ops, mesmo `count`, mesma ordem).
- High-contrast garante WCAG AA mínimo: par `BG_BASE = #000000` / `FG_PRIMARY = #FFFFFF` produz contraste 21:1 (máximo absoluto).
- `capy_theme_resolve` nunca crasha: NULL theme, token=0 ou out-of-range sempre retornam `fallback`.
- `capy_theme_apply(NULL, theme)` ou `capy_theme_apply(ctx, NULL)` são no-ops seguras.
- Sem `malloc` interno; theme é copiada por valor no `apply` (struct é ~72 bytes).
- Determinístico: mesma `(tree, theme)` → mesma sequência de comandos byte-idêntica.
- Backward-compat: callers pré-0.6 (sem tokens, sem theme em dl) produzem display-list bit-idêntico ao pré-0.6.

## Defaults

- **Light:** bg_base=`0xFFF5F6F8`, bg_raised=`0xFFFFFFFF`, fg_primary=`0xFF1A1B1F`, accent=`0xFF2563EB`, border=`0xFFE5E7EB`.
- **Dark:** bg_base=`0xFF1A1B1F`, bg_raised=`0xFF24262C`, fg_primary=`0xFFE8ECEF`, accent=`0xFF3B82F6`, border=`0xFF4B5563`.
- **High-contrast:** bg_base=`0xFF000000`, fg_primary=`0xFFFFFFFF`, accent=`0xFFFFFF00`, border=`0xFFFFFFFF`, success=`0xFF00FF00`, danger=`0xFFFF0000`.

Todos os 16 tokens semanticos são populados em cada default (ver implementação em `capy_widget.c`).

## Testes que cobrem o contrato

- `test_theme_defaults_snapshot` — valida valores documentados em light/dark/high-contrast.
- `test_theme_apply_stores_in_context` — apply copia tema; NULLs são no-op.
- `test_theme_resolve` — NULL theme, token=0, token=250 (out-of-range) e token válido.
- `test_theme_high_contrast_meets_wcag_aa` — bg_sum < 96 e fg_sum > 672 (componentes RGB).
- `test_displaylist_uses_theme_when_token_set` — token em style + theme em dl → cor do tema.
- `test_displaylist_backward_compat_with_literal` — token=0 → cor literal mesmo com theme setado.
- `test_displaylist_theme_change_preserves_structure` — dois temas, mesma árvore → ops idênticos, cores diferentes.

## Regras de evolução

- `enum capy_token` cresce **no final**; novos tokens entram antes de `CAPY_TOKEN_COUNT`.
- `CAPY_TOKEN_COUNT` cresce a cada bump aditivo; `sizeof(struct capy_theme)` cresce — **compatível em source, requer rebuild** de consumidores que armazenam temas por valor.
- `version` field permite migração entre defaults (v0.6 grava `version = 1`).
- `FOCUS_RING` token poderá ganhar resolução via style.focus_ring_token em slice futura sem quebrar 0.6.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.6.0 | Introdução do módulo: enum capy_token (17 entries), struct capy_theme, 3 defaults, apply/resolve, tokens em style, theme opcional em display_list |
| 0.14.0 (planned) | Serialize/deserialize de temas para persistência no host |
| 2.4.0 (planned) | Theme packs (formato binário bundled, hot reload) |
