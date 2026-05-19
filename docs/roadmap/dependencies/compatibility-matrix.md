# Matriz de compatibilidade CapyUI × CapyOS × ABI

Lookup table autoritativa para integração.

## CapyUI release → ABI exposto

| CapyUI | ABI `capy-ui-widget` | DL schema | Status |
|--------|----------------------|-----------|--------|
| 0.0.1 | 0.0 | — | tagged |
| 0.1.0 | 0.1 | — | tagged |
| 0.2.0 | 0.2 | 1 | tagged |
| 0.3.0 | 0.3 | 2 (`FOCUS_RING`) | tagged |
| 0.4.0 | 0.4 | 2 | tagged |
| 0.5.0 | 0.5 | 2 | tagged |
| **0.6.0** | **0.6** | **2** | **atual — curto prazo completo** |
| 0.7.0 | 0.7 | 2 | gate Etapa 4 |
| 0.8.0 | 0.8 | 3 (`DIRTY_HINT`) | planned |
| 0.9.0 | 0.9 | 4 (`font_id`) | planned |
| 0.10.0 | 0.10 | 4 | planned |
| 0.11.0 | 0.11 | 4 | planned |
| 0.12.0 | 0.12 | 4 | gate Etapa 6 |
| 0.13.0 | 0.13 | 4 | planned |
| 0.14.0 | 0.14 | 4 | planned |
| 0.15.0 | 0.15 | 4 | planned |
| 1.0.0 | **1.0** | 4 | congelamento |
| 1.1.0 | 1.1 | 4 | planned |
| 1.2.0 | 1.2 | 5 (GPU ops) | planned |
| 1.3.0 | 1.3 | 5 | planned |
| 1.4.0 | 1.4 | 5 | planned |
| 1.5.0 | 1.5 | 6 (`DPI_SCOPE`) | planned |
| 1.9.0 | 1.9 | 7 (`TRANSFORM_PUSH/POP`) | planned |
| 2.0.0 | **2.0** | 8 | quebra controlada |
| 3.0.0 | **3.0** | 9 | quebra controlada |

> Schema do display-list só incrementa quando novos ops são adicionados; mudanças de payload em ops existentes são aditivas via campos reserved sem bump.

## CapyOS Etapa → CapyUI mínimo requerido

| Etapa CapyOS | CapyUI mínimo | Notas |
|--------------|---------------|-------|
| 1-3 | nenhuma (legado) | gui legado CapyOS não usa CapyUI |
| 4 | 0.7.0 | adapter introduzido |
| 5-6 | 0.11.0+ | acessibilidade espelhada |
| 6 | 0.12.0 | shell integration |
| 7+ | 0.15.0+ | performance esperada |
| 1.0 CapyOS | CapyUI 1.0+ | ABIs estáveis em paralelo |

## Compatibilidade entre CapyUI versions

Pré-1.0:

- **Forward compatible (consumer pode usar API maior):** sim, todo bump é aditivo.
- **Backward compatible (binário CapyUI 0.X funciona em consumer 0.Y onde X<Y):** sim, ABI aditiva.

Pós-1.0:

- **1.x ↔ 1.y:** compatibilidade total dentro da major.
- **1.x ↔ 2.0:** consumidor precisa rebuild + possível adaptação (deprecation guide).
- **2.x ↔ 3.0:** mesmo padrão; LTS de 2.x por ≥12 meses.

## Display-list schema

| Schema | Ops | Introduzido em |
|--------|-----|----------------|
| 1 | `NONE`, `RECT`, `BORDER`, `TEXT`, `CLIP_PUSH`, `CLIP_POP`, `IMAGE_REF` | v0.2.0 |
| 2 | + `FOCUS_RING` | v0.3.0 |
| 3 | + `DIRTY_HINT` | v0.8.0 |
| 4 | + `font_id` em TEXT | v0.9.0 |
| 5 | + GPU primitives (opcional) | v1.2.0 |
| 6 | + `DPI_SCOPE` | v1.5.0 |
| 7 | + `TRANSFORM_PUSH/POP` | v1.9.0 |
| 8 | + plugin custom ops | v2.0.0 |
| 9 | + `LAYER_3D_PUSH/POP` (opcional) | v3.0.0 |

## Regras de leitura

Consumidor (compositor CapyOS, por exemplo) deve:

1. Ler `dl->version`.
2. Aceitar versão ≥ esperada.
3. Ignorar ops desconhecidas (skip silencioso).
4. Falhar limpo se versão < esperada (sem render parcial).

## Política de atualização desta matriz

Atualizar a cada release tag, antes do commit do tag.
