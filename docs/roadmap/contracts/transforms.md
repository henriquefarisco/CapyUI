# Contrato: Transforms 2D

**Since:** 1.9.0 (planned)
**Status:** planned
**Fatia:** `long-term/v1.9-transforms-2d.md`

## Ops novas

```c
CAPY_DL_TRANSFORM_PUSH   /* schema → 7 */
CAPY_DL_TRANSFORM_POP
```

## Struct

```c
struct capy_dl_transform {
  int32_t m00, m01, m02;   /* fixed-point 16.16 */
  int32_t m10, m11, m12;
};
```

Matriz 2×3 afim:

```
| m00  m01  m02 |   | x |   | x' |
| m10  m11  m12 | * | y | = | y' |
                    | 1 |
```

## APIs

```c
void capy_widget_set_transform(struct capy_widget *w,
                               const struct capy_dl_transform *t);
struct capy_dl_transform capy_transform_identity(void);
struct capy_dl_transform capy_transform_rotate(int32_t degrees_x256);
struct capy_dl_transform capy_transform_scale(int32_t sx_x256, int32_t sy_x256);
struct capy_dl_transform capy_transform_translate(int32_t tx, int32_t ty);
struct capy_dl_transform capy_transform_multiply(
    const struct capy_dl_transform *a,
    const struct capy_dl_transform *b);
```

## Semântica

- Fixed-point 16.16: 16 bits inteiro + 16 bits fracionário; precisão ~1/65536.
- `degrees_x256`: graus * 256 (ex.: 90° = 23040). Evita float.
- `*_x256` em scale: 256 = 1.0×.
- Identity: `m00=m11=0x10000` (1.0 em 16.16), `m01=m02=m10=m12=0`.

## Emit

- `TRANSFORM_PUSH` antes do widget; `TRANSFORM_POP` depois.
- Composição: transforms acumulam (push múltiplos multiplica matrizes).
- Compositor mantém stack interna; aplica top da stack a todos draws subsequentes.

## Hit-test

- Antes de comparar `(x, y)` com `bounds`, aplica matriz inversa (cumulada de ancestrais com transforms).
- Determinístico.

## Limitações

- Apenas 2D afim (sem perspectiva real).
- Compositor 2D sem suporte a transforms pode skip rendering com aviso (degradação visual).

## Invariantes

- Push/pop balanceados.
- Determinístico: mesma matriz → mesmo output byte-a-byte.
- Identity transform = output sem mudança.

## Histórico

| Versão | Mudança |
|--------|---------|
| 1.9.0 (planned) | Introdução de transforms 2D |
| 3.0.0 (planned) | LAYER_3D_PUSH/POP opcionais (separados, não substituem 2D) |
