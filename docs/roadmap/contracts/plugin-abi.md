# Contrato: Plugin ABI

**Since:** 2.0.0 (planned)
**Status:** planned
**Fatia:** `long-term/v2.0-plugin-system-sdk.md`

## ABI separada

`capy-ui-plugin` é uma ABI independente de `capy-ui-widget`. Plugins importam apenas headers públicos do plugin SDK.

## Descripto

```c
struct capy_plugin_descriptor {
  const char *id;                  /* "com.example.myplugin" */
  const char *version;             /* "1.0.0" */
  uint32_t capy_ui_abi_required;   /* ex. 0x00020000 = 2.0 */
  uint32_t capy_plugin_abi_required;

  int (*init)(struct capy_plugin_context *ctx);
  void (*destroy)(struct capy_plugin_context *ctx);
  int (*on_event)(struct capy_plugin_context *ctx,
                  struct capy_widget *target,
                  const struct capy_widget_event *ev);
  int (*emit)(struct capy_plugin_context *ctx,
              struct capy_widget *target,
              struct capy_display_list *dl);
};
```

## APIs

```c
int capy_plugin_register(struct capy_widget_context *ctx,
                         const struct capy_plugin_descriptor *desc);
int capy_plugin_unregister(struct capy_widget_context *ctx,
                           const char *plugin_id);
struct capy_plugin_context *capy_plugin_context_create(
    struct capy_widget_context *ctx,
    const struct capy_plugin_descriptor *desc);
```

## Sandbox

Cada plugin recebe:

- Allocator dedicado (não vê memória de outros plugins nem do host).
- Limite de tempo por callback (configurável pelo host).
- Sem acesso ao `capy_widget_context` global do host (apenas a sub-contexto isolado).

Plugin com timeout excedido é marcado como malfunctioning; chamadas subsequentes retornam erro sem executar.

## Custom display-list ops

Schema 8 introduz `CAPY_DL_PLUGIN_OP`:

- Payload genérico de 32 bytes.
- Tag de plugin (uint16_t plugin_op_id).
- Compositor delega para plugin renderer registrado.

## Versionamento

- Plugin declara `capy_ui_abi_required` (mínimo).
- CapyUI rejeita plugins com ABI maior que o seu.
- Plugin com ABI menor é aceito (compatibilidade aditiva pré-major).

## Invariantes

- Crash de plugin **não derruba host** (sandbox).
- Plugin sem `init` falha em `register`.
- IDs únicos: registrar plugin com mesmo ID retorna `-1`.

## Histórico

| Versão | Mudança |
|--------|---------|
| 2.0.0 (planned) | Introdução do plugin system |
