# Contrato: Política de deprecação

**Since:** 1.0.0 (em vigor)
**Status:** stable

## Regras

### Pré-1.0

- Sem deprecações; tudo é aditivo.
- Quebras só via ADR formal + exceção controlada.

### 1.x → 2.0

1. **Minor N (introdução do substituto):** adicionar nova API + manter antiga.
2. **Minor N+1 (deprecação formal):** marcar antiga com `CAPYUI_API_DEPRECATED`. CHANGELOG anuncia.
3. **Minor N+2 (último aviso):** warning de compilação ao usar antiga.
4. **Major 2.0:** remoção da antiga permitida.

**Mínimo: 2 minor releases entre deprecação e remoção.**

### 2.x → 3.0

Mesma regra. LTS de 2.x por **≥12 meses** pós-3.0.

## Macros

```c
#if defined(__GNUC__) || defined(__clang__)
  #define CAPYUI_API_DEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
  #define CAPYUI_API_DEPRECATED(msg) __declspec(deprecated(msg))
#else
  #define CAPYUI_API_DEPRECATED(msg)
#endif
```

Uso em headers:

```c
CAPYUI_API_DEPRECATED("Use capy_widget_new_api() since 1.5")
int capy_widget_old_api(struct capy_widget *w);
```

## Comunicação

- **Header:** comentário `/* DEPRECATED in 1.N: use foo_v2 instead. Will be removed in 2.0. */` antes do símbolo.
- **CHANGELOG.md:** entrada destacada na seção `### Deprecado`.
- **Compatibility doc:** lista de deprecações ativas.
- **Release notes:** call-out na release que introduz a deprecação.

## Exceções

Permitidas apenas com:

- ADR em `tracking/DECISIONS.md` justificando.
- Aprovação manual.
- Comunicação destacada em release notes.

Casos típicos:

- **Bug crítico:** correção de semântica nunca documentada como contrato.
- **Vulnerabilidade de segurança:** correção urgente; janela de deprecação compactada.

## Display-list schema

Caso especial:

- Schema **só cresce** (novas ops no final).
- Consumidores **devem aceitar** versões maiores e ignorar ops desconhecidas.
- Schema **major bump** (incompatibilidade real) apenas em release **major** de CapyUI.

## Verificação automática

CI pós-1.0 deve:

1. Comparar headers públicos vs. último tag minor.
2. Detectar remoção/renomeação de símbolos públicos.
3. Validar uso correto de `CAPYUI_API_DEPRECATED`.
4. Falhar build se símbolo deprecado removido em minor (apenas major pode remover).

## Como reportar uso de API deprecada

Consumidores devem checar warnings de compilação. Para programaticamente:

```sh
make 2>&1 | grep -i "deprecated"
```

Migração documentada em `docs/migration-guide-X.Y.md` (gerado por release).
