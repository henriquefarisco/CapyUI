# Dependências de terceiros

CapyUI tem política de **zero dependências externas em runtime**.

## Regra dura

O core CapyUI (`src/widget/`) só pode incluir:

- `<stddef.h>` (apenas `size_t`, `NULL`).
- `<stdint.h>` (apenas `uint8_t`, `uint16_t`, `uint32_t`, `int32_t`, etc.).

Qualquer outro include externo está **proibido** e será detectado pelo CI.

## Por quê

- **Portabilidade absoluta:** CapyUI roda em qualquer host com toolchain C11 mínimo, sem libc completa.
- **Determinismo:** dependências externas podem variar entre plataformas (memcpy, malloc, locale).
- **Auditabilidade:** todo código de runtime é nosso e revisado.
- **Segurança:** menor superfície de ataque; nenhum CVE de terceiros afeta CapyUI.
- **Tamanho:** binário compacto sem overhead de libc.

## O que é permitido

- **Compile-time:** ferramentas de build (`make`, `cc`) podem usar o que quiserem.
- **Tests:** podem usar funcionalidades padrão de testes em C, desde que não sejam linkadas no binário core.
- **Allocator do host:** injetado via `capy_widget_allocator` — não é dep do core.
- **Callbacks do host:** medição de texto, clock, I/O — tudo entra via pointer de função, não como dep direta.

## O que é proibido

- `<stdio.h>` (sem I/O no core).
- `<stdlib.h>` (sem malloc/free internos).
- `<string.h>` em hot paths (reimplementamos `memcpy`/`memset` mínimo quando necessário; aceito apenas em código de teste).
- `<math.h>` (sem float).
- `<time.h>` (host fornece tick monotônico).
- `<pthread.h>` (sem threads no core).
- Qualquer header CapyOS (`<gui/*>`, `<kernel/*>`, etc.).
- Qualquer biblioteca externa (zlib, freetype, harfbuzz, etc.).

## Verificação automática

CI deve rodar (em `make lint` ou similar):

```sh
# Detecta includes proibidos no core
grep -rn '#include' src/widget/ \
  | grep -v '<stddef.h>' \
  | grep -v '<stdint.h>' \
  | grep -v '"capy_'
```

Saída esperada: vazia. Qualquer match é falha de CI.

## Quando uma dep externa pode entrar?

Nunca no core. Possíveis exceções pós-1.0:

- **Tooling de devtools (`v2.5`):** parser JSON externo aceitável, mas roda no host de devs, não no app final.
- **Plugin SDK (`v2.0`):** plugins de terceiros podem usar libs deles próprios; CapyUI não carrega nem linka.

Qualquer mudança nesta política exige ADR formal e revisão arquitetural ampla.
