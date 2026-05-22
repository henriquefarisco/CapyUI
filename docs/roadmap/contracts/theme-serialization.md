# Contrato: Theme serialization

**Since:** 0.14.0 (delivered)
**Status:** delivered (CapyUI side); persistência em `~/.config/capyos/theme.conf`, painel de configurações e watcher seguem trilha CapyOS
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

- `CAPY_THEME_FORMAT_VERSION` constante (=2) exporta a versão atual do formato textual.
- `serialize` produz string canônica determinística (mesma `theme` → mesma string byte-a-byte) com ordem fixa: `version=2`, `variant=...`, `high_contrast=0|1`, depois cada token enum (1..COUNT-1) na ordem de declaração.
- Cores em `serialize` saem como `0xAARRGGBB` uppercase (8 dígitos hex zero-padded).
- `version` no arquivo segue **decimal**; o restante das cores segue **hex**.
- `serialize` retorna número de bytes escritos (excl. NUL); `-1` em overflow, `NULL t/out`, `cap == 0`, ou `variant` inválido.
- Saída sempre NUL-terminada em sucesso (1 byte reservado).
- `deserialize` aceita formato canônico + tolerância a whitespace (leading/trailing por linha, ao redor do `=`), commentários `#` e linhas em branco. Aceita `0xPREFIX` opcional no valor hex.
- Versão `0` → `-1`; versão `> CAPY_THEME_FORMAT_VERSION` → `-1`.
- Linha `version=` ausente → `-1`.
- Token desconhecido (key não-mapeada) → **ignorado controladamente** (não retorna -1).
- Malformados: linha sem `=`, hex inválido, `variant` desconhecido, `high_contrast` != 0/1, empty key, `NULL out/in` → `-1`.
- Tokens ausentes no input ficam zero em `out` (caller pode seedar com `capy_theme_default_light()` antes para herdar defaults).
- Capacity overflow em `serialize` → retorna `-1` (sem rollback formal; o conteúdo parcial em `out` não tem garantia de NUL-terminação).

## Invariantes

- Round-trip: `theme → serialize → deserialize → theme'`, com `theme == theme'` em todos tokens.
- Token enum cresce; serialize tolera tokens novos em deserialize de versões antigas.

## Testes que cobrem o contrato

- `test_theme_serialize_round_trip` — `default_dark` → serialize → deserialize → tokens/variant/high_contrast equivalentes.
- `test_theme_serialize_determinism` — mesma theme → mesma saída byte-a-byte em runs independentes.
- `test_theme_serialize_overflow_returns_minus_one` — buffer pequeno, NULL t/out, cap=0, variant inválido todos retornam `-1`.
- `test_theme_deserialize_rejects_future_version` — `version=3` → `-1`.
- `test_theme_deserialize_rejects_malformed` — linha sem `=`, hex inválido, variant desconhecido, bool `2`, `version=` ausente, NULL out/in todos retornam `-1`.
- `test_theme_deserialize_ignores_unknown_keys` — chave `future_token_v9` desconhecida é ignorada sem afetar o resto; tokens ausentes ficam zero.
- `test_theme_deserialize_tolerates_whitespace_and_comments` — commentários `#`, linhas em branco, espaços ao redor de chave/valor todos tolerados.

## Histórico

| Versão | Mudança |
|--------|---------|
| 0.14.0 | Introdução do formato textual canônico (key=value line-oriented), constante `CAPY_THEME_FORMAT_VERSION = 2`, APIs `capy_theme_serialize`/`capy_theme_deserialize`, defaults bumpados de `version=1` para `version=2` para alinhar com o formato. Lado CapyUI entregue. |
| 2.4.0 (planned) | Theme packs (formato binário evoluído) |
