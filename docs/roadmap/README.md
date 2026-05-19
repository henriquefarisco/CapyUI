# CapyUI Roadmap

Roadmap segregado por horizonte e área para tracking absoluto da biblioteca portátil CapyUI.

## Navegação

- **Estado atual:** `STATUS.md` (versão entregue, próxima fatia, bloqueios).
- **Checkpoint v0.6.0 (curto prazo completo):** `CHECKPOINT-v0.6.0.md` — patch consolidado, métricas, status do release gate.
- **Tracking absoluto:** `tracking/ABSOLUTE.md`, `BLOCKERS.md`, `METRICS.md`, `DECISIONS.md`, `CHANGELOG.md`.
- **Curto prazo (0-3 meses, v0.3 → v0.6):** `short-term/` — **TODAS ENTREGUES**.
- **Médio prazo (3-9 meses, v0.7 → v1.0):** `medium-term/` — bloqueado por Etapa 4 CapyOS.
- **Longo prazo (9+ meses, v1.1 → v3.0):** `long-term/`.
- **Contratos de ABI:** `contracts/`.
- **Dependências e compatibilidade:** `dependencies/`.

## Horizontes

### Curto prazo — núcleo retido completo
Foco e travessia por teclado (v0.3), edição de texto (v0.4), animação determinística (v0.5), tokens de tema v2 (v0.6). Zero dependência de CapyOS, todos os testes rodam em host puro.

### Médio prazo — integração com CapyOS e ABI 1.0
Adapter (v0.7, gate Etapa 4), compositor com dirty rects (v0.8), texto/fontes host (v0.9), input plumbing (v0.10), acessibilidade (v0.11), shell integration (v0.12, gate Etapa 6), i18n+RTL (v0.13), theming UX (v0.14), performance tier 1 (v0.15), **estabilização do ABI 1.0**.

### Longo prazo — plataforma ambiciosa
Damage tracking (v1.1), GPU path opcional (v1.2), animação rica com spring physics (v1.3), gesture engine (v1.4), multi-display e DPI scaling (v1.5), drag&drop (v1.6), performance tier 2 (v1.7), IME rico (v1.8), transforms 2D (v1.9), serialização do modelo (v1.10), **plugin system + SDK público (v2.0)**, widgets avançados (v2.1), virtualização (v2.2), undo/redo (v2.3), theme packs (v2.4), devtools/inspector (v2.5), **vision platform com voice/AR-VR/AI (v3.0)**.

## Invariantes inquebráveis

- Núcleo portátil em C11 estrito (`-Wall -Wextra -Werror -pedantic`).
- Zero dependências externas em runtime (apenas `<stddef.h>`, `<stdint.h>`).
- Zero `malloc` interno após init; allocator injetado.
- Zero I/O e zero threads no core.
- Zero float em hot paths (layout, measure, arrange, emit).
- Mudanças aditivas no ABI até v1.0; deprecação ≥2 minor releases pós-1.0.
- `make validate` verde obrigatório em qualquer merge.
- Nenhum include de runtime CapyOS dentro de CapyUI.

## Convenções de arquivo por fatia

Cada arquivo `vX.Y-*.md` segue o template:

```markdown
# vX.Y.Z — <Nome>

**Status:** planned | in-progress | delivered | blocked
**ABI bump:** capy-ui-widget X.Y
**Dependência:** vX.Y-1 + (gate CapyOS Etapa N quando aplicável)
**Estimativa:** S/M/L
**Owner:** <quem implementa>

## Objetivo
## Escopo CapyUI
## Escopo CapyOS
## Testes obrigatórios
## Contratos atualizados
## Release gate
## Notas de compatibilidade
```

## Como navegar

1. Consulte `STATUS.md` para saber em que fatia o projeto está.
2. Abra o arquivo da fatia em `short-term/`, `medium-term/` ou `long-term/`.
3. Consulte contratos atualizados em `contracts/`.
4. Verifique dependências em `dependencies/`.
5. Atualize `tracking/ABSOLUTE.md` ao fechar uma fatia.
