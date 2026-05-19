# CapyUI — Checkpoint v0.6.0 (Curto Prazo Completo)

**Data:** 2026-05-19
**Versão:** `0.6.0`
**ABI:** `capy-ui-widget 0.6`
**Display-list schema:** `2`
**Validação:** `make validate` **VERDE** (lint + security + 47 testes + version-check).
**Marco:** fase de curto prazo do roadmap (núcleo retido portátil) **integralmente entregue**.

---

## 1. Resumo executivo

Esta release fecha a fase de **curto prazo** do roadmap CapyUI. Todo o núcleo retido portátil está implementado, testado e documentado:

- **6 slices entregues** desde a baseline (`v0.0.1` → `v0.6.0`).
- **47 testes de contrato** verdes em compilação estrita C11 (`-Wall -Wextra -Werror -pedantic`).
- **~1.7k LOC core** + **1.28k LOC tests**, zero `float` em hot paths, zero `malloc` interno após init, zero deps externas em runtime.
- **65 arquivos markdown** de documentação (`docs/roadmap/` com 63 + 2 raiz).
- **Próxima fatia (`v0.7.0`) bloqueada por gate externo** (Etapa 4 do CapyOS).

---

## 2. Fatias entregues

| Slice | ABI | Conteúdo entregue | Testes |
|-------|-----|-------------------|--------|
| `v0.0.1` | 0.0 | Widget tree baseline: 12 tipos, eventos, style, allocator, hit-test | 2 |
| `v0.1.0` | 0.1 | Layout primitives: `CAPY_LAYOUT_NONE/STACK/GRID/FLOW`, measure+arrange determinístico e idempotente | 4 |
| `v0.2.0` | 0.2 | Display-list output: 7 ops, schema v1, emit determinístico, rollback em overflow | 4 |
| `v0.3.0` | 0.3 | Focus traversal: `tab_index`, `focus_next/prev` (2-pass DFS), `dispatch_key` TAB/ENTER/SPACE/ESC, `FOCUS_RING` (schema v2) | 10 |
| `v0.4.0` | 0.4 | Text editing: `capy_text_edit` struct, `insert/delete/set_selection/copy` UTF-8-aware, `ime_compose` stub, password rendering, caret RECT | 10 |
| `v0.5.0` | 0.5 | Animation + timing: `capy_anim` standalone, 4 easings em fixed-point 16.16 (zero float), `widget_tick` walker | 10 |
| `v0.6.0` | 0.6 | Theme tokens v2: 17 tokens, `capy_theme` com light/dark/high-contrast, `apply`/`resolve`, tokens em style, theme opcional em display-list | 7 |
| **Total** | — | — | **47** |

---

## 3. Estado do código

### 3.1 Arquivos do núcleo

```
src/widget/
├── capy_widget.h      265 LOC
├── capy_widget.c      986 LOC
├── capy_layout.h       43 LOC
├── capy_layout.c      193 LOC
├── capy_display_list.h  53 LOC
└── capy_display_list.c 207 LOC
                    -----------
                    1747 LOC core
```

### 3.2 Testes

```
tests/
└── test_widget_contracts.c   1278 LOC, 47 testes + 1 helper
```

Cobertura por área:

| Área | Testes | Slice de origem |
|------|--------|-----------------|
| Widget tree + eventos | 2 | 0.0.1 |
| Layout | 4 | 0.1.0 |
| Display-list (base) | 4 | 0.2.0 |
| Focus traversal | 5 | 0.3.0 |
| Dispatch key | 4 | 0.3.0 |
| Focus ring (DL) | 1 | 0.3.0 |
| Text editing | 9 | 0.4.0 |
| Password rendering (DL) | 1 | 0.4.0 |
| Animation | 9 | 0.5.0 |
| Widget tick | 1 | 0.5.0 |
| Theme tokens | 4 | 0.6.0 |
| Theme rendering (DL) | 3 | 0.6.0 |
| **Total** | **47** | — |

### 3.3 Build infra

- `Makefile` — `validate` agrega `lint` + `security` + `test` + `version-check`.
- `VERSION` = `0.6.0`.
- `README.md` linha 3 = `Version: 0.6.0`.
- CI workflows `.github/workflows/ci.yml` + `security.yml` ativos.

---

## 4. Status do release gate

Execução de `make validate` em **2026-05-19** retornou exit `0`:

