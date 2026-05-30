# CapyUI — Decisões arquiteturais (ADR-lite)

Registro cronológico de decisões com justificativa e contexto. Cada entrada é imutável após registro; revisões viram nova entrada.

## Formato

```markdown
## ADR-NNNN — <Título>

**Data:** YYYY-MM-DD
**Status:** proposed | accepted | superseded by ADR-MMMM
**Contexto:** <situação>
**Decisão:** <o que foi escolhido>
**Consequências:** <trade-offs aceitos>
```

---

## ADR-0001 — Núcleo retido em C11 estrito sem dependências externas

**Data:** 2026 (baseline `v0.0.1`)
**Status:** accepted
**Contexto:** CapyUI precisa ser portátil entre CapyOS, ambientes embarcados e testes host puros, sem amarrar a compositor/fontes/I/O do CapyOS.
**Decisão:** Implementar tudo em C11 estrito (`-std=c11 -Wall -Wextra -Werror -pedantic`), apenas `<stddef.h>` e `<stdint.h>`, sem libc heavy, sem float em hot paths, sem `malloc` interno após init.
**Consequências:** Maior portabilidade e testabilidade; código mais verboso; alguns padrões modernos C indisponíveis; uso de allocator injetado obriga callers a gerenciar memória.

## ADR-0002 — Determinismo absoluto como invariante

**Data:** 2026 (baseline `v0.0.1`)
**Status:** accepted
**Contexto:** Testes em host devem reproduzir comportamento bit-a-bit do que rodará sob CapyOS, sem clock real ou I/O.
**Decisão:** Toda transição do modelo retido é determinística dado o mesmo input. Display-list emit gera sequência byte-idêntica para mesma árvore + bounds. Animação usa ticks discretos fornecidos pelo host. Fixtures comparam bytes exatos.
**Consequências:** Debugging por fixture viável; uso de float proibido em hot paths; recursos não-determinísticos (random, time) precisam vir via callback do host.

## ADR-0003 — ABI aditiva até v1.0

**Data:** 2026 (planejamento)
**Status:** accepted
**Contexto:** Múltiplos consumidores (CapyOS, testes, futuros hosts) precisam compatibilidade durante evolução.
**Decisão:** Até `v1.0.0` toda mudança no ABI `capy-ui-widget` é aditiva (campos novos no final de structs, enum entries novos, funções novas). Nenhuma renomeação, nenhuma reordenação, nenhuma remoção. Pós-1.0 segue política de deprecação ≥2 minor releases.
**Consequências:** Algumas APIs ficam imperfeitas até v2.0; possível dívida arquitetural acumulada; ganho de estabilidade absoluta para consumidores.

## ADR-0004 — Display-list com schema versionado e ops aditivas

**Data:** 2026 (`v0.2.0`)
**Status:** accepted
**Contexto:** Display-list é o ponto de contato com compositor; mudanças quebram renderização.
**Decisão:** `CAPY_DISPLAY_LIST_SCHEMA_VERSION` exposto em header e em `dl->version`. Consumidores devem aceitar versões maiores e ignorar ops desconhecidas. Op enum cresce apenas no final.
**Consequências:** Compositor pode rodar contra CapyUI mais novo sem rebuild; mudança de schema major exige nova versão major do ABI.

## ADR-0005 — Adoção de roadmap estruturado com tracking absoluto

**Data:** 2026-05-19
**Status:** accepted
**Contexto:** Roadmap inicial em `~/.windsurf/plans/capyui-desktop-stack-roadmap-f5bdbf.md` cobria v0.0.1→v1.0.0 mas faltava granularidade pós-1.0, segregação por horizonte e tracking absoluto consultável dentro do repo.
**Decisão:** Reestruturar em `docs/roadmap/` com `short-term/`, `medium-term/`, `long-term/`, `contracts/`, `dependencies/`, `tracking/`. Cada fatia ganha arquivo dedicado com template fixo. ABSOLUTE.md atualizado a cada release tag.
**Consequências:** Mais arquivos para manter; visibilidade maior do estado do projeto; novos colaboradores conseguem navegar sem leitura de plano gigante; histórico de decisões e métricas versionado junto ao código.

## ADR-0006 — Remoções em 3.0 exigem deprecação prévia (≥2 minors) ou exceção formal

**Data:** 2026-05-29
**Status:** accepted
**Contexto:** A fase 2.x foi declarada completa (14/14, 2.0–2.13) **sem nenhuma deprecação** — o único uso de `CAPYUI_API_DEPRECATED` no header é a definição da macro. Ao mesmo tempo, `long-term/v3.0-vision-platform.md` lista "remoções de APIs deprecadas em 2.x" como parte da quebra controlada. A `deprecation-policy.md` (em vigor desde 1.0) exige **≥2 minor releases** entre a marcação de deprecação e a remoção num major. Logo, hoje **não existe** símbolo elegível para remoção em 3.0.
**Decisão:** Antes de qualquer remoção/renomeação em 3.0:
1. Inventariar os símbolos-alvo (dívida acumulada do período aditivo 0.x→2.x).
2. Para cada um, abrir uma janela de deprecação em minors **2.14+** (`CAPYUI_API_DEPRECATED` + CHANGELOG + compatibility doc), mantendo o substituto aditivo por ≥2 minors; **ou** registrar um ADR de exceção (bug crítico / vulnerabilidade) conforme a política.
3. Só então 3.0.0 pode remover. Enquanto isso, 3.0 só pode crescer **aditivamente** (novas ops DL 3D, novos eventos voice/AI, etc.).
**Consequências:** A frase "fase 2.x COMPLETA" passa a significar "completa em features aditivas", não "congelada para sempre" — minors 2.14+ de deprecação continuam permitidos e são, na prática, pré-requisito do 3.0. O guard automático de ABI (ADR-... / `tools/abi_guard.sh` + workflow) passa a falhar o build se um símbolo público sumir num minor sem ter sido deprecado antes.

## ADR-0007 — Desktop-session é código co-localizado do CapyOS (Opção A, interina)

**Data:** 2026-05-29
**Status:** accepted (interina; reavaliar quando Etapa 4/6 do CapyOS fecharem)
**Contexto:** Auditoria de includes (ver `contracts/desktop-session-coupling.md`) mostra que `src/desktop|window|apps` consomem headers CapyOS bem além de `gui/`+`apps/` (incluindo `kernel/`, `drivers/`, `fs/`, `net/`, `auth/`, `arch/x86_64/`, `services/`, `shell/`, `memory/`, `core/`, `lang/`, `util/`). A regra `10-decoupling-discipline` afirma uma fronteira mais estreita do que a real, e o `Makefile` não compila/testa esse módulo localmente (só `SRC_WIDGET`).
**Decisão:** Adotar a **Opção A** por ora: reconhecer o desktop-session como código de kernel CapyOS co-localizado neste repo após a migração `alpha.241`, compilado apenas pelo caminho cross-repo. Concretamente:
1. `make check-decoupling` protege **apenas** `src/widget/` (núcleo portátil) e entra em `make validate`.
2. `make lint-desktop-session CAPYOS_INCLUDE=...` fica como gate externo best-effort; o gate canônico é `CapyOS make all64 PROFILE=full`.
3. `contracts/desktop-session-coupling.md` documenta a superfície real como o contrato vigente.
**Consequências:** Decoupling do núcleo permanece garantido e agora automatizado; o desktop-session continua sem cobertura local (risco aceito e documentado). A **Opção B** (estreitar tudo a um único adapter `gui/desktop_host.h`) fica registrada como refactor futuro, candidato natural quando o adapter da Etapa 4 do CapyOS for implementado.
