# Grafo de dependências internas CapyUI

Ordem linear de execução das fatias, com forks paralelos quando aplicável.

## Visão geral

```
v0.0.1 (baseline)
   │
   ▼
v0.1.0 (layout)
   │
   ▼
v0.2.0 (display-list) ◄── ATUAL
   │
   ▼
v0.3.0 (focus) ─── adiciona CAPY_DL_FOCUS_RING (DL schema → v2)
   │
   ▼
v0.4.0 (text editing) ── usa focus para roteamento de teclado
   │
   ▼
v0.5.0 (animation) ── independente, mas TEXT/BUTTON podem animar
   │
   ▼
v0.6.0 (theme tokens v2) ── style usa tokens
   │
   ▼ ===== GATE ETAPA 4 =====
v0.7.0 (adapter CapyOS)
   │
   ▼
v0.8.0 (compositor + dirty)
   │
   ▼
v0.9.0 (text/fonts host)
   │
   ▼
v0.10.0 (input plumbing)
   │
   ▼
v0.11.0 (accessibility)
   │
   ▼ ===== GATE ETAPA 6 =====
v0.12.0 (shell integration)
   │
   ▼
v0.13.0 (i18n + RTL)
   │
   ▼
v0.14.0 (theming UX)
   │
   ▼
v0.15.0 (performance tier 1)
   │
   ▼
v1.0.0 (ABI estável)
   │
   ▼
v1.1.0 (damage tracking)
   │
   ▼
v1.2.0 (GPU path opcional)
   │
   ▼
v1.3.0 (rich animation)
   │
   ▼
v1.4.0 (gesture engine)
   │
   ▼
v1.5.0 (multi-display DPI)
   │
   ▼
v1.6.0 (drag and drop)
   │
   ▼
v1.7.0 (performance tier 2)
   │
   ▼
v1.8.0 (IME rico)
   │
   ▼
v1.9.0 (transforms 2D)
   │
   ▼
v1.10.0 (state serialization)
   │
   ▼
v2.0.0 (plugin system + SDK)
   │
   ▼
v2.1.0 (advanced widgets)
   │
   ▼
v2.2.0 (virtualization)
   │
   ▼
v2.3.0 (undo/redo)
   │
   ▼
v2.4.0 (theme packs)
   │
   ▼
v2.5.0 (devtools/inspector)
   │
   ▼
v3.0.0 (vision platform)
```

## Pontos de divergência permitidos

- **v0.4, v0.5, v0.6** podem ser desenvolvidas em paralelo após v0.3, desde que mergeadas em ordem.
- **v0.8–v0.11** dependem de v0.7 mas podem ser desenvolvidas em paralelo se os adapters host estiverem prontos.
- **v1.1–v1.10** são amplamente paralelizáveis após v1.0, mas tagueadas em ordem numérica para manter histórico legível.

## Dependências cruzadas

| Fatia | Pré-requisitos rígidos | Pré-requisitos suaves |
|-------|------------------------|----------------------|
| v0.3 | v0.2 (DL para focus ring) | nenhum |
| v0.4 | v0.3 (focus para input de teclado) | nenhum |
| v0.5 | v0.0.1 (apenas core) | nenhum |
| v0.6 | v0.0.1 (apenas core) | nenhum |
| v0.7 | **Etapa 4 CapyOS** + v0.6 | tokens estáveis |
| v1.2 | v1.1 (damage tracking) | display-list maduro |
| v1.4 | v0.10 (input com touch) | nenhum |
| v1.8 | v0.4 (text edit base) + v0.7 (host) | nenhum |
| v2.0 | v1.10 (serialização) + v1.0 (ABI estável) | todos v1.x mergeados |
| v3.0 | v2.5 (devtools) | maturidade do ecossistema |

## Política de reordenação

Mudança na ordem exige:
1. ADR em `tracking/DECISIONS.md` justificando.
2. Atualização deste arquivo e do `STATUS.md`.
3. Revisão da matriz em `compatibility-matrix.md`.
4. Comunicação em `CHANGELOG.md` no próximo release.
