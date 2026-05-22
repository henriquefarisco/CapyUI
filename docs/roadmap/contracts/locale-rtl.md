# Contrato: Locale + RTL

**Since:** 0.13.0 (delivered)
**Status:** delivered (CapyUI side); detecção de locale do SO + catálogos `.mo`/`.po` seguem trilha CapyOS
**Fatia:** `medium-term/v0.13-i18n-rtl.md`

## Struct em `capy_widget_context`

```c
struct capy_locale {
  char tag[16];           /* BCP 47: "en-US", "pt-BR", "ar-EG" */
  uint8_t rtl;
  uint8_t plural_rule;
  uint16_t reserved;
};
```

## Plural rules

```c
enum capy_plural_rule {
  CAPY_PLURAL_EN = 0,     /* one, other */
  CAPY_PLURAL_PT,         /* one, other */
  CAPY_PLURAL_AR,         /* zero, one, two, few, many, other */
  CAPY_PLURAL_RU,         /* one, few, many, other */
  CAPY_PLURAL_OTHER       /* só other */
};
```

## APIs

```c
struct capy_locale capy_locale_default(void);
void capy_context_set_locale(struct capy_widget_context *ctx,
                             const struct capy_locale *l);
int  capy_locale_plural(const struct capy_locale *l, uint32_t n);
int  capy_locale_format(const struct capy_locale *l,
                        char *out, uint16_t out_cap,
                        const char *fmt, ...);
```

- `capy_locale_default()` retorna `{tag="en-US", rtl=0, plural=EN}`. O mesmo default é propagado por `capy_widget_context_init` (since 0.13) para retrocompatibilidade.
- `capy_context_set_locale(ctx, NULL)` restaura o default.
- `capy_locale_plural(NULL, n)` cai no rule EN.

## Format subset

Subconjunto de printf (assinatura snprintf-like: `(l, out, out_cap, fmt, ...)`):

- `%d` — int promovido para `int32_t` (sinal tratado; INT32_MIN seguro via `int64_t`).
- `%u` — unsigned int → `uint32_t`.
- `%x` — uint32_t em hex minúsculo (sem prefixo `0x`).
- `%s` — `const char *` (NULL imprime nada).
- `%%` — literal `%`.
- `%` no fim do buffer → `-1` (fail-closed).
- Especificador desconhecido → `-1` (fail-closed).
- Retorna número de chars escritos (excl. NUL) em sucesso; `-1` em overflow / `out == NULL` / `out_cap == 0` / `fmt == NULL`.
- Saída sempre NUL-terminada em sucesso (reserva 1 byte interno).
- Sem `%f`, `%e`, `%g` (zero float no core).

## Semântica RTL

- `struct capy_layout_constraints` ganha `uint8_t rtl` no tail (since 0.13). Quando `rtl != 0` em `widget->layout.c.rtl`, `capy_widget_arrange` aplica um **mirror pós-layout** sobre os filhos diretos:
  - `new_x = 2 * ix + iw - old_x - width` (onde `ix`/`iw` definem o content rect após padding).
  - O mirror roda em cima das posições computadas por STACK / GRID / FLOW (ou por bounds pré-setados em `LAYOUT_NONE`).
  - Recursão nos filhos acontece **depois** do mirror, garantindo que netos sejam posicionados relativos ao `x` final do pai.
- Stack horizontal: ordem visual invertida (último filho na esquerda).
- Stack vertical (axis=1): `cross == iw` faz `width == iw`, e o mirror vira no-op.
- GRID e FLOW: itens fluem da direita para a esquerda quando `rtl=1`.
- `padding_left/right` permanecem como campos; apenas o resultado visual é espelhado.

## Invariantes

- Default: `tag="en-US"`, `rtl=0`, `plural=EN` (comportamento pré-0.13).
- `capy_widget_context_init` seeda o default automaticamente.
- Determinístico: mesma locale + árvore → mesmas posições e mesmo display-list em runs independentes.
- `locale_plural` para `n=1` retorna sempre índice `0` em `EN/PT/RU` (CLDR) e `1` em `AR` (forma "one" é índice 1, atrás de "zero").

## Testes que cobrem o contrato

- `test_locale_default_seeds_context` — `capy_locale_default()` retorna `en-US/LTR/EN` e `capy_widget_context_init` seeda o mesmo.
- `test_locale_set_null_restores_default` — set custom → NULL restaura o default.
- `test_locale_plural_en_pt` — EN/PT cobrem n=0,1,2,21; NULL locale cai em EN.
- `test_locale_plural_ar` — zero/one/two/few/many/other para n=0,1,2,5,21,100.
- `test_locale_plural_ru` — one/few/many para mod10/mod100 (n=1,2,4,5,11,21,100,0).
- `test_locale_format_subset` — `%d`, `%u`, `%x`, `%s`, `%%` (negativos, zero, hex lowercase, string vazia, todos misturados em uma string).
- `test_locale_format_overflow_returns_minus_one` — cap insuficiente / NULL out / cap=0 / NULL fmt / `%` final / especificador desconhecido todos retornam `-1`.
- `test_layout_rtl_mirrors_horizontal_stack` — STACK horizontal com rtl=1: posições espelhadas exatas (a:70, b:40 em panel 100w com filhos 30w); determinismo entre runs.
- `test_layout_rtl_vertical_stack_x_unchanged` — STACK vertical com rtl=1: x dos filhos permanece 0 porque `width == iw`.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.13.0 | Introdução de `capy_locale` (4 campos, BCP 47 tag), `enum capy_plural_rule` (EN/PT/AR/RU/OTHER), `locale` no tail de `capy_widget_context`, `rtl` no tail de `capy_layout_constraints`, 4 APIs novas (`capy_locale_default`, `capy_context_set_locale`, `capy_locale_plural`, `capy_locale_format`). `capy_widget_arrange` refatorada: layout kinds não recursam mais; o dispatcher faz mirror RTL + recursão uniformemente. Lado CapyUI entregue. |