```
cc -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g -fsyntax-only \
   src/widget/capy_widget.c src/widget/capy_layout.c \
   src/widget/capy_display_list.c                        # lint OK
git diff --check                                         # whitespace OK
test "$(cat VERSION)" = "0.6.0"                          # version OK

cc ... -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE \
   -fsyntax-only ...                                     # security OK

mkdir -p build
cc ... -Isrc/widget ... tests/test_widget_contracts.c \
   -o build/test_widget_contracts                        # build OK
build/test_widget_contracts                              # 47 tests PASS

test "$(cat VERSION)" = "0.6.0"                          # version-check OK
grep -q "Version: 0.6.0" README.md                       # README OK
```

Todos os 47 testes executaram sem `failures` (binário retornou 0).

---

## 5. Invariantes preservadas em todo o checkpoint

- ✅ **C11 estrito:** compila com `-std=c11 -Wall -Wextra -Werror -pedantic` sem warnings.
- ✅ **Hardening:** compila adicionalmente com `-D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE`.
- ✅ **Zero dependências externas em runtime:** apenas `<stddef.h>` e `<stdint.h>`.
- ✅ **Zero `float`/`double` em hot paths:** layout, emit, animação, edição de texto, foco — todos em inteiros (fixed-point 16.16 onde necessário, com `uint64_t`/`int64_t` para intermediários).
- ✅ **Zero `malloc` interno após init:** todas as alocações vêm do `capy_widget_allocator` injetado pelo caller; helpers operam in-place em buffers fixos.
- ✅ **Zero I/O no core:** sem `<stdio.h>`, sem syscalls, sem `<time.h>`.
- ✅ **ABI aditivo:** todas as expansões em structs públicas foram no final; campos novos com sufixo/reserved quando padding necessário. Programs pré-0.X continuam funcionando após upgrade.
- ✅ **Determinismo byte-a-byte:** mesma árvore + mesma entrada → mesma display-list, mesmo focus, mesma string, mesmo valor de animação. Sem clock real, sem `random`, sem alocação não-determinística.
- ✅ **Backward compat de display-list:** consumidores pré-0.6 (sem tokens, sem theme em dl) produzem display-list bit-idêntico ao pré-0.6.

---

## 6. Contratos versionados

15 contratos detalhados em `docs/roadmap/contracts/` (mais `abi-versions.md` e `deprecation-policy.md`):

**Entregues (delivered):**

- `widget-model.md`, `layout.md`, `display-list.md`, `focus.md`, `text-edit.md`, `animation.md`, `theme-tokens.md`.

**Planejados (planned, futuras slices):**

- `adapter.md`, `compositor.md`, `text-metrics-hook.md`, `input.md`, `accessibility.md`, `shell.md`, `locale-rtl.md`, `theme-serialization.md`, `plugin-abi.md`, `transforms.md`.

Cada contrato lista: definição formal, semântica, invariantes, regras de evolução, testes que o cobrem, e histórico de mudanças.

---

## 7. Patch consolidado — mudanças desde a baseline

### 7.1 Arquivos modificados (existentes)

```
.github/dependabot.yml
.github/workflows/ci.yml
.github/workflows/security.yml
.gitignore
LICENSE
Makefile                              # SRC expandida, version-check 0.6.0
README.md                             # Version: 0.6.0
SECURITY.md
VERSION                               # 0.6.0
docs/README.md                        # roadmap pointer + next slices marcadas
docs/compatibility.md                 # ABI 0.6, 4 seções novas (layout, DL, focus, text edit, animation, theme)
src/widget/capy_widget.c              # +840 LOC (de ~150 para 986)
src/widget/capy_widget.h              # +135 LOC (de ~130 para 265)
tests/test_widget_contracts.c         # +870 LOC (de ~410 para 1278)
```

### 7.2 Arquivos novos (untracked)

```
src/widget/capy_layout.h              # v0.1.0
src/widget/capy_layout.c              # v0.1.0
src/widget/capy_display_list.h        # v0.2.0
src/widget/capy_display_list.c        # v0.2.0
docs/roadmap/                         # plano-mestre + 63 arquivos
├── README.md
├── STATUS.md
├── CHECKPOINT-v0.6.0.md              # este arquivo
├── short-term/    (6 fatias seed)
├── medium-term/   (10 fatias seed)
├── long-term/     (17 fatias seed)
├── contracts/     (19 contratos)
├── dependencies/  (6 docs)
└── tracking/      (5 docs)
```

### 7.3 Suggested commit sequence

Para registrar formalmente este checkpoint em git, sugiro commits separados por slice (preservando granularidade do histórico):

