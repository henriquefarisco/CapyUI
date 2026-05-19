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
