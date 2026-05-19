# Requisitos de plataforma

## Toolchain

- **Compilador:** C11 conforme (`gcc >= 7`, `clang >= 6`).
- **Flags obrigatórias de build:**
  ```
  -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
  ```
- **Flags obrigatórias de segurança (target `security`):**
  ```
  -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE
  ```
- **Build system:** GNU Make (não introduzir CMake, Bazel, Meson no core).

## Tipos básicos suportados

CapyUI assume:

- `uint8_t`, `uint16_t`, `uint32_t`, `int8_t`, `int16_t`, `int32_t` (sizes garantidos).
- `size_t` em range `>= 32 bits`.
- Ponteiros de tamanho `>= 32 bits`.
- Endianness arbitrário (não importa: não fazemos serialização binária no core).

## Não-requisitos

- **Sem requisito de SO:** core não chama syscalls; pode rodar em userland Linux, kernel CapyOS, microcontrolador, navegador (via emscripten).
- **Sem requisito de memória mínima:** allocator injetado decide; testes usam 8KB heap por exemplo.
- **Sem requisito de threads:** API single-threaded; sincronização do host.
- **Sem requisito de clock:** ticks fornecidos pelo host.

## Política sobre extensões e padrões

- C11 estrito; nada de `_Generic`, VLA com tamanho variável em runtime, designated initializers fora de structs.
- Sem extensões GNU (`__attribute__` apenas em headers via macro condicional).
- Sem inline assembly.
- Sem char signed/unsigned dependent (preferir `uint8_t` explícito).

## Hot paths sem float

As seguintes áreas **não** podem usar `float` ou `double`:

- `capy_widget_measure`, `capy_widget_arrange` (layout).
- `capy_widget_emit` (display-list).
- `capy_widget_handle_event` (event routing).
- `capy_anim_sample` (use fixed-point 16.16 ou 24.8).
- Hit-testing.

Allowed em ferramentas externas e testes que rodam fora do binário core.

## Empacotamento

- Source-only por enquanto; releases via tag GitHub.
- Distribuição binária pós-`v1.0.0` segue contrato de release CapyOS.
- ABI versionado em soname quando virar shared library.

## Suporte a arquiteturas

- **Primário:** x86_64 (host de testes, máquinas dev).
- **Secundário:** aarch64 (CapyOS futuro, mobile-like).
- **Verificado periodicamente:** i386, riscv64 (não bloqueiam release).

## Compatibilidade ABI entre arquiteturas

- Alinhamento de structs documentado em `contracts/widget-model.md`.
- Padding explícito quando necessário (`reserved` fields).
- Nenhuma serialização binária implícita do core (caller faz se quiser, com schema).