```sh
cd /Volumes/CapyOS/CapyUI

# Slice 1: Layout (v0.1.0)
git add src/widget/capy_layout.{h,c}
git commit -m "feat(layout): add layout primitives (v0.1.0)"

# Slice 2: Display-list (v0.2.0)
git add src/widget/capy_display_list.{h,c}
git commit -m "feat(dl): add deterministic display-list emit (v0.2.0)"

# Slice 3: Focus (v0.3.0) — mudanças em widget.{h,c} + display_list.{h,c}
# Slice 4: Text edit (v0.4.0)
# Slice 5: Animation (v0.5.0)
# Slice 6: Theme tokens (v0.6.0)

# (As mudanças em widget.{h,c}, display_list.{h,c} e tests/ acumularam-se
#  entre as slices 3-6. Para reconstrução granular use git log com
#  inspeção interativa, ou trate como um único commit "fase curto prazo".)

# Alternativa: commit único do checkpoint
git add -A
git commit -m "checkpoint: short-term phase complete (v0.0.1 → v0.6.0)

- 6 slices delivered: layout, display-list, focus, text editing,
  animation, theme tokens.
- 47 contract tests passing under -Wall -Wextra -Werror -pedantic.
- ABI capy-ui-widget at 0.6 (additive); display-list schema at 2.
- ~1.7k LOC core, ~1.28k LOC tests.
- Zero external runtime deps; zero float in hot paths; zero malloc
  after init; deterministic byte-identical output.
- Next slice (v0.7.0) blocked on CapyOS Etapa 4 (host adapter)."

# Tag the checkpoint
git tag -a v0.6.0 -m "CapyUI v0.6.0 — short-term phase complete"
```

---

## 8. Próximos passos

### 8.1 Bloqueio externo (médio prazo)

`v0.7.0` (adapter CapyOS) está bloqueada pela **Etapa 4 do roadmap CapyOS** (drivers maduros + scheduler + display abstraction). Ver `dependencies/capyos-gates.md` para detalhes.

### 8.2 Opções paralelas independentes do gate

Durante o bloqueio, é possível avançar em fatias do longo prazo que dependem apenas do núcleo retido:

- **`v1.1.0`** Damage tracking (`capy_widget_invalidate`, diff incremental, frame budget).
- **`v1.3.0`** Rich animation (tracks com keyframes, spring physics, cubic-bezier).
- **`v1.5.0`** Multi-display DPI scaling (`capy_widget_set_dpi_scale`, snap em pixel).
- **`v1.9.0`** Transforms 2D (matriz 2×3 em fixed-point, op aditiva no DL).
- **`v1.10.0`** Serialização do modelo retido.

### 8.3 Endurecimento (qualidade)

Trabalho de hardening que não bumpa ABI:

- Aumentar cobertura de edge cases nos testes existentes.
- Adicionar fixtures binárias canônicas para regressão de display-list.
- Documentar exemplos de uso (templates de adapter, app templates).
- Auditoria de includes (script de CI que rejeita `#include <gui/`, etc.).
- Benchmark suite (sem bloquear CI) para baseline de performance.

---

## 9. Como restaurar este checkpoint

Se for necessário voltar exatamente a este estado:

1. **Conferir VERSION = 0.6.0:** `cat VERSION`.
2. **Conferir README.md linha 3 = Version: 0.6.0:** `sed -n '3p' README.md`.
3. **Conferir ABI:** `grep "Current ABI version" docs/compatibility.md`.
4. **Conferir test count:** `grep -c "^static void test_" tests/test_widget_contracts.c` deve retornar `48` (47 tests + 1 helper `test_free`).
5. **Conferir build:** `make clean && make validate` deve retornar `0`.
6. **Conferir docs:** `find docs/roadmap -type f -name '*.md' | wc -l` deve retornar `63`.

Se algum desses falhar, o checkpoint foi tocado externamente.

---

## 10. Referências cruzadas

- **Plano-mestre:** `@/Users/t808981/.windsurf/plans/capyui-platform-master-roadmap-f5bdbf.md`
- **Status atualizado:** `docs/roadmap/STATUS.md`
- **Estado absoluto:** `docs/roadmap/tracking/ABSOLUTE.md`
- **Histórico de releases:** `docs/roadmap/tracking/CHANGELOG.md`
- **Decisões arquiteturais:** `docs/roadmap/tracking/DECISIONS.md`
- **Bloqueios:** `docs/roadmap/tracking/BLOCKERS.md`
- **Métricas:** `docs/roadmap/tracking/METRICS.md`
- **Contratos:** `docs/roadmap/contracts/`
- **Dependências:** `docs/roadmap/dependencies/`
- **Próxima fatia (bloqueada):** `docs/roadmap/medium-term/v0.7-host-adapter-gate-etapa4.md`
