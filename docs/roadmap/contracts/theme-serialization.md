# Contrato: Theme serialization

**Since:** 0.14.0 (planned)
**Status:** planned
**Fatia:** `medium-term/v0.14-theming-ux.md`

## APIs

```c
int capy_theme_serialize(const struct capy_theme *t,
                         char *out, uint32_t cap);
int capy_theme_deserialize(struct capy_theme *out,
                           const char *in, uint32_t len);
```

## Formato canônico

```
version=2
variant=dark
high_contrast=0
bg_base=0xFF1A1B1F
bg_raised=0xFF24262C
bg_sunken=0xFF14161A
fg_primary=0xFFE8ECEF
fg_muted=0xFF8B95A1
fg_inverse=0xFF1A1B1F
accent=0xFF3B82F6
accent_hover=0xFF60A5FA
border=0xFF4B5563
border_focus=0xFF3B82F6
focus_ring=0xFF60A5FA
danger=0xFFEF4444
warning=0xFFF59E0B
success=0xFF22C55E
info=0xFF60A5FA
disabled=0xFF6B7280
```

Regras:

- Uma chave por linha.
- Separador: `=`.
- Cores em hex `0xAARRGGBB`.
- Linhas em branco e `#` comentários ignorados.
- Ordem das chaves canônica (sempre na mesma ordem em serialize).
- Encoding: ASCII puro (sem multibyte).

## Semântica

- `serialize` produz string canônica determinística (mesma `theme` → mesma string byte-a-byte).
- `deserialize` aceita formato canônico + tolerância a whitespace.
- Versão desconhecida (`version > expected`) → retorna `-1`.
- Token desconhecido (linha com key não-mapeada) → ignorado controladamente.
- Capacity overflow em `serialize` → retorna `-1` sem corromper.

## Invariantes

- Round-trip: `theme → serialize → deserialize → theme'`, com `theme == theme'` em todos tokens.
- Token enum cresce; serialize tolera tokens novos em deserialize de versões antigas.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.14.0 (planned) | Introdução do formato textual |
| 2.4.0 (planned) | Theme packs (formato binário evoluído) |
