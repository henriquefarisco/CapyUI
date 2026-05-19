# ABI versions — `capy-ui-widget`

Histórico autoritativo de versões do ABI master e do display-list schema.

## Linha do tempo

| Versão | Release | Status | Mudanças principais |
|--------|---------|--------|---------------------|
| **0.0** | 0.0.1 | tagged | Baseline widget tree, eventos, style, allocator, hit-test |
| **0.1** | 0.1.0 | tagged | Layout primitives (STACK/GRID/FLOW + measure/arrange) |
| 0.2 | 0.2.0 | tagged | Display-list output (schema 1) + emit determinístico |
| 0.3 | 0.3.0 | tagged | Focus traversal + DL schema 2 (FOCUS_RING) |
| 0.4 | 0.4.0 | tagged | Text editing (caret, seleção, UTF-8, IME stub, password render, caret RECT) |
| 0.5 | 0.5.0 | tagged | Animation (ticks discretos, easings inteiros 16.16, widget_tick walker) |
| **0.6** | 0.6.0 | **atual** | Theme tokens v2 (17 tokens, light/dark/high-contrast, resolver com fallback) — **curto prazo completo** |
| 0.7 | 0.7.0 | gate Etapa 4 | Adapter CapyOS |
| 0.8 | 0.8.0 | planned | Compositor + dirty rects (DL schema 3) |
| 0.9 | 0.9.0 | planned | Texto/fontes host (DL schema 4, font_id) |
| 0.10 | 0.10.0 | planned | Input plumbing (wheel/touch/gamepad + modifiers) |
| 0.11 | 0.11.0 | planned | Accessibility tree |
| 0.12 | 0.12.0 | gate Etapa 6 | Shell integration (NOTIFICATION/TRAY_ICON) |
| 0.13 | 0.13.0 | planned | i18n + RTL |
| 0.14 | 0.14.0 | planned | Theming UX (serialize/deserialize) |
| 0.15 | 0.15.0 | planned | Performance tier 1 (pool, cache measure) |
| **1.0** | 1.0.0 | planned | **CONGELAMENTO** |
| 1.1 | 1.1.0 | planned | Damage tracking otimizado |
| 1.2 | 1.2.0 | planned | GPU path opcional (DL schema 5) |
| 1.3 | 1.3.0 | planned | Rich animation (keyframes, springs, bezier) |
| 1.4 | 1.4.0 | planned | Gesture engine |
| 1.5 | 1.5.0 | planned | Multi-display DPI (DL schema 6) |
| 1.6 | 1.6.0 | planned | Drag and drop |
| 1.7 | 1.7.0 | planned | Performance tier 2 (compression, slab) |
| 1.8 | 1.8.0 | planned | IME rico |
| 1.9 | 1.9.0 | planned | Transforms 2D (DL schema 7) |
| 1.10 | 1.10.0 | planned | State serialization |
| **2.0** | 2.0.0 | planned | **Plugin system + SDK público (quebra controlada)** |
| 2.1 | 2.1.0 | planned | Advanced widgets |
| 2.2 | 2.2.0 | planned | Virtualization |
| 2.3 | 2.3.0 | planned | Undo/Redo |
| 2.4 | 2.4.0 | planned | Theme packs |
| 2.5 | 2.5.0 | planned | Devtools/inspector |
| **3.0** | 3.0.0 | planned | **Vision platform (quebra controlada, DL schema 9)** |

## Regras

- **Pré-1.0:** aditivo apenas. Ver `dependencies/breaking-change-policy.md`.
- **1.0 → 2.0:** deprecação ≥2 minor releases antes de remoção. Guia de migração obrigatório.
- **2.x LTS:** ≥12 meses pós-3.0.

## Como consultar

- **"Que versão de CapyUI atende o requisito X?"** → ver tabela acima e clicar no link da release.
- **"Quando o display-list ganhou op Y?"** → ver `compatibility-matrix.md` seção "Display-list schema".
- **"Posso usar API foo() em CapyOS Etapa 5?"** → ver `compatibility-matrix.md` seção "CapyOS Etapa → CapyUI mínimo".

## Política de atualização

Atualizar:

- A cada release tag (linha "tagged" → próxima vira "atual").
- Ao planejar nova fatia (adiciona linha "planned").
- Ao mudar status de bloqueio (gate → tagged).
