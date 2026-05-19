# Contrato: Animation

**Since:** 0.5.0
**Status:** delivered
**Fatia:** `short-term/v0.5-animation-timing.md`

## Struct

```c
struct capy_anim {
  uint32_t start_tick;
  uint32_t duration_ticks;
  int32_t from;
  int32_t to;
  uint8_t easing;
  uint8_t active;
  uint16_t reserved;
};
```

## Easings

```c
enum capy_anim_easing {
  CAPY_ANIM_LINEAR = 0,
  CAPY_ANIM_EASE_IN,
  CAPY_ANIM_EASE_OUT,
  CAPY_ANIM_EASE_IN_OUT
};
```

## APIs

```c
void capy_anim_start(struct capy_anim *a, uint32_t now, uint32_t duration,
                     int32_t from, int32_t to, uint8_t easing);
int  capy_anim_sample(const struct capy_anim *a, uint32_t now, int32_t *out);
void capy_widget_tick(struct capy_widget *root, uint32_t now);
```

## Semântica

- Host fornece `now` (uint32_t ms monotônico).
- Implementação 100% em inteiros (fixed-point 16.16 internamente; intermediários `uint64_t`/`int64_t` para evitar overflow).
- `sample` em `now <= start_tick` → retorna `from` (inclui clock retrocedido).
- `sample` em `now >= start_tick + duration_ticks` → retorna `to`.
- `sample` em range intermediário → `from + (to - from) * easing(t) / 65536` com `t = elapsed * 65536 / duration`.
- `!active` ou `duration_ticks == 0` → retorna `from` (defesa contra divisão por zero).
- `capy_widget_tick(root, now)` faz DFS walk determinístico; em 0.5 é no-op por widget (hook para integrações futuras).
- Easings:
  - `LINEAR`: `t_fp`.
  - `EASE_IN`: `t²/65536`.
  - `EASE_OUT`: `65536 - (1-t)²/65536`.
  - `EASE_IN_OUT`: simetricamente espelhada em torno de t=0.5.

## Invariantes

- Determinístico: mesma `(now, anim)` → mesmo `int32_t` output, byte-idêntico entre runs e plataformas.
- Easings monotônicas em `[from, to]` ou `[to, from]`; endpoints exatos em `now == start_tick` (→ from) e `now == start_tick + duration` (→ to).
- `now` retrocedido (clock pulou para trás): comportamento estável (sempre `from`), sem corromper.
- Animação não-ativa (`active == 0`) ou `duration_ticks == 0` retorna `from`.
- Sem `float`/`double` em qualquer execução; sem `malloc` interno.
- `widget_tick` não muta nada em 0.5 (somente walk); futuras versões adicionarão per-widget anim updates aditivos.

## Testes que cobrem o contrato

- `test_anim_sample_start_returns_from` — endpoint inicial.
- `test_anim_sample_end_returns_to` — endpoint final (incluindo now > end).
- `test_anim_sample_inactive_returns_from` — `active=0` retorna from.
- `test_anim_linear_midpoint` — interp linear exato em 50%.
- `test_anim_easings_monotonic_and_endpoints` — 4 easings: endpoints exatos + monotonicidade em 5 amostras.
- `test_anim_now_rewound_stable` — clock retrocedido sempre retorna from.
- `test_anim_zero_duration_returns_from` — duração zero retorna from sem dividir por zero.
- `test_anim_multiple_independent` — duas animações paralelas não interferem.
- `test_anim_determinism` — mesma sequência de chamadas → mesmos outputs.
- `test_widget_tick_walks_tree` — walker recursivo não crasha (inclui NULL root).

## Regras de evolução

- `easing` é `uint8_t`; range 0-255 permite extensão aditiva (cubic-bezier, spring etc.).
- Animação rica (v1.3.0) adicionará `capy_anim_track` + keyframes em header separado, sem mudar este struct.
- Embed `capy_anim` em `capy_widget` adiado para slice futura quando o uso justificar.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.5.0 | Introdução do módulo (4 easings em fixed-point 16.16, sample/start/tick) |
| 1.3.0 (planned) | Animação rica: tracks com keyframes, spring physics, cubic-bezier customizável |
