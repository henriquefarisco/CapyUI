# Contrato: acoplamento do desktop-session ao CapyOS

**Since:** auditoria 2026-05-29 (pós `v2.13.1`)
**Status:** descreve o estado atual + decisão pendente (ver ADR-0007)
**Escopo:** `src/desktop/`, `src/window/`, `src/apps/` (módulo `org.capyos.ui.desktop-session`, ABI `capy-ui-desktop-session` v1).

> Este documento **não** se aplica a `src/widget/` (módulo `org.capyos.ui.widget-core`),
> que permanece portátil e só inclui `<stddef.h>`/`<stdint.h>` + headers locais
> `capy_*.h`. A invariante de decoupling do núcleo é agora verificada por
> `make check-decoupling` (parte de `make validate`).

## Por que este doc existe

A regra `10-decoupling-discipline` afirma que o desktop-session "may reference
CapyOS **adapter headers** in `CapyOS/include/{gui,apps}/`". Uma auditoria dos
includes reais mostra que a superfície consumida é **bem mais larga** que
`gui/` + `apps/`: o desktop-session inclui headers de kernel, drivers, fs, net,
auth, arch e serviços do CapyOS. Ou seja, ele é efetivamente **código de kernel
CapyOS que reside neste repo** após a migração `alpha.241`, compilado apenas
pelo caminho cross-repo (`CapyOS make all64 PROFILE=full`).

Isto tem duas consequências operacionais:

1. **Sem cobertura local.** O `Makefile` só compila/testa `SRC_WIDGET`. As
   ~9,5k LOC de `src/desktop|window|apps` **não** são compiladas nem testadas
   por `make validate`; regressões só aparecem no build do CapyOS.
2. **Fronteira de decoupling divergente da documentada.** A regra precisa ou
   ser ampliada para refletir a superfície real, ou o acoplamento precisa ser
   reduzido a um adapter mais estreito. Ver "Decisão pendente".

## Superfície de include real (evidência)

Grupos de headers CapyOS incluídos por `src/desktop|window|apps` (snapshot
2026-05-29; cada grupo é resolvido pelo `-I` do build CapyOS, não por este repo):

| Namespace | Headers (amostra) | Comentário |
|---|---|---|
| `gui/` | `compositor.h`, `font.h`, `widget.h`, `taskbar.h`, `desktop.h`, `desktop_icons.h`, `context_menu.h`, `inline_prompt.h`, `capyui_display_adapter.h`, `desktop_runtime.h`, `notification.h`, `event.h`, `window_manager.h`, `window_dispatcher.h`, `terminal.h` | Superfície de adapter "esperada" pela regra 10 |
| `apps/` | `file_manager.h`, `settings.h`, `text_editor.h`, `task_manager.h`, `calculator.h` | Superfície de adapter "esperada" pela regra 10 |
| `auth/` | `session.h`, `user.h`, `privilege.h`, `user_prefs.h`, `user_home.h` | **Além** da fronteira documentada |
| `fs/` | `vfs.h`, `buffer.h` | **Além** da fronteira documentada |
| `net/` | `stack.h` | **Além** da fronteira documentada |
| `memory/` | `kmem.h` | **Além** da fronteira documentada |
| `kernel/` | `task.h`, `process.h`, `scheduler.h`, `task_iter.h`, `process_iter.h` | **Além** da fronteira documentada |
| `drivers/` | `input/mouse.h`, `input/keyboard.h`, `input/keyboard_layout.h`, `timer/pit.h`, `rtc/rtc.h`, `acpi/acpi.h` | **Além** da fronteira documentada |
| `arch/x86_64/` | `kernel_shell_dispatch.h`, `kernel_runtime_control.h`, `framebuffer_console.h`, `apic.h` | **Além** da fronteira documentada |
| `services/` | `update_agent.h`, `service_manager.h` | **Além** da fronteira documentada |
| `shell/`, `core/`, `lang/`, `util/` | `shell/core.h`, `core/system_init.h`, `core/version.h`, `lang/app_language.h`, `lang/localization.h`, `util/kstring.h` | **Além** da fronteira documentada |
| local | `internal/*_internal.h`, `internal/app_display_list_bridge.h`, `capy_display_list.h` | Headers internos do próprio repo (`-Isrc/widget`, `-Isrc`) |

Regenerar a evidência:

```sh
grep -rhoE '#include "[a-z0-9_]+/[^"]+"' src/desktop src/window src/apps \
  | sort | uniq -c | sort -rn
```

## Gates de validação

| Gate | Onde roda | O que cobre |
|---|---|---|
| `make check-decoupling` | local / CI CapyUI | Garante que **`src/widget`** não regrida (zero headers CapyOS). Agora em `validate`. |
| `make lint-desktop-session CAPYOS_INCLUDE=...` | externo (precisa checkout CapyOS) | Best-effort `-fsyntax-only` do desktop-session. **Não** está em `validate`. |
| `CapyOS make all64 PROFILE=full` | externo (CI CapyOS) | **Gate canônico**: compila o desktop-session pelo caminho cross-repo. |
| `CapyOS make all64 PROFILE=core-only` | externo (CI CapyOS) | Confirma que o desktop-session é totalmente excluído. |
| `CapyOS make smoke-x64-vmware-apps-basic-roundtrip` | externo (lab) | Comportamento de apps/dispatcher/window-manager. |

> Política local (`05-local-execution-policy`): nenhum desses comandos roda
> nesta máquina. São recomendações para humano/CI.

## Decisão pendente

A regra `10-decoupling-discipline` e este doc estão em conflito de escopo. Há
duas saídas, rastreadas em ADR-0007:

- **Opção A — aceitar e documentar.** Reconhecer que o desktop-session é
  código co-localizado do CapyOS e que sua "fronteira" é o conjunto de headers
  acima. Ampliar a regra 10 e adicionar `make check-decoupling` + o gate
  cross-repo como guardas. (Menor esforço; não muda código.)
- **Opção B — estreitar o adapter.** Rotear todo o acesso a kernel/drivers/fs/
  net/auth por um único header de adapter (`gui/capyui_display_adapter.h` ou um
  novo `gui/desktop_host.h`), reduzindo a superfície a `gui/` + `apps/` como a
  regra afirma hoje. (Maior esforço; refactor faseado.)

Até a decisão, vale a Opção A de fato: o desktop-session compila só via CapyOS,
e `check-decoupling` protege apenas o núcleo portátil.
