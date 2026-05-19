# Gates externos no CapyOS

CapyUI tem dois gates duros que dependem do roadmap CapyOS.

## Gate Etapa 4 — Adapter (v0.7.0)

**Bloqueia:** entrega de `v0.7.0` em CapyUI.
**Local da decisão:** roadmap CapyOS `docs/plans/active/capyos-master-plan.md` (Etapa 4 — driver framework, scheduler, display abstraction).
**Por que bloqueia:** `v0.7.0` introduz o adapter CapyOS→CapyUI. O adapter vive no lado CapyOS (`CapyOS/src/ui_adapter/`) e consome o ABI `capy-ui-widget`. Sem driver framework + scheduler + display abstraction maduros no CapyOS, o adapter não tem onde plugar.

**Critério de liberação:**

- Etapa 4 fechada (código + docs + smoke externo).
- `CapyDisplay 2D` operacional (surface, framebuffer, blit).
- Scheduler cooperativo entregando frames.
- USB HID maduro permitindo input real (teclado, mouse).

**O que CapyUI pode fazer sem o gate:** v0.1–v0.6 são 100% independentes. Núcleo retido evolui em paralelo.

**O que CapyUI exporta para o adapter:**

- ABI `capy-ui-widget 0.7+` estável.
- Display-list schema v2 (com `FOCUS_RING`).
- Eventos abstratos (pointer + key + modificadores).
- Tema com tokens (variantes light/dark/high-contrast).

**Documento espelhado:** `CapyOS/docs/reference/integration/capyui-widget-integration-contract.md`.

## Gate Etapa 6 — Shell integration (v0.12.0)

**Bloqueia:** entrega de `v0.12.0` em CapyUI.
**Local da decisão:** roadmap CapyOS Etapa 6 (apps básicos do desktop maduros + integração de shell).
**Por que bloqueia:** `v0.12.0` introduz tipos aditivos `NOTIFICATION` e `TRAY_ICON`. Esses tipos só fazem sentido com taskbar, dock e sistema de notificações concretos no CapyOS.

**Critério de liberação:**

- Etapa 6 fechada (taskbar, dock, notification stacking, política de foco entre janelas).
- App launcher operacional.
- Pelo menos um app exemplo CapyUI rodando no shell.

**O que CapyUI pode fazer sem o gate:** v0.8–v0.11 podem prosseguir após v0.7. Apenas `v0.12.0` espera.

## Dependências em fase média

| Fatia CapyUI | Depende de | Status atual |
|--------------|------------|--------------|
| v0.7.0 | Etapa 4 CapyOS | Não iniciada |
| v0.8.0 | v0.7.0 + Etapa 4 estável | Aguarda |
| v0.9.0 | v0.8.0 + Etapa 4 + fontes do host | Aguarda |
| v0.10.0 | v0.9.0 + drivers de input concretos | Aguarda |
| v0.11.0 | v0.10.0 + bridge a11y do host | Aguarda |
| v0.12.0 | v0.11.0 + Etapa 6 CapyOS | Aguarda |
| v0.13.0 | v0.12.0 + detecção de locale do host | Aguarda |
| v0.14.0 | v0.13.0 + persistência do host | Aguarda |
| v0.15.0 | v0.14.0 (puro CapyUI, sem host extra) | Aguarda |
| v1.0.0 | v0.15.0 + 30 dias CI verde | Aguarda |

## Política

- Gates não podem ser pulados.
- Trabalho paralelo é permitido: enquanto Etapa 4 estiver em validação, CapyUI pode prototipar `v0.7.0` em branch separado, mas o tag final só sai depois do gate.
- Mudança de ordem ou pulo de gate exige ADR em `tracking/DECISIONS.md` e revisão.
