# CapyUI — Estado Absoluto do Projeto

Documento vivo, atualizado a cada release tag.

**Última atualização:** 2026-05-19
**Versão atual:** `0.6.0`
**ABI:** `capy-ui-widget 0.6`
**Release gate:** `make validate` **VERDE** em 2026-05-19.
**Checkpoint consolidado:** `CHECKPOINT-v0.6.0.md` (na raiz de `docs/roadmap/`).

## Pipeline

- **Em produção (tagged, validated):** `0.6.0`
- **Em validação:** —
- **Próxima fatia:** `v0.7.0 — Adapter CapyOS` (bloqueada por gate CapyOS Etapa 4)
- **Bloqueios:** Etapa 4 do roadmap CapyOS deve fechar antes de iniciar v0.7.0 (ver `BLOCKERS.md` e `dependencies/capyos-gates.md`)

## Cobertura por fase

| Fase | Faixa | Concluído | Em andamento | Pendente |
|------|-------|-----------|--------------|----------|
| Baseline | v0.0.1 | 1 | 0 | 0 |
| Curto | v0.3–v0.6 | **6 (TODAS)** | 0 | 0 |
| Médio | v0.7–v1.0 | 0 | 0 | 10 |
| Longo | v1.1–v3.0 | 0 | 0 | 17 |

> **Marco:** fase de curto prazo (núcleo retido portátil v0.0.1→v0.6.0) **integralmente entregue**. Próximo trabalho aguarda gate CapyOS Etapa 4 para iniciar fase média.

## Métricas-chave (snapshot)

- Testes: 47 funções em `tests/test_widget_contracts.c`
- LOC core: ~2.3k (`src/widget/*.c` + `*.h`)
- LOC tests: ~1280
- ABI versions ativas: 0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6
- Display-list schema: 2 (sem mudança em v0.6; theme é ponteiro opcional)
- Etapa CapyOS bloqueante: **Etapa 4** (apenas para v0.7+)
- CI green streak: contínuo desde `v0.0.1`

## Diretrizes inquebráveis

- Núcleo portátil C11 estrito, zero malloc interno após init, zero I/O.
- Adições ABI aditivas até 1.0; deprecação ≥2 minor releases pós-1.0.
- `make validate` verde obrigatório em qualquer merge.
- Nenhum include de runtime CapyOS dentro de CapyUI.
- Zero float em hot paths.
- Determinismo: mesma entrada → mesma saída byte-a-byte.

## Pendências críticas

Nenhuma técnica no momento. **Curto prazo completo.** Trabalho de médio prazo (v0.7.0+) aguarda gate externo CapyOS Etapa 4.

## Histórico de releases

Ver `CHANGELOG.md`.

## Política de atualização deste arquivo

Atualizar a cada tag de release com:
1. Versão atual e ABI.
2. Tabela de cobertura.
3. Métricas-chave atualizadas (rodar `wc -l` sobre `src/`, `tests/`).
4. Status dos bloqueios.
5. Próxima fatia.

Não substituir histórico — preservar como `# Release 0.X.Y` blocos quando relevante.
