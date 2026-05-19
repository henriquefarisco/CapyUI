# CapyUI — Bloqueios atuais

**Última atualização:** 2026-05-19

## Estado

Nenhum bloqueio ativo.

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
