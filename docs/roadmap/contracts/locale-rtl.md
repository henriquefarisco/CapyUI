# Contrato: Locale + RTL

**Since:** 0.13.0 (planned)
**Status:** planned
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
void capy_context_set_locale(struct capy_widget_context *ctx,
                             const struct capy_locale *l);
int  capy_locale_format(const struct capy_locale *l,
                        const char *fmt, ...,
                        char *out, uint16_t out_cap);
int  capy_locale_plural(const struct capy_locale *l, uint32_t n);
```

## Format subset

Subconjunto de printf:

- `%d` — int32_t.
- `%u` — uint32_t.
- `%x` — uint32_t hex lowercase.
- `%s` — const char*.
- `%%` — literal `%`.

Sem `%f`, `%e`, `%g` (zero float no core).

## Semântica RTL

- `rtl=1` causa reflow automático em `CAPY_LAYOUT_STACK` horizontal: ordem visual invertida (último filho à esquerda).
- `padding_left` ↔ `padding_right` em rendering (não em campos).
- Layouts verticais não afetados.
- GRID e FLOW respeitam direção de fluxo.

## Invariantes

- Default: `tag="en-US"`, `rtl=0`, `plural=EN` (comportamento atual).
- Determinístico: mesma locale + árvore → mesmo display-list.
- `locale_plural` para `n=1` retorna sempre índice `0` em `EN/PT`.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.13.0 (planned) | Introdução de locale + RTL |
