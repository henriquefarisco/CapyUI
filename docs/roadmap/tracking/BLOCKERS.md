# CapyUI — Bloqueios atuais

**Última atualização:** 2026-05-21 (pós-freeze 1.0)

## Estado

Nenhum bloqueio ativo para trabalho **portátil dentro de 1.x**. ABI `capy-ui-widget` está **congelada em 1.0**; novas features entram como minors aditivas (1.1, 1.2, ...) seguindo a `deprecation-policy.md`.

Gates externos `Etapa 4` (CapyOS) e `Etapa 6` (CapyOS) permanecem abertos, mas afetam apenas o adapter CapyOS (v0.7, código mora em `CapyOS/src/ui_adapter/`, agora um bump metadata-only sobre o ABI 1.x já congelado) e a shell integration (v0.12, mesmo status). Nenhum deles bloqueia novas fatias longo-prazo (v1.1 damage tracking, v1.3 rich animation, v1.10 serialização, v2.3 undo/redo) que operam puramente no núcleo retido.

Os gates de release pendentes para a **tag externa `v1.0.0`** (não bloqueiam o commit do freeze):
- CI verde 30 dias contínuos sobre `1.0.0`.
- Soak da suite (97 testes) 1000× consecutivas sem flakiness.
- Audit cross-repo: `CapyOS/docs/reference/integration/compatibility-audit-2026-05-2X.md`.
- Tag `v1.0.0` Ed25519 assinada quando o signer CapyAgent estiver pronto.
- Comunicado público de release notes.

## Formato esperado

Quando houver bloqueio, registrar como:

```markdown
## [BLK-N] <Título curto>

**Aberto em:** YYYY-MM-DD
**Fatia afetada:** vX.Y.Z
**Severidade:** crítico | alto | médio | baixo
**Responsável:** <nome>
**Origem:** CapyUI | CapyOS | externo
**Status:** aberto | mitigado | resolvido

### Descrição
<descrição do problema>

### Impacto
<o que está parado>

### Mitigação
<o que estamos fazendo>

### Resolução (quando fechado)
<o que destravou>
```

## Bloqueios típicos esperados

- Gate Etapa 4 CapyOS (afeta `v0.7.0`).
- Gate Etapa 6 CapyOS (afeta `v0.12.0`).
- Regressão de determinismo (afeta qualquer fatia, severidade crítica).
- Quebra de ABI não-aditiva detectada em revisão (bloqueia merge).
- Performance budget excedido em CI de benchmark (afeta `v0.15`+).
