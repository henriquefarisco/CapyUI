# Política de breaking changes

Regras formais sobre quando é permitido quebrar API/ABI e como comunicar.

## Pré-1.0 (atual)

**Tudo aditivo. Zero quebras permitidas.**

Especificamente:

- ✅ **Permitido:** adicionar campo no final de struct pública.
- ✅ **Permitido:** adicionar entry no final de enum público.
- ✅ **Permitido:** adicionar nova função pública.
- ✅ **Permitido:** adicionar novo header.
- ✅ **Permitido:** adicionar opcionalidades em ops existentes (campo `reserved` virando válido).
- ❌ **Proibido:** renomear símbolo público.
- ❌ **Proibido:** remover símbolo público.
- ❌ **Proibido:** mudar assinatura de função pública.
- ❌ **Proibido:** mudar ordem de campos em struct pública.
- ❌ **Proibido:** mudar tamanho de tipo público.
- ❌ **Proibido:** mudar semântica de função existente.

**Justificativa para a regra dura:** durante v0.x, consumidores (CapyOS, testes, futuros hosts) precisam confiar que upgrades não quebram nada.

## v1.0.0 (gate de congelamento)

Marco onde:

- ABI `capy-ui-widget 1.0` declarado **estável**.
- Política de deprecação formal entra em vigor.
- 30 dias contínuos de CI verde em todas as v0.x.
- Matriz CapyUI × CapyOS consolidada.

## Pós-1.0 (v1.1+ e v2.0+)

### Deprecação obrigatória antes de remoção

Para remover ou mudar semântica de símbolo público:

1. **Minor N:** marcar com `CAPYUI_API_DEPRECATED` macro + comentário no header.
2. **Minor N:** atualizar `CHANGELOG.md` com aviso de deprecação.
3. **Minor N+1:** manter funcionando, emitir warning de compilação.
4. **Minor N+2 (mínimo):** remoção permitida em release major (`v2.0.0`).

Total: **≥2 minor releases entre deprecação e remoção**, e remoção só em major bump.

### Quebra de ABI majo

Apenas em transições `v1.x → v2.0`, `v2.x → v3.0`, etc., com:

- Guia de migração detalhado em `tracking/CHANGELOG.md`.
- Compatibilidade source via shim quando possível.
- LTS da major anterior por **≥12 meses**.
- Anúncio público antes da release.

### Schema do display-list

Caso especial: schema é versionado em `dl->version`. Consumidores devem:

- Aceitar versões maiores e ignorar ops desconhecidas.
- Falhar limpo (não-crash) se schema for menor que esperado.

Schema major bump (`v1` → `v2`) só em release major de CapyUI.

## Exceções controladas

Pré-1.0, exceção permitida se:

- ✅ Bug crítico de correção (semântica nunca foi documentada como contrato).
- ✅ Vulnerabilidade de segurança (correção urgente).
- ✅ Decisão registrada como ADR em `tracking/DECISIONS.md`.

Toda exceção exige changelog destacado e comunicação aos consumidores conhecidos.

## Como comunica

- `CHANGELOG.md`: entrada explícita na seção `### Deprecado` ou `### Removido`.
- Header: comentário `/* DEPRECATED in 1.N: use foo_v2 instead. Will be removed in 2.0. */`.
- Compatibility doc: matriz atualizada.
- Release notes (pós-1.0): destaque na nota da release.

## Verificação automática

Pós-1.0, CI deve incluir:

- Diff de headers públicos vs. último tag minor.
- Detecção de remoção/renomeação não-intencional.
- Validação de macros de deprecação corretas.

Falha bloqueia merge.
