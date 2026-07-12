# CapyUI — Changelog

Mudanças por release tag, da mais recente para a mais antiga. Cada entrada é imutável após release.

## [2.24.0] — 2026-07-12

**Estabilidade do desktop:** o chat CapyAI deixa de executar inferência na task gráfica e passa a usar o serviço assíncrono persistente do CapyOS.

### Corrigido

- Fechar e reabrir o chat invalida a geração antiga sem manter referências de janela obsoletas.
- O desktop continua renderizando enquanto a resposta é processada; o resultado chega por polling não bloqueante.
- A integração usa saída limitada e snapshot da sessão, eliminando concorrência sobre buffers globais.
- O gate QEMU cobre frames concorrentes e quatro ciclos de fechar/reabrir o chat.

## [2.22.2] — 2026-06-22

**Aditivo (in-tree):** busca de widget por id (`capy_widget_find_by_id`), pura/aditiva sobre a ABI capy-ui-widget v2.22.

### Adicionado

- `struct capy_widget *capy_widget_find_by_id(struct capy_widget *root, uint32_t id)` (`src/widget/capy_widget.{h,c}`): busca em profundidade (pre-order) pelo primeiro widget cujo `id` (auto-atribuido por contexto) bate; complementa `capy_widget_find_at` (hit-test por coordenada) com enderecamento estavel por id para inspector/automacao/atualizacao dirigida. Puro: sem mutacao, sem efeito em layout/display-list.
- 1 teste (`test_find_by_id`). Suite 347 → **348** (assercoes de contagem no `Makefile` atualizadas).

## [2.22.1] — 2026-06-18

**Aditivo (in-tree, sem bump de versão):** acessor de versão de ABI em runtime + nome de tipo de widget.

### Adicionado

- `uint32_t capy_widget_abi_version(void)` (`src/widget/capy_widget.{h,c}`): retorna `CAPYUI_API_VERSION_TAG` para negociação de versão em runtime por um host dinamicamente ligado, o adapter CapyOS ou um loader de plugin (complementa a macro de compile-time e o gate `capy_ui_abi_required`). Puro, determinístico, zero-alloc/zero-float.
- `const char *capy_widget_type_name(enum capy_widget_type)` (`src/widget/capy_widget.{h,c}`): nome ASCII curto estável por tipo de widget ("BUTTON", "TEXTBOX", ...; "NONE"/"UNKNOWN" nos extremos) para labels de inspector/debug/acessibilidade e diagnósticos de serialização. Puro, determinístico, aditivo.
- 2 testes (`test_abi_version_accessor`, `test_widget_type_name`). Suíte 345 → **347** (asserções de contagem no `Makefile` atualizadas).

### Compatibilidade

- 100% aditivo. Nenhuma struct/enum/op de display-list alterada; DL schema permanece `7`; macros de versão inalteradas (`2/22/0`). Sem bump de `VERSION` (entrega in-tree, pendente de release/tag).

### Validação (no host via `Automation/remote-exec.sh`)

- `remote-exec.sh -d CapyUI make validate` → exit 0 (lint + security + decoupling + 347 testes + version-check).

## [2.22.0] — 2026-06-02

**Marco:** **multi-touch gestures** — completa os 4 slots `PINCH_IN/OUT` + `ROTATE_CW/CCW` reservados desde a v1.4 (gesture engine). Primeira fatia fora da trilha de _advanced-widget state_ (que fechou 8/8 na 2.21); retoma o backlog aditivo deferido. O recognizer agora rastreia um segundo dedo e emite pinch/rotate. Integer-only (zero-float): pinch = delta assinado da separação Chebyshev; rotate = sinal do produto vetorial inteiro (direção) + significância escala-independente `|cross|/dot > 27/100` (~15°). Aditivo sobre 2.21; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/22/0`. `CAPYUI_API_VERSION_TAG = 0x00021600`.
- Tail fields multi-touch em `struct capy_gesture_recognizer`: `pinch_min_distance_px` (caller-tunable, seed 20) + `touch2_active`/`pinch_emitted`/`rotate_emitted`/`reserved2` + `touch2_id` + `touch2_pos` + `multi_v0` (vetor baseline touch1−touch2) + `multi_start_dist` (separação Chebyshev baseline). `sizeof(capy_gesture_recognizer)` cresce (tail aditivo) — recompilar callers que embedam o struct.
- `CAPY_GESTURE_PINCH_IN`/`PINCH_OUT`/`ROTATE_CW`/`ROTATE_CCW` (reservados no enum desde 1.4) **agora emitidos**.
- `capy_gesture_recognizer_init` seeds os novos campos. `capy_gesture_feed`: um 2º `TOUCH_BEGIN` com id distinto abre uma sessão de dois dedos (captura baseline); `TOUCH_MOVE` em qualquer dedo reavalia pinch/rotate (cada um **one-shot por sessão**, como `drag_emitted`); qualquer `TOUCH_END` encerra a sessão e reseta o recognizer; um 3º dedo concorrente é ignorado; o path single-touch fica suprimido durante a sessão.
- Helper estático `capy_gesture_eval_multi` (`src/widget/capy_widget.c`).
- 8 testes (`test_gesture_pinch_out`/`pinch_in`/`rotate_cw`/`rotate_ccw`/`rotate_below_threshold`/`multi_touch_end_resets`/`single_touch_swipe_regression`/`third_finger_ignored`). Suíte 337 → **345**.

### Compatibilidade

- 100% aditivo. Single-touch (tap/double-tap/long-press/swipe/drag) inalterado — regressão coberta por teste. Zero-float (produto vetorial em `int64_t`, sem overflow para coords de tela). DL schema permanece `7`. `capy_widget_serialize` inalterado.

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 345 testes + version-check) e `make package`.

## [2.21.0] — 2026-06-02

**Marco:** **oitava e última fatia da trilha de estado dos advanced widgets** (após date 2.14, color 2.15, table 2.16, autocomplete 2.17, tree 2.18, chart 2.19, rich-text 2.20) — **fecha 8/8 famílias**. Levanta o `CANVAS`: a **primeira família que carrega um comportamento** (callback do host) em vez de um modelo de dados. O core guarda o callback e **nunca** o invoca em `capy_widget_emit` (emit continua self-contained/determinístico); o host chama `capy_widget_canvas_draw` ao montar um frame para o canvas anexar seus próprios ops de display-list. Aditivo sobre 2.20; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/21/0`. `CAPYUI_API_VERSION_TAG = 0x00021500`.
- Typedef `capy_canvas_draw_fn(struct capy_widget *w, struct capy_display_list *dl, void *user_data)` (callback do host; retorna 0 = ok / não-zero = falha).
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_CANVAS`): `capy_canvas_draw_fn canvas_draw` (NULL = sem desenho) + `void *canvas_user_data`. `sizeof` 896 → 912.
- 5 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_set_canvas_draw(w, fn, user_data)` — guarda callback + user_data; `fn==NULL` limpa; fail-closed por tipo.
  - `int capy_widget_clear_canvas_draw(w)` — reseta callback + user_data a NULL.
  - `int capy_widget_has_canvas_draw(w)` — 1 set / 0 unset / -1 NULL/tipo.
  - `int capy_widget_canvas_user_data(w, out_user_data)` — escreve o user_data (NULL se unset); 0 / -1 erro.
  - `int capy_widget_canvas_draw(w, dl)` — invoca o callback com `dl` + user_data; 0 se rodou e retornou 0; -1 em NULL/tipo/`dl` NULL/sem callback/retorno não-zero.
- 8 testes (`test_canvas_*`) em `tests/test_widget_contracts.c`. Suíte 329 → **337**.

### Compatibilidade

- 100% aditivo. Callback e user_data caller-owned: CapyUI nunca aloca/invoca em emit (determinismo preservado; o host controla a invocação). `capy_widget_serialize` (1.10) não serializa o callback. DL schema permanece `7`. **Trilha de estado dos advanced widgets COMPLETA (8/8 famílias: tree/table/rich-text/canvas/chart/color/date/autocomplete).**

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 337 testes + version-check) e `make package`.

## [2.20.0] — 2026-06-02

**Marco:** sétima fatia da trilha de **estado dos advanced widgets** (após date 2.14, color 2.15, table 2.16, autocomplete 2.17, tree 2.18, chart 2.19). Levanta o `RICH_TEXT`: array caller-owned de runs estilizados (`capy_text_range`) + a **primeira busca por offset** (`rich_text_range_at` acha o run que cobre um caractere; em overlap, o último run vence). Aditivo sobre 2.19; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/20/0`. `CAPYUI_API_VERSION_TAG = 0x00021400`.
- Style flags bitmask `CAPY_TEXT_STYLE_NONE/BOLD/ITALIC/UNDERLINE/STRIKETHROUGH` + `struct capy_text_range` (`start`/`length`/`flags`/`reserved`/`color` 0xAARRGGBB).
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_RICH_TEXT`): `const struct capy_text_range *rich_text_ranges` (caller-owned; NULL = sem estilo) + `uint16_t rich_text_range_count`. `sizeof` 880 → 896.
- 5 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_set_rich_text_ranges(w, ranges, count)` — `count==0` limpa; `count>0` exige `ranges!=NULL`; fail-closed.
  - `int capy_widget_clear_rich_text_ranges(w)` — volta a NULL/0.
  - `int capy_widget_rich_text_range_count(w)` — nº de runs (0 quando nenhum); `-1` em NULL/tipo.
  - `int capy_widget_rich_text_range(w, index, out_range)` — bounds-checked; `-1` em `index >= count` / sem dados / NULL.
  - `int capy_widget_rich_text_range_at(w, offset, out_range)` — run que cobre `offset` (`[start, start+length)`, zero-length cobre nada; overflow-safe); overlap = último run vence; 1 achou / 0 nenhum (out zerado) / -1 erro.
- 8 testes (`test_rich_text_*`) em `tests/test_widget_contracts.c`. Suíte 321 → **329**.

### Compatibilidade

- 100% aditivo. Array caller-owned: CapyUI nunca copia/aloca/libera (zero-alloc). `capy_widget_serialize` (1.10) não serializa os runs (estilo é caller-owned). DL schema permanece `7`.

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 329 testes + version-check) e `make package`.

## [2.19.0] — 2026-05-29

**Marco:** sexta fatia da trilha de **estado dos advanced widgets** (após date 2.14, color 2.15, table 2.16, autocomplete 2.17, tree 2.18). Levanta o `CHART`: array caller-owned de pontos `int32` + a **primeira redução numérica** (`chart_range` faz scan single-pass do min/max assinado). Aditivo sobre 2.18; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/19/0`. `CAPYUI_API_VERSION_TAG = 0x00021300`.
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_CHART`): `const int32_t *chart_values` (caller-owned; NULL = sem dados) + `uint16_t chart_count`. `sizeof` 864 → 880.
- 5 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_set_chart_data(w, values, count)` — `count==0` limpa; `count>0` exige `values!=NULL`; fail-closed.
  - `int capy_widget_clear_chart_data(w)` — volta a NULL/0.
  - `int capy_widget_chart_count(w)` — nº de pontos (0 quando nenhum); `-1` em NULL/tipo.
  - `int capy_widget_chart_value(w, index, out_value)` — bounds-checked; `-1` em `index >= count` / sem dados / NULL.
  - `int capy_widget_chart_range(w, out_min, out_max)` — min/max assinado single-pass; 1 com dados / 0 vazio (out zerado) / -1 erro.
- 8 testes (`test_chart_*`) em `tests/test_widget_contracts.c`. Suíte 313 → **321**.

### Compatibilidade

- 100% aditivo. Array caller-owned: CapyUI nunca copia/aloca/libera (zero-alloc). `capy_widget_serialize` (1.10) não serializa o dataset. DL schema permanece `7`.

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 321 testes + version-check) e `make package`.

## [2.18.0] — 2026-05-29

**Marco:** quinta fatia da trilha de **estado dos advanced widgets** (após date picker 2.14, color picker 2.15, table columns 2.16, autocomplete 2.17). Levanta o `TREE`: estado de fold/indent por nó + a **primeira leitura que caminha a hierarquia** (`tree_row_visible` deriva visibilidade dos ancestrais `TREE`). Aditivo sobre 2.17; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/18/0`. `CAPYUI_API_VERSION_TAG = 0x00021200`.
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_TREE`): `uint8_t tree_collapsed` (0 = expandido, default) + `uint16_t tree_depth` (indent). `sizeof` 856 → 864.
- 5 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_set_tree_collapsed(w, collapsed)` — qualquer não-zero normaliza p/ 1; `-1` em NULL/tipo.
  - `int capy_widget_tree_is_collapsed(w)` — 1 collapsed / 0 expandido / -1 erro.
  - `int capy_widget_set_tree_depth(w, depth)` — seta indent; `-1` em NULL/tipo.
  - `int capy_widget_tree_depth(w)` — indent (0 na raiz) / -1 erro.
  - `int capy_widget_tree_row_visible(w)` — caminha os pais; 0 se algum ancestral `TREE` está collapsed (só ancestrais escondem); 1 caso contrário; -1 erro.
- 8 testes (`test_tree_*`) em `tests/test_widget_contracts.c`. Suíte 305 → **313**.

### Compatibilidade

- 100% aditivo. Estado de fold/depth é UI efêmero: `capy_widget_serialize` (1.10) não serializa. DL schema permanece `7`.

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 313 testes + version-check) e `make package`.

## [2.17.0] — 2026-05-29

**Marco:** quarta fatia da trilha de **estado dos advanced widgets** (após date picker 2.14, color picker 2.15, table columns 2.16). Levanta o `AUTOCOMPLETE`: como a table guarda um **modelo caller-owned** (lista de strings), mas adiciona um **cursor de seleção mutável** com clamp/fail-closed. Aditivo sobre 2.16; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/17/0`. `CAPYUI_API_VERSION_TAG = 0x00021100`.
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_AUTOCOMPLETE`): `const char *const *autocomplete_items` (caller-owned; NULL = sem lista) + `uint16_t autocomplete_count` + `int32_t autocomplete_selected` (-1 = nenhum). `sizeof` 840 → 856.
- 6 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_set_autocomplete(w, items, count)` — `count==0` limpa; `count>0` exige `items!=NULL`; reseta seleção p/ -1; fail-closed.
  - `int capy_widget_clear_autocomplete(w)` — volta a NULL/0 e seleção -1.
  - `int capy_widget_autocomplete_count(w)` — nº de sugestões (0 quando nenhuma); `-1` em NULL/tipo.
  - `int capy_widget_autocomplete_item(w, index, out_item)` — bounds-checked; `-1` em `index >= count` / sem lista / NULL.
  - `int capy_widget_set_autocomplete_selected(w, index)` — `-1` limpa; `0..count-1` seleciona; `-1` em `index < -1` / `index >= count`.
  - `int capy_widget_get_autocomplete_selected(w, out_index)` — seleção viva clampada; `1` selecionado / `0` nenhum / `-1` erro.
- 8 testes (`test_autocomplete_*`) em `tests/test_widget_contracts.c`. Suíte 297 → **305**.

### Compatibilidade

- 100% aditivo. Lista de strings + índice caller-owned/efêmeros: CapyUI nunca copia/aloca/libera (zero-alloc). `capy_widget_serialize` (1.10) não serializa lista nem seleção. DL schema permanece `7`.

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 305 testes + version-check) e `make package`.

## [2.16.0] — 2026-05-29

**Marco:** terceira fatia da trilha de **estado dos advanced widgets** (após date picker 2.14, color picker 2.15). Levanta o `TABLE` para campos de cauda; diferente das duas fatias escalares, guarda um **array de larguras caller-owned** + contagem, com acessor **bounds-checked / fail-closed**. Aditivo sobre 2.15; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/16/0`. `CAPYUI_API_VERSION_TAG = 0x00021000`.
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_TABLE`): `const uint16_t *table_column_widths` (caller-owned; NULL = sem modelo) + `uint16_t table_column_count`. `sizeof` 824 → 840 (o ponteiro força alinhamento de 8).
- 4 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_set_table_columns(w, widths, count)` — `count==0` limpa; `count>0` exige `widths!=NULL`; fail-closed (modelo intacto em falha).
  - `int capy_widget_clear_table_columns(w)` — volta a NULL/0.
  - `int capy_widget_table_column_count(w)` — nº de colunas (0 quando nenhuma); `-1` em NULL/tipo.
  - `int capy_widget_table_column_width(w, index, out_width)` — bounds-checked; `-1` em `index >= count` / sem modelo / NULL.
- 8 testes (`test_table_*`) em `tests/test_widget_contracts.c`. Suíte 289 → **297**.

### Compatibilidade

- 100% aditivo. Array caller-owned: CapyUI nunca copia/aloca/libera (zero-alloc). `capy_widget_serialize` (1.10) não serializa o modelo de coluna. DL schema permanece `7`.

### Validação (externa, não executada nesta máquina)

- Recomendado: `make validate` (lint + security + check-decoupling + 297 testes + version-check) e `make package`.

## [2.15.0] — 2026-05-29

**Marco:** segunda fatia da trilha de **estado dos advanced widgets** (após o date picker em 2.14). Levanta o `COLOR_PICKER` de `user_data` para campos de cauda de primeira classe. Aditivo sobre 2.14; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/15/0`. `CAPYUI_API_VERSION_TAG = 0x00020F00`.
- Tail fields em `struct capy_widget` (válidos só p/ `CAPY_WIDGET_COLOR_PICKER`): `uint32_t picker_color` (0xAARRGGBB) + `uint8_t picker_color_set` (flag de presença). `sizeof` 816 → 824.
- 4 APIs em `src/widget/capy_widget.c`:
  - `uint32_t capy_color_pack(uint8_t r, uint8_t g, uint8_t b, uint8_t a)` — 0xAARRGGBB, cast-before-shift (sem UB no byte de alpha).
  - `int capy_widget_set_color(w, argb)` — armazena + marca set; `-1` fail-closed em NULL/tipo.
  - `int capy_widget_clear_color(w)` — volta ao unset (cor 0, flag 0).
  - `int capy_widget_get_color(w, out)` — `1` set / `0` unset (escreve 0) / `-1` NULL/tipo/out NULL.
- 8 testes novos em `tests/test_widget_contracts.c` (`test_color_*`). Total 281 → **289** (pinado em `make version-check`).

### ABI

- `capy-ui-widget` `2.14` → **`2.15`** (aditivo; sem remoções/renomeações).
- `capy-ui-desktop-session` permanece `1`. Display-list schema permanece `7`.

## [2.14.0] — 2026-05-29

**Marco:** primeira fatia de **estado dos advanced widgets**. Os 8 advanced widgets de 2.1 eram enum-only; esta release levanta o `DATE_PICKER` de `user_data` para um campo de cauda de primeira classe, estabelecendo o padrão aditivo das próximas famílias (TABLE/TREE/COLOR_PICKER/...). Aditivo sobre 2.13; DL schema permanece `7`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/14/0`. `CAPYUI_API_VERSION_TAG = 0x00020E00`.
- `struct capy_date { uint16_t year; uint8_t month; uint8_t day; }` em `capy_widget.h` (0 em qualquer campo = unset).
- Tail field `struct capy_date date_value;` em `struct capy_widget` (válido só quando `type == CAPY_WIDGET_DATE_PICKER`; `sizeof` +4B).
- 4 APIs em `src/widget/capy_widget.c`:
  - `int capy_date_is_valid(year, month, day)` — predicado de calendário Gregoriano proléptico (bissexto `4/100/400`).
  - `int capy_widget_set_date(w, year, month, day)` — valida + armazena; `-1` fail-closed em NULL/tipo/data inválida (valor anterior intacto).
  - `int capy_widget_clear_date(w)` — reseta para unset.
  - `int capy_widget_get_date(w, out)` — `1` set válido / `0` unset / `-1` NULL/tipo/out NULL.
- 8 testes novos em `tests/test_widget_contracts.c` (`test_date_*`). Total 273 → **281** (pinado em `make version-check`).

### Infra / higiene (não-ABI)

- `make check-decoupling` (agora em `make validate`): garante que `src/widget` não inclua headers CapyOS — gate documentado antes só rodado à mão.
- `make lint-desktop-session CAPYOS_INCLUDE=...`: gate externo best-effort para o desktop-session (gate canônico continua `CapyOS make all64 PROFILE=full`).
- `tools/abi_guard.sh` + `.github/workflows/abi-guard.yml`: implementa o guard de remoção de símbolos exigido pela `deprecation-policy.md`.
- Novo doc `docs/roadmap/contracts/desktop-session-coupling.md` + ADR-0006 / ADR-0007 em `tracking/DECISIONS.md`.
- `README.md` corrige o pin CapyOS para `0.8.0-alpha.260+20260525` (alinha com `docs/compatibility.md`).
- `docs/roadmap/long-term/v3.0-vision-platform.md` corrige "DL schema 8 → 9" para "7 → 9" e a dependência obsoleta.

### ABI

- `capy-ui-widget` `2.13` → **`2.14`** (aditivo; sem remoções/renomeações).
- `capy-ui-desktop-session` permanece `1`.
- Display-list schema permanece `7`.

# [2.13.1] — 2026-05-23

**Marco:** hotfix de packaging e alinhamento do release corrente após `2.13.0`. A ABI `capy-ui-widget` continua em `2.13`; este corte existe para publicar os novos assets e manter os URLs tag-pinned usados pelo CapyOS.

### Mudado

- `VERSION` 2.13.0 → 2.13.1.
- `README.md` e `Makefile` agora apontam para `v2.13.1`.
- `docs/roadmap/STATUS.md` e `docs/roadmap/tracking/ABSOLUTE.md` passam a mostrar `2.13.1` como pacote corrente.
- `README.md` atualiza o pin CapyOS para `0.8.0-alpha.257+20260523`.

### ABI

- `capy-ui-widget` permanece `2.13`.
- `capy-ui-desktop-session` permanece `1`.
- Display-list schema permanece `7`.

## [2.13.0] — 2026-05-21

**Marco:** **décimo quarto e último minor da fase 2.x — FASE 2.x COMPLETA (14/14)**. Login screen plumbing minimalista entregue: 12 macros (taxonomia layout + thresholds + power) + 1 API determinística (`capy_login_choose_layout`). **Biometric/SSO, PIN keypad, locked-badge animation e wake-screen deferidos** para 3.x.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/13/0`. `CAPYUI_API_VERSION_TAG = 0x00020D00`.
- 12 macros novas em `capy_widget.h`:
  - 4 layout: `CAPY_LOGIN_LAYOUT_SINGLE` (=0), `_GRID` (=1), `_LIST` (=2), `_COUNT` (=3).
  - 2 thresholds: `CAPY_LOGIN_GRID_THRESHOLD` (=2), `CAPY_LOGIN_LIST_THRESHOLD` (=7).
  - 7 power: `CAPY_LOGIN_POWER_NONE/SHUTDOWN/REBOOT/SLEEP/LOCK/LOGOUT/COUNT` (= 0..6).
- 1 API em `src/widget/capy_widget.c`:
  - `uint8_t capy_login_choose_layout(uint32_t user_count)` — `0|1→SINGLE`, `2..6→GRID`, `≥7→LIST`. Total function, zero-alloc, zero-float.
- 6 novos testes em `tests/test_widget_contracts.c`:
  1. `login_layout_zero_or_one` — 0 e 1 → SINGLE.
  2. `login_layout_grid_range` — 2..6 → GRID.
  3. `login_layout_list_threshold` — 6→GRID, 7→LIST, UINT32_MAX→LIST.
  4. `login_layout_deterministic` — 32× chamadas → mesma saída.
  5. `login_layout_taxonomy` — valores estáveis.
  6. `login_power_taxonomy` — valores estáveis.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 12 → 13. Tag `0x00020D00`.
- `Makefile` banner para `2.13.0 (alpha.276) — ABI 2.x (login screen aditivo, fim da fase 2.x)`.
- `VERSION` 2.12.0 → 2.13.0; `README.md` Version banner + URLs → `v2.13.0`.

### ABI

- `capy-ui-widget` **2.12 → 2.13** (aditivo: 12 macros + 1 API).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Total function.** `capy_login_choose_layout` nunca falha — todo `uint32_t` mapeia a um layout válido.
- **Threshold em 7.** Acima de 6 avatares, grid 2×3 fica denso; lista + search escala melhor.
- **Single cobre 0 e 1.** Zero usuários → first-boot prompt; um → form direto.
- **Power taxonomy estável.** Hosts ocultam subsets por policy (kiosk esconde SHUTDOWN/REBOOT) sem alterar a enumeração.
- **Mesma math em 3 surfaces.** Login screen + lock screen + user switcher compartilham `capy_login_choose_layout`.
- **Compõe com toda a stack 2.x:** v2.7 user_directory para auth, v2.11 icon_resolve para avatares, v2.12 wallpaper para background, v2.13 layout chooser para grid.

### Marco — fase 2.x COMPLETA

Esta é a **última fatia do major 2.x**. Entregaram-se 14/14 minors:
- v2.0 plugin system + SDK
- v2.1 advanced widgets
- v2.2 virtualization plumbing
- v2.3 undo/redo
- v2.4 theme packs
- v2.5 devtools/inspector
- v2.6 display mode
- v2.7 user management
- v2.8 contrast/WCAG refinement
- v2.9 desktop icon arrangement
- v2.10 file manager UX
- v2.11 icon & thumbnail system
- v2.12 wallpaper management
- **v2.13 login screen**

Próximo marco: **fase 3.x (Vision platform)** — quebra controlada via major bump 3.0 (DL schema 9, sketch surface, GPU integration, cross-device handoff). 1.x continua em LTS ≥12m.

### Docs

- `docs/compatibility.md` ganhou bloco "Login screen (since 2.13)"; ABI line bumpada para `v2.13`.
- `docs/roadmap/long-term/v2.13-login-screen.md` status = delivered; **marca fim da fase 2.x**.
- `docs/roadmap/contracts/abi-versions.md` linha 2.13 promovida para **atual**; 2.12 demovida para tagged; tag `v2.13.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.13.0 / `capy-ui-widget 2.13` / DL schema 7 / **272 testes** / **fase 2.x COMPLETA 14/14**.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.13** + nota de fechamento de fase.

## [2.12.0] — 2026-05-21

**Marco:** décimo terceiro minor da fase 2.x. **Wallpaper management plumbing** entregue: blob canonical LE `CWLP` (header 24B + slides 8B) + 2 structs + 3 APIs (validate/load/serialize) + 13 macros. **Live wallpaper, per-locale defaults, transition effects e image format negotiation deferidos**.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/12/0`. `CAPYUI_API_VERSION_TAG = 0x00020C00`.
- 13 macros novas em `capy_widget.h`:
  - `CAPY_WALLPAPER_MAGIC0..3` = `'C','W','L','P'`.
  - `CAPY_WALLPAPER_FORMAT_VERSION` (=1), `_HEADER_SIZE` (=24), `_ENTRY_SIZE` (=8).
  - `CAPY_WALLPAPER_FIT_STRETCH/FILL/FIT/CENTER/TILE` + `_COUNT` (=5).
  - `CAPY_WALLPAPER_FLAG_RANDOM` (=0x01), `_PER_MONITOR` (=0x02).
- 2 structs em `capy_widget.h`:
  - `capy_wallpaper_config { default_image_id, slideshow_interval_sec, monitor_index, fit_mode, flags, reserved }`.
  - `capy_wallpaper_slide { image_id, duration_sec, reserved }`.
- 3 APIs em `src/widget/capy_widget.c`:
  - `capy_wallpaper_validate(buf, len)` — magic + version + reserved + fit_mode + length + checksum + per-slide guards.
  - `capy_wallpaper_load(buf, len, *out_config, *out_slides, cap, *out_count)` — short-buffer detection via `*out_count > cap`.
  - `capy_wallpaper_serialize(*config, *slides, count, *out, cap)` — pre-flight + byte-determinístico.
- Reusa helpers LE + FNV-1a do `CDLA` (v2.9) — format consistency com `CUIS`/`CTHM`/`CDLA`/`CWLP`.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `wallpaper_roundtrip` — config + 3 slides preservados.
  2. `wallpaper_no_slides_roundtrip` — count=0 → 24-byte header.
  3. `wallpaper_invalid_fit_rejected` — `fit_mode >= COUNT` → -1.
  4. `wallpaper_slide_zero_id_rejected` — `image_id=0` + `duration=0` rejeitados.
  5. `wallpaper_bad_magic_and_checksum_rejected`.
  6. `wallpaper_short_buffer_reports_count` — cap=2 < 6 declared.
  7. `wallpaper_null_guards`.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 11 → 12. Tag `0x00020C00`.
- `Makefile` banner para `2.12.0 (alpha.275) — ABI 2.x (wallpaper management aditivo)`.
- `VERSION` 2.11.0 → 2.12.0; `README.md` Version banner + URLs → `v2.12.0`.

### ABI

- `capy-ui-widget` **2.11 → 2.12** (aditivo: 13 macros + 2 structs + 3 APIs).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing-only.** Widget core nunca compõe pixels, nunca agenda timer, nunca lê arquivo.
- **Helpers reused.** Byte readers/writers LE + FNV-1a vêm do bloco `CDLA` (v2.9) — uma única implementação para todos os 4 blobs canônicos (`CUIS`, `CTHM`, `CDLA`, `CWLP`).
- **Per-slide guards.** `image_id=0` reservado (silhouette default v2.11). `duration_sec=0` rejeitado (slides instantâneos não fazem sentido).
- **`monitor_index`.** `0 = global`, `≥ 1 = per-monitor` (compõe com v2.6 display mode).
- **Compõe com v2.11 icon system** (image_id consumido pelo provider) e v2.7 user management (envelope per-uid no host).

### Docs

- `docs/compatibility.md` ganhou bloco "Wallpaper management (since 2.12)"; ABI line bumpada para `v2.12`.
- `docs/roadmap/long-term/v2.12-wallpaper.md` status = delivered; widget-core escopo documentado.
- `docs/roadmap/contracts/abi-versions.md` linha 2.12 promovida para **atual**; 2.11 demovida para tagged; tag `v2.12.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.12.0 / `capy-ui-widget 2.12` / DL schema 7 / **266 testes** / fase 2.x com 13 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.12**.

## [2.11.0] — 2026-05-21

**Marco:** décimo segundo minor da fase 2.x. **Icon & thumbnail system plumbing** entregue: 1 struct + 2 typedefs + 1 tail field em context + 4 APIs + 1 event type aditivo. **Icon theme cascade, vector icons, cancellation e group folder stacks deferidos** para minors futuros.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/11/0`. `CAPYUI_API_VERSION_TAG = 0x00020B00`.
- 1 event type aditivo: `CAPY_WIDGET_EVENT_THUMBNAIL_READY` (appended ao tail do enum). Payload `event->x = (int32_t)request_id`, `event->y = (int32_t)image_id`.
- 2 typedefs em `capy_widget.h`:
  - `capy_icon_resolve_fn(user_data, mime_type, extension, *out_icon_id) → int`.
  - `capy_icon_thumbnail_request_fn(user_data, path, *out_request_id) → int`.
- 1 struct: `capy_icon_provider { resolve, thumbnail_request, user_data }` (caller-owned).
- Tail field aditivo `icon_provider` em `capy_widget_context` (NULL = host-less).
- 4 APIs em `src/widget/capy_widget.c`:
  - `capy_widget_set_icon_provider(ctx, provider)` — attach/clear; NULL ctx no-op.
  - `capy_icon_resolve(ctx, mime, ext, *out_id)` — fallback determinístico `image_id=0`.
  - `capy_icon_thumbnail_request(ctx, path, *out_request_id)` — host devolve id ≥ 1; -1 em failure/0.
  - `capy_icon_thumbnail_ready(ctx, root, request_id, image_id)` — emite `THUMBNAIL_READY` em root.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `icon_resolve_forwards_to_host` — mime+ext → image_id=42.
  2. `icon_resolve_fallback_no_provider` — sem provider → image_id=0.
  3. `icon_resolve_fallback_host_failure` — host -1 → fallback 0.
  4. `icon_thumbnail_request_forwards` — host atribui request_id incrementando.
  5. `icon_thumbnail_request_failure_modes` — host -1 + host devolve 0 → -1.
  6. `icon_thumbnail_ready_rejects_id_zero` — request_id=0 rejeitado.
  7. `icon_no_provider_thumbnail_request_returns_negative` — sem provider → -1.
  8. `icon_null_guards` — todas as combinações NULL.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 10 → 11. Tag `0x00020B00`.
- `Makefile` banner para `2.11.0 (alpha.274) — ABI 2.x (icon & thumbnail system aditivo)`.
- `VERSION` 2.10.0 → 2.11.0; `README.md` Version banner + URLs → `v2.11.0`.

### ABI

- `capy-ui-widget` **2.10 → 2.11** (aditivo: 2 typedefs + 1 struct + 1 tail field em ctx + 4 APIs + 1 event type).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing-only.** Widget core nunca decode bytes, nunca toca disco.
- **Fallback determinístico `image_id=0`.** Reservado para silhouette default.
- **`request_id=0` reservado.** Hosts não devolvem; widget core rejeita em `_thumbnail_ready`.
- **Stateless.** Widget core não rastreia request_id em flight — host responsável por cancelamento/timeout.
- **Event routing.** Dispatcher do CapyOS lê `event->x = request_id` para rotear ao widget que pediu.
- **Compõe com v2.10 file mgr + v2.9 desktop layout + v1.5 DPI.** File manager usa `icon_resolve` + `thumbnail_request`; desktop usa `icon_resolve` para ícones; multi-DPI atende via `image_id` consumido pelo backend.

### Docs

- `docs/compatibility.md` ganhou bloco "Icon & thumbnail system (since 2.11)"; ABI line bumpada para `v2.11`.
- `docs/roadmap/long-term/v2.11-icons-thumbnails.md` status = delivered; widget-core escopo documentado.
- `docs/roadmap/contracts/abi-versions.md` linha 2.11 promovida para **atual**; 2.10 demovida para tagged; tag `v2.11.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.11.0 / `capy-ui-widget 2.11` / DL schema 7 / **259 testes** / fase 2.x com 12 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.11**.

## [2.10.0] — 2026-05-21

**Marco:** décimo primeiro minor da fase 2.x. **File manager UX plumbing** entregue: 1 struct + 1 API determinística + 6 macros (taxonomia toolbar + threshold compact). Math zero-float, zero-alloc. **Variable-width measurement, multi-line breadcrumb, locale-aware ellipsis e per-item descriptors deferidos** para minors futuros e para o desktop session.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/10/0`. `CAPYUI_API_VERSION_TAG = 0x00020A00`.
- 6 macros novas em `capy_widget.h`:
  - `CAPY_FILE_MGR_COMPACT_THRESHOLD_PX` (=600) — viewport breakpoint compact toolbar.
  - `CAPY_TOOLBAR_GROUP_NAV` (=0), `_VIEW` (=1), `_ACTION` (=2), `_LAYOUT` (=3), `_COUNT` (=4) — taxonomy estável.
- 1 struct em `capy_widget.h`: `capy_breadcrumb_segment { id, text_offset, text_len, icon_image_id, is_clickable, reserved }`.
- 1 API em `src/widget/capy_widget.c`:
  - `capy_breadcrumb_truncate(in, in_count, available_width_px, segment_avg_px, out, cap, *out_count, *out_dropdown)` — root+tail com dropdown; floor em 2; pre-flight cap check em full-fit; determinístico zero-float.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `breadcrumb_full_fit_no_dropdown` — 4×80=320 ≤ 800: todos inline.
  2. `breadcrumb_truncated_root_plus_tail` — 8×100>300: root + 2 tail.
  3. `breadcrumb_min_two_with_narrow_width` — floor em 2.
  4. `breadcrumb_empty_input`.
  5. `breadcrumb_single_segment`.
  6. `breadcrumb_null_guards`.
  7. `breadcrumb_cap_too_small_for_full_fit`.
  8. `toolbar_group_taxonomy` — valores estáveis + threshold.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 9 → 10. Tag `0x00020A00`.
- `Makefile` banner para `2.10.0 (alpha.273) — ABI 2.x (file manager UX plumbing aditivo)`.
- `VERSION` 2.9.0 → 2.10.0; `README.md` Version banner + URLs → `v2.10.0`.

### ABI

- `capy-ui-widget` **2.9 → 2.10** (aditivo: 6 macros + 1 struct + 1 API).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing-only.** Widget core não dirige o file-manager; publica math + taxonomy.
- **Determinístico.** Mesmo `(in, in_count, available_width, segment_avg, cap)` → mesma saída byte-a-byte.
- **Floor em 2 segmentos.** Quando budget < 2×avg, garantimos sempre root + current (UX coerente em telas estreitas).
- **Pre-flight cap check.** Em full-fit, `cap < in_count` retorna -1 sem partial writes.
- **Reserved em segment.** Forward-compat para flags futuras (icon-only, sticky-root).
- **Compõe com v2.1 advanced widgets (TABLE/TREE) + v2.9 desktop layout.** File manager app no `src/apps/` usa essa math + as advanced widget types.

### Docs

- `docs/compatibility.md` ganhou bloco "File manager UX plumbing (since 2.10)"; ABI line bumpada para `v2.10`.
- `docs/roadmap/long-term/v2.10-file-manager-polish.md` status = delivered; widget-core escopo documentado.
- `docs/roadmap/contracts/abi-versions.md` linha 2.10 promovida para **atual**; 2.9 demovida para tagged; tag `v2.10.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.10.0 / `capy-ui-widget 2.10` / DL schema 7 / **251 testes** / fase 2.x com 11 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.10**.

## [2.9.0] — 2026-05-21

**Marco:** décimo minor da fase 2.x. **Desktop icon arrangement plumbing** entregue: blob canonical LE `CDLA` + 2 structs + 3 APIs (validate/load/serialize) + 14 macros. **Multi-monitor envelope, group folders, locale-aware sort e drag hit-test in core deferidos** para minors futuros e para o desktop session.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/9/0`. `CAPYUI_API_VERSION_TAG = 0x00020900`.
- 14 macros novas em `capy_widget.h`:
  - `CAPY_DESKTOP_LAYOUT_MAGIC0..3` = `'C','D','L','A'`.
  - `CAPY_DESKTOP_LAYOUT_FORMAT_VERSION` (=1), `_HEADER_SIZE` (=24), `_ENTRY_SIZE` (=16).
  - `CAPY_DESKTOP_SORT_MANUAL/NAME/TYPE/MTIME/SIZE` + `_COUNT` (=5).
  - `CAPY_DESKTOP_LAYOUT_FLAG_ALIGN_RIGHT` (=0x01), `_VERTICAL_FIRST` (=0x02).
- 2 structs em `capy_widget.h`:
  - `capy_desktop_layout { cell_w, cell_h, snap, auto_arrange, sort_by, flags }`.
  - `capy_desktop_icon_position { icon_id, x, y, grid_x, grid_y, pinned, reserved[3] }`.
- 3 APIs em `src/widget/capy_widget.c`:
  - `capy_desktop_layout_validate(buf, len)` — magic + version + reserved + length + checksum + cell>0 + snap/auto≤1 + sort<COUNT.
  - `capy_desktop_layout_load(buf, len, *out_layout, *out_positions, cap, *out_count)` — valida + escreve config + copia entries até `cap`. Short-buffer detection via `*out_count > cap`.
  - `capy_desktop_layout_serialize(*layout, *positions, count, *out, cap)` — escreve blob canônico byte-determinístico.
- 6 helpers estáticos: `capy_desktop_layout_fnv1a`, `_read_u16`/`_read_i16`/`_read_u32`, `_write_u16`/`_write_i16`/`_write_u32`.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `desktop_layout_roundtrip` — 5 entries serialize → validate → load preserva campos.
  2. `desktop_layout_empty_roundtrip` — count=0 → 24 bytes.
  3. `desktop_layout_bad_magic_rejected`.
  4. `desktop_layout_checksum_mismatch_rejected`.
  5. `desktop_layout_zero_cell_rejected`.
  6. `desktop_layout_invalid_sort_rejected`.
  7. `desktop_layout_short_buffer_reports_count` — cap=3 < 8 declared: count==8.
  8. `desktop_layout_null_guards`.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 6 → 9 (resync: bumps anteriores de 7/8 não tinham aplicado no header; outras facetas dos minors 2.7/2.8 já estavam corretas).
- `Makefile` banner para `2.9.0 (alpha.272) — ABI 2.x (desktop icon arrangement aditivo)`.
- `VERSION` 2.8.0 → 2.9.0; `README.md` Version banner + URLs → `v2.9.0`.

### ABI

- `capy-ui-widget` **2.8 → 2.9** (aditivo: 14 macros + 2 structs + 3 APIs).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing-only.** Widget core nunca abre arquivo, nunca dirige drag, nunca decide posição. Apenas round-trip de bytes.
- **FNV-1a sobre body.** Mesma função que `CUIS` (v1.10), `CTHM` (v2.4) — audit comum.
- **Reserved fail-closed.** Header `reserved[4]` em offset 20 + per-entry `reserved[3]` em offset 13 devem ser zero. Forward-compat fechado por enquanto.
- **Short-buffer semantics.** `*out_count` sempre carrega declared count, mesmo quando `cap < count`. Hosts sizing o buffer com 1 chamada (`cap=0` → declared count + buf untouched além do header).
- **Pinned bit.** `pinned > 1u` rejeitado em load/serialize — defensive.
- **Compõe com v2.6 display mode + v2.11 icon system.** Multi-monitor envelope futura usa `monitor_index` do display controller; group folders ficam no descriptor v2.11.

### Docs

- `docs/compatibility.md` ganhou bloco "Desktop icon arrangement (since 2.9)"; ABI line bumpada para `v2.9`.
- `docs/roadmap/long-term/v2.9-desktop-icon-arrange.md` status = delivered; widget-core escopo documentado; deferidos listados.
- `docs/roadmap/contracts/abi-versions.md` linha 2.9 promovida para **atual**; 2.8 demovida para tagged; tag `v2.9.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.9.0 / `capy-ui-widget 2.9` / DL schema 7 / **243 testes** / fase 2.x com 10 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.9**.

## [2.8.0] — 2026-05-21

**Marco:** nono minor da fase 2.x. **Contrast & accessibility refinement** entregue: 3 APIs de medição WCAG-style + 3 macros + struct de finding + 4ª factory built-in (`default_dark_high_contrast`). Cálculo zero-float γ≈2.0 determinístico. **WCAG 3 APCA, large-text threshold (3.0), per-locale palettes e auto-fix suggestions deferidos** para minors futuros.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/8/0`. `CAPYUI_API_VERSION_TAG = 0x00020800`.
- 3 macros novas em `capy_widget.h`:
  - `CAPY_CONTRAST_AA_X1000` (=4500) — WCAG 2.1 AA threshold ×1000.
  - `CAPY_CONTRAST_AAA_X1000` (=7000) — WCAG 2.1 AAA threshold ×1000.
  - `CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST` (=3) — 4º variant byte.
- 1 struct `capy_contrast_finding { uint8_t fg_token; uint8_t bg_token; uint16_t ratio_x1000; uint8_t passes_aa; uint8_t passes_aaa; uint8_t reserved[2]; }`.
- 3 APIs em `src/widget/capy_widget.c`:
  - `uint32_t capy_theme_contrast_ratio_x1000(fg, bg)` — ratio ×1000 clamp `[1000, 21000]`. Alpha ignorado. Zero-float.
  - `int capy_theme_audit_wcag(theme, out, cap)` — walk 11 canonical pairs. `cap=0` dry-run retorna total; `-1` em NULL theme ou `cap>0 && out==NULL`.
  - `struct capy_theme capy_theme_default_dark_high_contrast(void)` — 4ª factory, todos pairs ≥ 7:1 (AAA).
- 4 helpers estáticos: `capy_contrast_srgb_linear_q16`, `capy_contrast_luminance_q16`, `capy_contrast_audit_pairs[]`, `CAPY_CONTRAST_PAIR_COUNT` macro.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `contrast_white_on_black_is_21000` — ratio máximo + simétrico.
  2. `contrast_same_color_is_1000` — ratio mínimo.
  3. `contrast_alpha_ignored` — RGB-only.
  4. `contrast_ratio_in_bounds` — mid-grey ∈ (1000, 21000).
  5. `audit_dry_run_returns_total` — `cap=0` returns 11.
  6. `audit_high_contrast_all_pass_aa` — light HC theme: AA across canonical pairs.
  7. `audit_dark_high_contrast_all_pass_aaa` — novo factory: todos pairs ≥ 7000.
  8. `audit_null_and_capacity_overflow` — NULL guards + cap < total.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 7 → 8.
- `Makefile` banner para `2.8.0 (alpha.271) — ABI 2.x (contrast/WCAG aditivo)`.
- `VERSION` 2.7.0 → 2.8.0; `README.md` Version banner + URLs → `v2.8.0`.

### ABI

- `capy-ui-widget` **2.7 → 2.8** (aditivo: 3 macros + 1 struct + 3 APIs novas + 1 nova factory built-in).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Zero-float.** Toda matemática inteira 64-bit. Cross-platform byte-determinístico.
- **γ≈2.0 approximation.** `lin = c * c * 65536 / 65025` em vez da fórmula WCAG-2.1 exata. Diferenças bounded; AA/AAA pass/fail concordam nos pares canônicos.
- **Weighted luminance.** `L = (2126*R + 7152*G + 722*B) / 10000` — coeficientes BT.709/sRGB × 10000 (mesmos do WCAG).
- **Offset 0.05.** Em Q16.16 = `3277`. Aplicado a ambos `L_max` e `L_min` no `(L_max + 3277)*1000 / (L_min + 3277)`.
- **Clamp `[1000, 21000]`.** Cobre o range WCAG 1.0–21.0 sem precisar de `uint32_t` largo no finding (cabe em `uint16_t`).
- **11 canonical pairs.** Estáticas no source (não data-driven) — auditável e estável.
- **Dry-run `cap=0`.** Hosts sizing o buffer em 1 chamada.
- **4ª factory.** `default_dark_high_contrast`: BG_BASE=#000000, FG_PRIMARY=#FFFFFF, ACCENT=#FFFF00, DANGER=#FF8080, SUCCESS=#80FF80, INFO=#80FFFF, etc. Todos pairs canônicos ≥ 7:1.
- **Compõe com v2.4 theme packs.** Hosts podem distribuir `.captheme` packs custom e validar via `audit_wcag` antes de aplicar.

### Docs

- `docs/compatibility.md` ganhou bloco "Contrast & accessibility refinement (since 2.8)"; ABI line bumpada para `v2.8`.
- `docs/roadmap/long-term/v2.8-contrast-refinement.md` status = delivered; deferidos documentados.
- `docs/roadmap/contracts/abi-versions.md` linha 2.8 promovida para **atual**; 2.7 demovida para tagged; tag `v2.8.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.8.0 / `capy-ui-widget 2.8` / DL schema 7 / **235 testes** / fase 2.x com 9 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.8**.

## [2.7.0] — 2026-05-21

**Marco:** oitavo minor da fase 2.x. **User management UI plumbing** entregue em escopo conservador: 5 callbacks no host + 6 APIs forwarding + 2 guards de safety (root undeletable, empty username rejeitado). **Authentication, groups/roles e audit log deferidos** para minors futuros e para o desktop session.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/7/0`. `CAPYUI_API_VERSION_TAG = 0x00020700`.
- 2 macros novas em `capy_widget.h`: `CAPY_USER_NAME_MAX` (=32), `CAPY_USER_DISPLAY_NAME_MAX` (=64).
- 1 struct `capy_user_account { char username[32]; char display_name[64]; uint32_t uid; uint8_t is_admin; uint8_t is_locked; uint16_t avatar_image_id; uint32_t last_login_ms; }`.
- 5 typedefs (callbacks no host): `capy_user_list_fn`, `capy_user_create_fn` (password verbatim), `capy_user_update_fn`, `capy_user_delete_fn`, `capy_user_set_avatar_fn`.
- 1 struct `capy_user_directory { list, create, update, del, set_avatar, user_data }` (caller-owned).
- Tail field aditivo `user_directory` em `capy_widget_context` (NULL = surface ausente).
- 6 APIs em `src/widget/capy_widget.c`:
  - `capy_widget_set_user_directory(ctx, dir)` — attach/clear; NULL ctx no-op.
  - `capy_user_list(ctx, out, cap)` — forwards; `-1` em NULL/missing callback ou `cap>0 && out==NULL`.
  - `capy_user_create(ctx, account, password)` — rejeita `username[0] == '\0'` antes do host; password forwardada verbatim.
  - `capy_user_update(ctx, account)` — forwards; host faz lookup por uid.
  - `capy_user_delete(ctx, uid)` — rejeita `uid == 0` (root) antes do host.
  - `capy_user_set_avatar(ctx, uid, png, len)` — rejeita `len > 0 && png == NULL`; `(NULL, 0)` clears.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `user_list_forwards_to_host` — roster de 2 contas (root + alice) enumerado.
  2. `user_create_appends_and_passes_password` — host recebe password verbatim ("s3cret"), conta added.
  3. `user_create_rejects_empty_username` — `username[0] == '\0'` → -1; host nunca chamado.
  4. `user_update_roundtrip` — `is_admin`/`is_locked` toggleadas via update.
  5. `user_delete_rejects_root_uid_zero` — uid=0 rejected; uid=1000 normal passa.
  6. `user_set_avatar_clear_and_set` — set+clear+invalid combinations.
  7. `user_no_directory_returns_negative` — sem directory, todas as 5 APIs retornam -1.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 6 → 7.
- `Makefile` banner para `2.7.0 (alpha.270) — ABI 2.x (user management aditivo)`.
- `VERSION` 2.6.0 → 2.7.0; `README.md` Version banner + URLs → `v2.7.0`.

### ABI

- `capy-ui-widget` **2.6 → 2.7** (aditivo: 2 macros + 1 struct account + 5 typedefs + 1 struct directory + 1 tail field em context + 6 APIs).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing-only.** CapyUI nunca autentica, nunca hashea, nunca toca `/etc/passwd`. Host CapyOS implementa via backend de autenticação real (PAM, libsodium, etc.).
- **Password verbatim.** `password` em `create` é forward direto ao host; widget core nunca armazena o ponteiro nem inspeciona os bytes.
- **Guard convencional `uid == 0`.** Root é undeletable pelo widget core. Host pode redefinir convenções via wrapper que mapeia outro uid para "system account".
- **Avatar bytes caller-owned.** `set_avatar` nunca copia bytes; host responsável por decodificar/persistir.
- **Surface compartilhada.** v2.13 login screen consome a mesma `user_directory` para listar avatares + autenticar.
- **Fail-closed em pre-flight.** NULL guards + empty username + uid==0 + `len>0 && png==NULL` → -1 antes do host.

### Docs

- `docs/compatibility.md` ganhou bloco "User management UI (since 2.7)"; ABI line bumpada para `v2.7`.
- `docs/roadmap/long-term/v2.7-user-management.md` status = delivered; ABI ↔ desktop session split documentado.
- `docs/roadmap/contracts/abi-versions.md` linha 2.7 promovida para **atual**; 2.6 demovida para tagged; tag `v2.7.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.7.0 / `capy-ui-widget 2.7` / DL schema 7 / **227 testes** / fase 2.x com 8 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.7**.

## [2.6.0] — 2026-05-21

**Marco:** sétimo minor da fase 2.x e primeira fatia da série UX polish v2.6–v2.13. **Display mode plumbing** entregue em escopo conservador: 2 callbacks no host + 4 APIs forwarding + 1 event aditivo. **Fractional refresh, HDR/VRR metadata, rollback/preview UI 15s e persistência per-user deferidos** para minors futuros e para o desktop session.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/6/0`. `CAPYUI_API_VERSION_TAG = 0x00020600`.
- 2 macros novas em `capy_widget.h`: `CAPY_DISPLAY_MODE_FLAG_PREFERRED` (0x01u), `CAPY_DISPLAY_MODE_FLAG_INTERLACED` (0x02u).
- 1 struct `capy_display_mode { uint16_t width, height, refresh_hz_q8; uint8_t bpp, flags; }`.
- 2 typedefs:
  - `capy_display_enum_modes_fn(user_data, out, cap) → int` — host enumera modos em buffer caller-owned.
  - `capy_display_set_mode_fn(user_data, *mode) → int` — host aplica mode (0 ok, -1 rejeitado).
- 1 struct `capy_display_controller { enum_modes, set_mode, user_data, current_mode, has_current, reserved[3] }` (caller-owned).
- Tail field aditivo `display_controller` em `capy_widget_context` (NULL = surface ausente).
- Event type aditivo `CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED` (appended ao tail do enum). Payload: `event->x = (width<<16)|height`, `event->y = (bpp<<16)|refresh_hz_q8`.
- 4 APIs em `src/widget/capy_widget.c`:
  - `capy_widget_set_display_controller(ctx, controller)` — attach/clear; NULL ctx no-op.
  - `capy_display_enum_modes(ctx, out, cap)` — forwards; `-1` em NULL ctx/missing callback ou `cap>0 && out==NULL`.
  - `capy_display_set_mode(ctx, root, mode)` — valida `width>0 && height>0`, forwards; em sucesso cacheia `current_mode`, set `has_current=1`, emite `DISPLAY_MODE_CHANGED` em `root` (se non-NULL).
  - `capy_display_current_mode(ctx, out)` — copia `current_mode` se `has_current`; `-1` caso contrário.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `display_enum_modes_forwards_to_host` — catalog de 3 modos enumerado; PREFERRED flag preservada.
  2. `display_set_mode_caches_and_emits_event` — set cacheia, `has_current=1`, current_mode legível.
  3. `display_set_mode_host_failure_no_cache` — host -1 → `has_current` permanece 0.
  4. `display_no_controller_returns_negative` — sem controller, todas as 3 APIs retornam -1.
  5. `display_set_mode_rejects_zero_dimensions` — width=0 ou height=0 → -1 antes do callback.
  6. `display_enum_modes_capacity_truncates` — cap < count → host trunca; CapyUI repassa.
  7. `display_null_guards` — NULL ctx em todas; setter NULL ctx é no-op.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 5 → 6.
- `Makefile` banner para `2.6.0 (alpha.269) — ABI 2.x (display mode aditivo)`.
- `VERSION` 2.5.0 → 2.6.0; `README.md` Version banner + URLs → `v2.6.0`.

### ABI

- `capy-ui-widget` **2.5 → 2.6** (aditivo: 2 macros + 1 struct mode + 2 typedefs + 1 struct controller + 1 tail field em context + 1 event type + 4 APIs).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing-only.** CapyUI nunca toca framebuffer, DRM/KMS, registers de video card. Tudo via callbacks no host CapyOS.
- **Cached current.** `capy_display_controller` carrega o último mode aplicado com sucesso para acesso O(1) sem ida no host.
- **Event payload packing.** `event->x = (width<<16)|height` e `event->y = (bpp<<16)|refresh_hz_q8` empacotam tudo no `capy_widget_event` existente sem precisar de novo tipo de struct.
- **Fail-closed.** NULL guards + missing callback + `width==0||height==0` + host -1 → todos retornam -1 sem mutação.
- **Determinístico.** Mesma sequência `(controller, mode)` → mesmo side effect observável + mesma event payload.
- **DL schema permanece 7.** Sem novas ops; o event é apenas no `capy_widget_event_type`.
- **Refresh em Q8.0** hoje. Promoção para Q24.8 (fractional refresh para 23.976 / 59.94 / 119.88 Hz / G-SYNC) deferida.
- **Compõe com v1.5** (DPI scaling): mode change atualiza o framebuffer físico; o host pode também ajustar `dpi_scale_x256` no contexto para corresponder à nova DPI.

### Docs

- `docs/compatibility.md` ganhou bloco "Display mode (since 2.6)"; ABI line bumpada para `v2.6`.
- `docs/roadmap/long-term/v2.6-display-mode.md` status = delivered; ABI ↔ desktop session split documentado; deferidos listados.
- `docs/roadmap/contracts/abi-versions.md` linha 2.6 promovida para **atual**; 2.5 demovida para tagged; tag `v2.6.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.6.0 / `capy-ui-widget 2.6` / DL schema 7 / **220 testes** / fase 2.x com 7 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.6**.

## [2.5.0] — 2026-05-21

**Marco:** sexto minor da fase 2.x. **Devtools/inspector** entregue em escopo read-only: tree flatten depth-first determinístico + snapshot de perf counters do contexto. **Dump textual de tree/DL, event recording/replay e headless harness CLI deferidos** para minors futuros.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/5/0`. `CAPYUI_API_VERSION_TAG = 0x00020500`.
- 12 macros publicadas em `capy_widget.h`:
  - `CAPY_INSPECTOR_NO_PARENT` = `0xFFFFFFFFu`.
  - `CAPY_INSPECTOR_FLAG_VISIBLE`, `_ENABLED`, `_FOCUSED`, `_FOCUSABLE`, `_CHECKED`, `_HOVERED`, `_LAYOUT_DIRTY`, `_HAS_TRANSFORM`, `_HAS_VIRTUAL_SRC`, `_HAS_DRAG_PAYLOAD`, `_HAS_DROP_TARGET`.
- 2 structs em `capy_widget.h`:
  - `struct capy_inspector_node` (id, type, depth, parent_index, child_count, x/y/w/h, flags, tab_index, font_id, text_offset/len, layout_version, dirty_version).
  - `struct capy_perf_counters` (widget_count, plugin_count, undo_count/redo_count/capacity, dpi_scale_x256, frame_budget_microseconds, theme_variant, theme_high_contrast).
- 2 APIs em `src/widget/capy_widget.c`:
  - `int capy_widget_inspect(root, out_nodes, cap, out_text_pool, text_cap, out_node_count, out_text_used)` — depth-first pre-order flatten que espelha `capy_widget_emit_recursive`.
  - `int capy_perf_counters_snapshot(ctx, root, out)` — snapshot fixed-size do contexto + recursive widget count.
- 3 helpers estáticos: `capy_inspector_flags_for`, `capy_inspector_visit` (recursivo), `capy_inspector_count_widgets`.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_inspector_root_only_returns_one_node` — single node, depth/parent/flags corretos.
  2. `test_inspector_depth_first_preorder` — tree (A(B(C),D)) → ordem [A,B,C,D], depths [0,1,2,1], parent_index corretos.
  3. `test_inspector_text_pool_copies_label_text` — bytes "hello" copiados sem NUL no offset 0.
  4. `test_inspector_capacity_overflow_fails_closed` — cap=2, 3 nodes → -1 + rollback `node_count`/`text_used` para 0.
  5. `test_inspector_text_pool_overflow_fails_closed` — text_cap=2 com "long-text" → -1 + rollback.
  6. `test_inspector_null_guards` — 5 NULL-arg patterns rejeitados.
  7. `test_perf_counters_snapshot_populates_fields` — widget_count/plugin/undo/dpi/frame_budget/theme; NULL ctx/out rejeitado; NULL root tolerado (widget_count=0).

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 4 → 5.
- `Makefile` banner para `2.5.0 (alpha.268) — ABI 2.x (devtools/inspector aditivo)`.
- `VERSION` 2.4.0 → 2.5.0; `README.md` Version banner + URLs → `v2.5.0`.

### ABI

- `capy-ui-widget` **2.4 → 2.5** (aditivo: 12 macros novas + 2 structs novas + 2 APIs novas).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Zero allocation.** Ambas as APIs escrevem em buffers caller-owned; nenhuma alocação interna no core.
- **Read-only.** Nenhuma mutação no widget tree, no contexto ou no theme.
- **Ordem determinística.** Tree flatten depth-first pre-order alinhado com `capy_widget_emit_recursive` — diff entre frames é estável.
- **Fail-closed.** NULL guards + capacity overflow + text pool overflow → `-1` com rollback de `*out_node_count` e `*out_text_used` para `0`.
- **Encoding.** Text bytes copiados sem NUL terminator; consumidor lê `(text_offset, text_len)` do pool.
- **Composição.** Foundation para devtools UI futuras + headless test harness + snapshot tests + IDE integration. Combina com state serialization (1.10), undo stack (2.3) e theme packs (2.4) para time-travel debugging completo no follow-up.
- **DL schema permanece 7.** Sem novos eventos, sem novos campos em structs existentes, sem novos tail fields em `capy_widget`/`capy_widget_context`.

### Docs

- `docs/compatibility.md` ganhou bloco "Devtools / inspector (since 2.5)"; ABI line bumpada para `v2.5`.
- `docs/roadmap/long-term/v2.5-devtools-inspector.md` status = delivered; deferidos documentados (dump textual de tree/DL, event recording/replay, headless harness CLI, IDE protocol).
- `docs/roadmap/contracts/abi-versions.md` linha 2.5 promovida para **atual**; 2.4 demovida para tagged; tag `v2.5.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.5.0 / `capy-ui-widget 2.5` / DL schema 7 / **213 testes** / fase 2.x com 6 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.5**.

## [2.4.0] — 2026-05-21

**Marco:** quinto minor da fase 2.x. **Theme packs** entregue em escopo binário-de-cores: formato canonical LE `.captheme` com magic/version/checksum fail-closed, 2 APIs (`validate`/`load`), forward-compat para token_ids desconhecidos. **Hot reload, Ed25519 signature, refs de fonts/ícones e metadata textual deferidos** para minors futuros.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/4/0`. `CAPYUI_API_VERSION_TAG = 0x00020400`.
- 7 novas macros publicadas em `capy_widget.h`:
  - `CAPY_THEME_PACK_MAGIC0..3` = `'C'`, `'T'`, `'H'`, `'M'`.
  - `CAPY_THEME_PACK_FORMAT_VERSION` = `1`.
  - `CAPY_THEME_PACK_HEADER_SIZE` = `16`.
  - `CAPY_THEME_PACK_ENTRY_SIZE` = `6`.
- 2 APIs em `src/widget/capy_widget.c`:
  - `int capy_theme_pack_validate(const void *buf, uint32_t len)` — checa magic, format_version ≤ `CAPY_THEME_PACK_FORMAT_VERSION`, reserved zero, length match, FNV-1a body checksum.
  - `int capy_theme_pack_load(const void *buf, uint32_t len, struct capy_theme *out)` — validates + applies tokens em `*out`, sobrescreve `variant`/`high_contrast`, preserva `version`.
- 4 helpers estáticos: `capy_theme_pack_fnv1a`, `capy_theme_pack_read_u16`, `capy_theme_pack_read_u32`.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `test_theme_pack_validate_well_formed_zero` — pack válido + NULL/zero guards.
  2. `test_theme_pack_load_applies_tokens` — tokens overwritten, variant/high_contrast aplicados, version preservado.
  3. `test_theme_pack_bad_magic_rejected` — magic alterada → -1 em validate e load.
  4. `test_theme_pack_future_version_rejected` — format_version+1 → -1.
  5. `test_theme_pack_checksum_mismatch_rejected` — flip de byte no body sem re-checksum.
  6. `test_theme_pack_unknown_token_skipped` — entry com token_id=250 silently skipada.
  7. `test_theme_pack_truncated_buffer_rejected` — declared count > buffer; header só; abaixo de header.
  8. `test_theme_pack_reserved_field_nonzero_rejected` — offset 14 != 0.

### Formato binário canonical LE

```
offset  size  field
0       4     magic = "CTHM"
4       2     format_version (uint16, currently 1)
6       1     variant (CAPY_THEME_VARIANT_*; rejected if >= 16)
7       1     high_contrast flag (0 or 1)
8       4     FNV-1a 32-bit checksum over bytes [16, end)
12      2     entry_count (uint16)
14      2     reserved (must be 0)
16+     ...   N entries of 6 bytes:
                uint8  token_id (>=1, <CAPY_TOKEN_COUNT)
                uint8  reserved (must be 0)
                uint32 colour_le (0xAARRGGBB little-endian)
```

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 3 → 4.
- `Makefile` banner para `2.4.0 (alpha.267) — ABI 2.x (theme packs aditivo)`.
- `VERSION` 2.3.0 → 2.4.0; `README.md` Version banner + URLs → `v2.4.0`.

### ABI

- `capy-ui-widget` **2.3 → 2.4** (aditivo: 7 macros novas + 2 APIs novas).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Zero allocation no core.** Pack vive no caller-buffer; load nunca aloca, nunca lê arquivo, nunca abre socket.
- **Endianness explícita.** Todos os multi-byte reads via helpers little-endian — blobs portáveis cross-platform.
- **FNV-1a 32-bit** sobre body bytes (`offset >= 16`), idêntico ao `CUIS` (v1.10) para auditoria comum.
- **Forward-compat** em `token_id`: entries com id ≥ `CAPY_TOKEN_COUNT` são silenciosamente skipadas. Packs de schemas futuros carregam sem fail em hosts antigos.
- **Fail-closed.** Magic incorreto, version > 1, reserved != 0, length mismatch, checksum errado, variant ≥ 16 ou high_contrast > 1 → `-1`.
- **Determinístico.** Mesmo `(buf, len)` → mesmo `*out`.
- **Não-objetivos.** Pack não carrega metadata textual (name/author/locale), refs de fonts, refs de ícones, signature. Esses ficam em envelope do host ou em minors futuros (`format_version = 2`).
- **Coexistência com v0.14:** `capy_theme_serialize` textual key=value continua diff-friendly para git; pack binário 2.4 é o formato de distribuição.

### Docs

- `docs/compatibility.md` ganhou bloco "Theme packs (since 2.4)"; ABI line bumpada para `v2.4`.
- `docs/roadmap/long-term/v2.4-theme-packs.md` status = delivered; deferidos documentados (hot reload, Ed25519, fonts/icons, metadata).
- `docs/roadmap/contracts/abi-versions.md` linha 2.4 promovida para **atual**; 2.3 demovida para tagged; tag `v2.4.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.4.0 / `capy-ui-widget 2.4` / DL schema 7 / **206 testes** / fase 2.x com 5 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.4**.

## [Unreleased] — roadmap additions

**Roadmap-only**: adicionadas 8 fatias planejadas cobrindo polish UX e features de usuário-final. Nenhum código de release. Slice docs criadas em `docs/roadmap/long-term/`.

- `v2.6.0` **planned** — Display mode (resolução + refresh rate). `struct capy_display_mode`, `capy_display_controller` com callbacks `enum_modes`/`set_mode`, 3 APIs forwarding, event aditivo `DISPLAY_MODE_CHANGED`. Settings → Display no desktop-session com rollback de 15s.
- `v2.7.0` **planned** — User management UI. `struct capy_user_account` (uid, username, display_name, avatar_image_id, is_admin, is_locked), `capy_user_directory` com 5 callbacks (list/create/update/delete/set_avatar). Settings → Users app. CapyUI nunca armazena senha — só forwarda.
- `v2.8.0` **planned** — Contrast & accessibility refinement. Reavaliação WCAG 2.1 AA dos 3 temas built-in + novo `default_dark_high_contrast` (AAA). APIs `capy_theme_contrast_ratio_x1000` (Q0 ×1000) + `capy_theme_audit_wcag` + `struct capy_contrast_finding`. Cálculo zero-float via tabela de linearização 256-entry.
- `v2.9.0` **planned** — Desktop icon arrangement. Drag-to-move + snap-to-grid + auto-arrange (name/type/mtime/size) + pinned. Persist via blob `CDLA` versionado em `/var/state/capyui/desktop-layout.bin`. Desktop-session only (sem bump de widget core).
- `v2.10.0` **planned** — File manager UX polish. Toolbar refatorada em 4 grupos (Navegação/Visualização/Ações/Layout), modo compacto < 600px colapsa em overflow `⋯`, breadcrumb com dropdown clicável, status bar fixa. Desktop-session only.
- `v2.11.0` **planned** — Icon & thumbnail system. `struct capy_icon_descriptor` (id + base_size + dpi_x256 + theme_variant + state), `capy_icon_provider` com `resolve_fn` + `thumbnail_request_fn` (async), 3 APIs, event aditivo `THUMBNAIL_READY`. Fallback determinístico `image_id=0`. Icon ids canônicos em novo contrato `docs/roadmap/contracts/icon-ids.md`.
- `v2.12.0` **planned** — Wallpaper management. `capy_wallpaper_config` com 5 fit modes (stretch/fill/fit/center/tile), per-user + per-monitor + slideshow + shuffle. Persist via blob `CWLP`. Desktop-session only.
- `v2.13.0` **planned** — Login screen with user avatars. Layouts adaptativos (1 user form direto; 2–6 grid 3-col 128×128; 7+ lista + search). Power options visíveis. Locked badge. Tab navigation determinística. Login wallpaper independente do desktop. Sem biometria/SSO (deferido para 3.x).

`docs/roadmap/contracts/abi-versions.md` recebeu linhas planejadas 2.6–2.13 entre 2.5 e 3.0.

## [2.3.0] — 2026-05-21

**Marco:** quarto minor da fase 2.x. **Undo/redo per-context** entregue com escopo focado: stack genérico caller-buffer-backed com FIFO eviction + branch cut em push. **Coalescing automático + auto-hook de TEXTBOX/RICH_TEXT deferidos** (hosts implementam externamente). Constrói sobre state serialization de 1.10.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/3/0`. `CAPYUI_API_VERSION_TAG = 0x00020300`.
- 2 structs em `src/widget/capy_widget.h`:
  - `struct capy_undo_entry { const char *action_id; const void *redo_data; uint32_t redo_len; const void *undo_data; uint32_t undo_len; uint32_t timestamp_ms; }`. Caller-owned pointers; widget core nunca copia bytes.
  - `struct capy_undo_stack { struct capy_undo_entry *entries; uint32_t capacity; uint32_t count; uint32_t redo_count; uint32_t coalesce_window_ms; }`. Lives at offset 0 of caller buffer; `entries` aponta para imediatamente após o header.
- Tail field aditivo em `capy_widget_context`: `struct capy_undo_stack *undo_stack` (NULL até `_init`).
- 6 APIs em `src/widget/capy_widget.c`:
  - `int capy_undo_stack_init(ctx, buf, buf_size)` — particiona buffer, computa capacity, attach. Default `coalesce_window_ms = 500`. Retorna -1 em NULL ctx/buf ou buffer < `sizeof(stack)+sizeof(entry)`.
  - `int capy_undo_push(ctx, action_id, redo, rlen, undo, ulen)` — push top; FIFO eviction quando capacity atingida; branch cut da redo stack.
  - `int capy_undo_undo(ctx, *out)` — pop top → redo stack; copia entry em `*out` (opcional). Retorna -1 em stack vazio.
  - `int capy_undo_redo(ctx, *out)` — pop redo → top; copia entry em `*out`. Retorna -1 em redo vazio.
  - `int capy_undo_can_undo(ctx)` / `int capy_undo_can_redo(ctx)` — 1 se non-empty, 0 caso contrário.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_undo_init_partitions_caller_buffer` — init, capacity, defaults.
  2. `test_undo_push_undo_redo_roundtrip` — entry copiada via `out`; transições de `can_undo`/`can_redo`.
  3. `test_undo_push_truncates_redo` — push após undo descarta redo (branch cut).
  4. `test_undo_fifo_eviction_on_full_buffer` — buffer cap=2; 3 pushes; oldest evicted.
  5. `test_undo_on_empty_returns_negative` — sem init: tudo -1/0; com stack vazio: undo/redo -1.
  6. `test_undo_null_guards_and_too_small_buffer` — NULL/buffer<header+entry.
  7. `test_undo_determinism_same_sequence` — dois contexts → estado idêntico.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 2 → 3.
- `Makefile` banner para `2.3.0 (alpha.266) — ABI 2.x (undo/redo aditivo)`.
- `VERSION` 2.2.0 → 2.3.0; `README.md` Version banner + URLs → `v2.3.0`.

### ABI

- `capy-ui-widget` **2.2 → 2.3** (aditivo: 2 structs novas, 1 tail field em context, 6 APIs novas).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Zero allocation no core.** Storage é o caller-buffer; entries armazenam só ponteiros (action_id + redo_data + undo_data) caller-owned.
- **Aplicação real do undo é callback do host** baseado em `action_id`. Widget core devolve a entry via `out`; host dispatcha.
- **FIFO eviction.** Push em buffer cheio faz shift O(N) das entries — trade-off de simplicidade por throughput. Hosts dimensionam buffer com mais capacity para históricos grandes.
- **Branch cut em push.** Qualquer push descarta a redo stack — usuário bifurcou da linhagem.
- **`coalesce_window_ms` é metadata em 2.3** (default 500). Hosts leem para implementar coalescing externo. O core não mescla entries.
- **Determinístico.** Buffer idêntico + mesma sequência (push/undo/redo) → mesmo (count, redo_count, top.action_id).
- **Foundation para devtools (v2.5).** Time-travel debugging constrói sobre este histórico + state serialization de 1.10.

### Docs

- `docs/compatibility.md` ganhou bloco "Undo/redo per-context (since 2.3)"; ABI line bumpada para `v2.3`.
- `docs/roadmap/long-term/v2.3-undo-redo.md` status = delivered; coalescing + auto-hook documentados como deferidos.
- `docs/roadmap/contracts/abi-versions.md` linha 2.3 promovida para **atual**; 2.2 demovida para tagged; tag `v2.3.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.3.0 / `capy-ui-widget 2.3` / DL schema 7 / **198 testes** / fase 2.x com 4 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.3**.

## [2.2.0] — 2026-05-21

**Marco:** terceiro minor da fase 2.x. **Virtualization data source plumbing** entregue com escopo conservador: `struct capy_virtual_data_source` + tail field em `capy_widget` + 3 APIs de acesso + 1 event type aditivo. O **viewport-only emit rewiring + row recycling** ficam para fatia futura aditiva — a infraestrutura está toda no lugar.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/2/0`. `CAPYUI_API_VERSION_TAG = 0x00020200`.
- 2 typedefs em `src/widget/capy_widget.h`:
  - `typedef int (*capy_virtual_get_count_fn)(void *user_data)`.
  - `typedef int (*capy_virtual_get_item_fn)(void *user_data, uint32_t index, char *out_text, uint16_t cap)`.
- `struct capy_virtual_data_source { get_count, get_item, user_data; }`.
- Tail field aditivo em `capy_widget`: `const struct capy_virtual_data_source *virtual_source` (NULL = eager rendering pre-2.2).
- 3 APIs em `src/widget/capy_widget.c`:
  - `void capy_widget_set_virtual_source(w, src)` — store pointer; NULL clear; NULL widget no-op.
  - `int capy_widget_virtual_count(w)` — forwards a `get_count(user_data)`; retorna `-1` em NULL widget / NULL source / NULL `get_count`.
  - `int capy_widget_virtual_get_item(w, index, out_text, cap)` — forwards a `get_item(...)`; retorna `-1` em NULL guards / `cap == 0`.
- Event type aditivo `CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE` appended ao tail de `capy_widget_event_type`. Host renderer emite com `event->x = start_index`, `event->y = end_exclusive` para sinalizar prefetch. `capy_widget_handle_event` retorna `0` (pass-through).
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_virtual_set_source_stores_pointer` — set/clear/NULL widget no-op.
  2. `test_virtual_count_forwards_to_callback` — `get_count` chamado uma vez, retorna 42.
  3. `test_virtual_get_item_decodes_index` — decodifica 0/42/999 em decimal; out-of-range retorna -1.
  4. `test_virtual_null_source_returns_negative` — sem source attach + source com fn ptrs NULL.
  5. `test_virtual_helpers_null_guards` — NULL widget / NULL out_text / cap=0; callback nunca chamado.
  6. `test_virtual_request_range_event_routes_without_crash` — event pass-through em `handle_event`.
  7. `test_virtual_source_on_non_list_widget_tolerated` — source aceito em LABEL (core não enforça tipo).

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 1 → 2.
- `Makefile` banner para `2.2.0 (alpha.265) — ABI 2.x (virtualization data source aditivo)`.
- `VERSION` 2.1.0 → 2.2.0; `README.md` Version banner + URLs → `v2.2.0`.

### ABI

- `capy-ui-widget` **2.1 → 2.2** (aditivo: 2 typedefs, 1 struct nova, 1 tail field em `capy_widget`, 3 APIs novas, 1 event type novo).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Plumbing entregue, viewport emit deferido.** Rewriting de `capy_widget_emit_recursive` para LIST/TREE/TABLE consultarem `virtual_source` ao invés de `children[]` é invasivo (toca o caminho de emit que tem byte-equivalence garantida desde 0.2). Melhor entregar a infraestrutura agora e fazer o rewiring em fatia dedicada que se concentre só nele.
- **Zero allocation no core.** Callbacks são caller-owned function pointers; `user_data` é opaque; widget core nunca toca os bytes além de NULL checks.
- **Determinismo.** Dado `(source, index)`, o resultado depende apenas do callback. Widget core não cacheia, não valida count entre chamadas.
- **`virtual_source` em widget non-LIST/TREE/TABLE é tolerado** mas semanticamente neutro. O host renderer decide se honra.
- **Event `VIRTUAL_REQUEST_RANGE`** carrega index range em `event->x`/`event->y` (reuso aditivo dos campos existentes de coord; sem novos campos no `capy_widget_event`).

### Docs

- `docs/compatibility.md` ganhou bloco "Virtualization data source (since 2.2)"; ABI line bumpada para `v2.2`.
- `docs/roadmap/long-term/v2.2-virtualization.md` status = delivered (plumbing-only); viewport emit + row recycling documentados como deferidos.
- `docs/roadmap/contracts/abi-versions.md` linha 2.2 promovida para **atual**; 2.1 demovida para tagged; tag `v2.2.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.2.0 / `capy-ui-widget 2.2` / DL schema 7 / **191 testes** / fase 2.x com 3 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.2**.

## [2.1.0] — 2026-05-21

**Marco:** segundo minor da fase 2.x. **Advanced widgets** entregues com escopo conservador: 8 novas entradas no `enum capy_widget_type` reservando slots para `TREE`, `TABLE`, `RICH_TEXT`, `CANVAS`, `CHART`, `COLOR_PICKER`, `DATE_PICKER`, `AUTOCOMPLETE`. Cada tipo integra com o pipeline genérico (layout, emit, focus, serialize, a11y) usando os campos existentes do `capy_widget`. **Per-widget state e APIs dedicadas (tree hierarchy, table columns, rich-text ranges, canvas draw callback, chart datasets, pickers, autocomplete suggestions) ficam para fatias 2.x dedicadas** (cada widget potencialmente seu próprio minor focado para evitar inflar `sizeof(capy_widget)` numa só fatia).

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/1/0`. `CAPYUI_API_VERSION_TAG = 0x00020100`.
- 8 entradas aditivas em `enum capy_widget_type` (appended ao tail após `CAPY_WIDGET_TABS`):
  - `CAPY_WIDGET_TREE` — hierarquia expansível com chevron, indentação, lazy load via callback (per-widget state deferido).
  - `CAPY_WIDGET_TABLE` — linhas/colunas, header sticky, sort callback, seleção single/multi (per-widget state deferido).
  - `CAPY_WIDGET_RICH_TEXT` — ranges de estilo (cor, peso, tamanho, link) no mesmo widget; caret/seleção reusa `capy_text_edit` de 0.4 (ranges deferidos).
  - `CAPY_WIDGET_CANVAS` — draw custom via callback que recebe display-list (callback API deferido).
  - `CAPY_WIDGET_CHART` — line/bar/pie básico com dataset via callback (dataset API deferido).
  - `CAPY_WIDGET_COLOR_PICKER` — HSV + hex + recent (model deferido).
  - `CAPY_WIDGET_DATE_PICKER` — calendário com locale (since 0.13; grid deferido).
  - `CAPY_WIDGET_AUTOCOMPLETE` — TEXTBOX + dropdown filtrado via callback (suggestion API deferido).
- 8 novos testes em `tests/test_widget_contracts.c`, um por tipo:
  1. `test_advanced_widget_tree_creates_and_lays_out` — cria, set_bounds, set_text.
  2. `test_advanced_widget_table_in_panel_emits` — table como filho de panel; `capy_widget_emit` produz ≥2 cmds.
  3. `test_advanced_widget_rich_text_basic` — set_text "Hello, world!".
  4. `test_advanced_widget_canvas_creates` — defaults visible=enabled=1.
  5. `test_advanced_widget_chart_creates` — dataset vazio não crasha; set_visible alterna.
  6. `test_advanced_widget_color_picker_focusable` — focusable + tab_index persistem.
  7. `test_advanced_widget_date_picker_respects_rtl_context` — `ctx.locale.rtl=1` observável.
  8. `test_advanced_widget_autocomplete_roundtrips_serialization` — serialize+deserialize entre 2 contexts preserva type/bounds/text; valida enum ordering.
- `capy_widget_serialize` (since 1.10): type-byte ceiling no deserialize moveu de `CAPY_WIDGET_TABS` para `CAPY_WIDGET_AUTOCOMPLETE`. Bytes acima do ceiling continuam rejeitados fail-closed.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 0 → 1.
- `Makefile` banner para `2.1.0 (alpha.264) — ABI 2.x (advanced widgets aditivo)`.
- `VERSION` 2.0.0 → 2.1.0; `README.md` Version banner + URLs → `v2.1.0`.
- `capy_widget_serialize.c`: type-byte ceiling no deserialize widened para `CAPY_WIDGET_AUTOCOMPLETE`.

### ABI

- `capy-ui-widget` **2.0 → 2.1** (aditivo: 8 enum entries novas; sem novos campos struct, sem APIs novas, sem mudanças no DL).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **7**.

### Notas de design

- **Por que enum-only nesta fatia.** Os 8 tipos juntos exigiriam ≈20 tail fields novos em `capy_widget` + ≈25 APIs dedicadas (TREE chevron + expanded set, TABLE columns + sort, RICH_TEXT ranges, CANVAS draw callback, CHART datasets, COLOR_PICKER HSV+recent, DATE_PICKER grid, AUTOCOMPLETE callback). Empacotar tudo numa só fatia inflaria descontroladamente `sizeof(capy_widget)`. A escolha conservadora reserva slots de enum agora — `capy_widget_create(ctx, CAPY_WIDGET_TREE)` retorna um widget válido com bounds/text/visible/enabled/focused funcionando — e deixa o per-widget state para minors 2.x onde cada widget ganha seu próprio incremento focado.
- **Integração gratuíta com 1.x.** Os novos tipos usam o pipeline genérico: `capy_widget_emit` produz CLIP_PUSH/RECT/...CLIP_POP, `capy_widget_serialize` armazena type+bounds+text, `capy_widget_a11y_snapshot` cai no role default (deferido para fatia dedicada).
- **Stub `capy_widget_ime_compose` (0.4)** continua funcionando para callers mínimos de RICH_TEXT enquanto a fatia rica fica pendente.
- **Deserialize ceiling widened.** Era a única mudança comportamental: `type_byte > CAPY_WIDGET_TABS` rejeita → agora `type_byte > CAPY_WIDGET_AUTOCOMPLETE`. Testes existentes do v1.10 (`test_state_unknown_type_rejected`) ainda passam porque usam `0xFF` (255), bem acima de qualquer tipo válido.
- **Sem mudança no DL.** Os 8 tipos emitem via op genéricas; sem novas ops, schema permanece 7.

### Docs

- `docs/compatibility.md` ganhou bloco "Advanced widget types (since 2.1)"; ABI line bumpada para `v2.1`; cobertura adiciona o novo enum.
- `docs/roadmap/long-term/v2.1-advanced-widgets.md` status = delivered (enum-only); per-widget APIs documentadas como deferidas.
- `docs/roadmap/contracts/abi-versions.md` linha 2.1 promovida para **atual**; 2.0 demovida para tagged; tag `v2.1.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.1.0 / `capy-ui-widget 2.1` / DL schema 7 / **184 testes** / fase 2.x com 2 fatias entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.1**.

## [2.0.0] — 2026-05-21

**Marco:** **major bump conservador**. Primeira fatia da fase 2.x. Plugin system + SDK público introduzidos. **Nenhum símbolo do 1.x foi removido** — o bump 1.10 → 2.0 é obrigatório pelo SemVer (nova op no DL pode surpreender decoders ingênuos pre-schema-7; novo ABI surface; deprecation policy agora ARMADA para minors 2.x). 1.x continua em **LTS ≥12 meses pós-2.0** conforme `docs/roadmap/contracts/deprecation-policy.md`.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `2/0/0`.
- 2 novas macros públicas em `src/widget/capy_widget.h`:
  - `CAPYUI_API_VERSION_TAG` — composite ABI tag `(major << 16) | (minor << 8) | patch` (= `0x00020000`). Plugins comparam contra este valor.
  - `CAPY_MAX_PLUGINS` — static cap por contexto (`8`). Zero-alloc; hosts que precisam de mais forkam o limite em compile time.
- `struct capy_plugin_descriptor` em `capy_widget.h`:
  - `const char *id` (reverse-DNS) + `const char *version` (semver string).
  - `uint32_t capy_ui_abi_required` (compile-time gate).
  - `struct capy_widget_allocator allocator` (allocator isolado por plugin; o core armazena mas nunca chama).
  - `uint32_t timeout_microseconds` (hint host-enforced).
  - `int (*init)(pc)`, `void (*destroy)(pc)`, `int (*on_event)(pc, target, ev)`, `int (*emit)(pc, target, dl)` (todos opcionais NULL).
- `struct capy_plugin_context { const capy_plugin_descriptor *descriptor; void *user_data; }`.
- 2 widget context tail fields (`plugins[CAPY_MAX_PLUGINS]` + `plugin_count`). `capy_widget_context_init` zera ambos.
- 2 APIs em `src/widget/capy_widget.c`:
  - `int capy_plugin_register(ctx, &desc)` valida (non-NULL ctx + desc, `capy_ui_abi_required <= CAPYUI_API_VERSION_TAG`, descritor não duplicado, `plugin_count < CAPY_MAX_PLUGINS`) e empurra. Retorna o slot index (0..7) no sucesso, `-1` em falha sem mutação.
  - `void capy_plugin_unregister_all(ctx)` walks o registro, chama cada `destroy` non-NULL com um `capy_plugin_context` stack-local, e zera o count. Idempotente.
- Op aditiva `CAPY_DL_PLUGIN_OP` em `capy_dl_op` (`src/widget/capy_display_list.h`):
  - Carrega 32-byte payload nos campos existentes do `capy_dl_cmd` (`rect` + `color` + `text_offset` + `text_len` + `border_width` + `font_size` + `font_id` + `image_id` = 32 bytes).
  - `image_id` é o canal canônico do plugin id (host mapeia de volta).
  - `capy_widget_emit` **nunca emite** este op — host-side plugin code o append via `emit` callback.
- `CAPY_DISPLAY_LIST_SCHEMA_VERSION` bumpado de `6` para `7` (terceiro bump real desde 0.9; 4→5 em 1.5, 5→6 em 1.9, 6→7 em 2.0).
- GPU translator (`capy_dl_to_quads`, since 1.2) ganha case para `CAPY_DL_PLUGIN_OP`: skip (metadata para plugin renderer registrado).
- **SDK público em `sdk/`** (seed para 2.0):
  - `sdk/README.md` — guia curador para autores de plugin.
  - `sdk/plugin-template.c` — descriptor compilável com todos os callbacks no-op documentados.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `test_plugin_version_tag_macro` — macros 2/0/0, `CAPYUI_API_VERSION_TAG == 0x00020000`.
  2. `test_plugin_register_valid_descriptor` — aceita; duplicate pointer rejeitado.
  3. `test_plugin_rejects_abi_mismatch` — `required > host` rejeitado; `==` e `<` aceitos.
  4. `test_plugin_capacity_overflow_rejected` — 8 aceitos, 9º `-1` sem mutação.
  5. `test_plugin_unregister_all_calls_destroy` — destroy chamado uma vez; idempotente.
  6. `test_plugin_null_guards` — `register(NULL, *)`/`register(*, NULL)` → `-1`; `unregister_all(NULL)` no-op.
  7. `test_plugin_op_in_dl_skipped_by_gpu_translator` — PLUGIN_OP + RECT → 1 quad emitido.
  8. `test_plugin_dl_schema_bumped_to_7` — schema 7; `CAPY_DL_PLUGIN_OP > CAPY_DL_TRANSFORM_POP`.
- 3 assertions schema-6 → schema-7 atualizadas (em `test_displaylist_emits_font_id` e no freeze marker).

### Mudado

- Macros `CAPYUI_API_VERSION_MAJOR` 1 → **2**, `_MINOR` 10 → 0.
- `Makefile` banner para `2.0.0 (alpha.263) — ABI **2.0** (major bump, plugin system + SDK público)`.
- `VERSION` 1.10.0 → **2.0.0**; `README.md` Version banner + URLs → `v2.0.0`.

### ABI

- `capy-ui-widget` **1.10 → 2.0** (major bump conservador: 2 structs novas, 2 APIs novas, 2 macros novas, 2 widget context tail fields, 1 op DL nova, 1 schema bump). **Nenhum símbolo 1.x removido**.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **6 → 7**. Terceiro bump real desde 0.9.
- `sizeof(capy_widget_context)` cresce (registry de 8 pointers + count). Hosts que stack-allocate context precisam recompilar.

### Notas de design

- **Por que major bump em vez de minor.** SemVer requer major bump quando:
  1. Uma op nova no display-list pode surpreender decoders ingênuos que tratam unknown ops como erro (escolha conservadora do consumer). Aditividade ABI não exime decoders.
  2. Novo ABI surface (plugin) que abrirá quebras de remoção em minors futuras sob a `deprecation-policy.md`.
  3. Comunica claramente "LTS de 1.x começa agora".
- **Conservador no commit, agressivo no contrato.** Compatível em código-fonte com 1.10.0 modulo as 3 asserts de schema literal. Mítico: major bump não quebra **agora**, abre a porta para quebrar em 2.x minors.
- **Plugin core nunca invoca callbacks.** Mantém o widget core float-free, determinístico e crash-isolado por construção. Host dispatcher itera o registro e aplica sandbox próprio (timeout via signal/alarm, signal handler para SIGSEGV, etc.). Por isso `init`/`destroy`/`on_event`/`emit` são todos opcionais (NULL aceito) — se o host nunca chamar, eles nunca rodam.
- **`unregister_all` chama destroy.** Esta é a única exceção à regra "core nunca invoca callbacks": durante unregister, `destroy` precisa rodar para o plugin liberar seus recursos. Documentado.
- **Allocator isolation auditável.** O descritor carrega seu próprio `capy_widget_allocator` separado do `capy_widget_context::allocator`. O core nunca mistura mas também nunca usa o allocator do plugin diretamente — auditoria fica com o host.
- **Plugin DL op como metadata canal.** `CAPY_DL_PLUGIN_OP` reusa os 32 bytes dos campos existentes do `capy_dl_cmd` (rect 16B + color 4B + text fields 6B + image_id 4B = 30B + 2B border/font = 32B). `sizeof(capy_dl_cmd)` **não muda em 2.0**. Plugin schema interno define os 32 bytes; `image_id` recebe o plugin id por convenção.
- **SDK seed minimal.** `sdk/README.md` + `sdk/plugin-template.c` são suficientes para um autor compilar um plugin no-op. Iterações futuras podem vendor headers curados sob `sdk/include/capyui/`.
- **LTS de 1.x.** Conforme `docs/roadmap/contracts/deprecation-policy.md`: 1.x recebe security e bug-fix patches por **≥12 meses pós-2.0**. Major-bump não retira features.
- **Cross-repo:** o documento `tracking/cross-repo-sync-pending.md` será atualizado para refletir o major bump + bump de DL schema + nova plugin surface.

### Docs

- `docs/compatibility.md` ganhou bloco "Plugin system (since 2.0)"; ABI line bumpada para `v2.0`; cobertura cita schema v7.
- `docs/roadmap/long-term/v2.0-plugin-system-sdk.md` status = delivered (escopo conservador); breakage diferidos para 2.x minors documentados.
- `docs/roadmap/contracts/abi-versions.md` linha 2.0 promovida para **atual** (major bump conservador); 1.10 demovida para tagged (LTS); tag `v2.0.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 2.0.0 / `capy-ui-widget 2.0` / DL schema **7** / **176 testes** / fase 1.x linear concluída + 2.x iniciada.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 2.0** + alerta sobre **major bump + 3º bump de DL schema**.
- Novo diretório `sdk/` com 2 arquivos (README + plugin-template.c).

## [1.10.0] — 2026-05-21

**Marco:** décimo minor pós-freeze e **última fatia da fase 1.x linear**. State serialization no widget core: formato binário canonical little-endian para crash recovery, hot reload e devtools snapshot. Próximo marco no roadmap é v2.0 (Plugin system + SDK público, primeira quebra controlada pós-1.0 sob a `deprecation-policy.md`).

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `1/10/0`.
- Novo source file `src/widget/capy_widget_serialize.c` adicionado a `SRC_WIDGET` no Makefile.
- 8 constantes públicas em `src/widget/capy_widget.h`:
  - `CAPY_STATE_MAGIC0/1/2/3` ('C', 'U', 'I', 'S').
  - `CAPY_STATE_FORMAT_VERSION` (atualmente `1`).
  - `CAPY_STATE_HEADER_SIZE` (`16` bytes).
  - Flag bits: `CAPY_STATE_FLAG_VISIBLE`/`ENABLED`/`FOCUSED`/`FOCUSABLE`/`CHECKED`.
- 3 APIs públicas:
  - `int capy_widget_serialize(root, out, cap, &out_len)` — escreve blob binário canônico; retorna `0` no sucesso com `*out_len` setado, `-1` em NULL args / capacity overflow / count failure (com `*out_len` refletindo a parte escrita antes da falha quando aplicável).
  - `int capy_widget_deserialize(buf, len, ctx, &out_root)` — valida magic, version, reserved, checksum, node_count; reconstrói o tree via `ctx->allocator`. Retorna `0` no sucesso, `-1` em qualquer falha (todas as alocações parciais são destruidas antes do retorno → no leak).
  - `int capy_widget_serialize_text(root, out, cap)` — dump de debug indentado `[type=N bounds=(x,y,w,h) flags=ve text="..." children=K]\n`. Retorna byte length escrito (sem NUL) no sucesso, `-1` em overflow / NULL.
- Formato binário canonical **little-endian** para portabilidade cross-host:
  - Header de 16 bytes: magic `"CUIS"` (4) + format_version (uint16 LE) + reserved (uint16 LE, deve ser `0`) + FNV-1a 32-bit checksum (uint32 LE) sobre o body + node_count (uint32 LE).
  - Per node (pre-order, variable): `uint8 type`, `uint8 flags`, `int16 tab_index`, `int32 x`, `int32 y`, `uint32 w`, `uint32 h`, `uint16 text_len`, `text_len` UTF-8 bytes, `uint16 child_count`. Limites enforçados no deserialize: `child_count <= CAPY_WIDGET_MAX_CHILDREN` (32) e `text_len <= CAPY_WIDGET_MAX_TEXT - 1` (255).
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_state_serialize_header_and_magic` — magic, version, reserved, node_count.
  2. `test_state_round_trip` — serialize ctx_a → deserialize ctx_b preserva type/bounds/text/focused/tab_index/child_count.
  3. `test_state_determinism` — mesma árvore 2× → mesmos bytes.
  4. `test_state_rejects_bad_magic_version_checksum` — 3 corruptions → `-1`.
  5. `test_state_capacity_overflow_no_corruption` — buf < HEADER_SIZE, buf < body, NULL guards.
  6. `test_state_text_dump` — formato indentado; capacity overflow; NULL guards.
  7. `test_state_unknown_type_rejected` — type > `CAPY_WIDGET_TABS` (com checksum válido) rejeitado.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 9 → 10.
- `Makefile` banner para `1.10.0 (alpha.262) — ABI 1.x (décimo minor pós-freeze)`. `SRC_WIDGET` ganha `src/widget/capy_widget_serialize.c`.
- `VERSION` 1.9.0 → 1.10.0; `README.md` Version banner + URLs → `v1.10.0`.

### ABI

- `capy-ui-widget` **1.9 → 1.10** (aditivo: 8 constantes, 3 APIs públicas, 1 source file novo). Nenhum struct existente tocado.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **6** (serialização não toca o DL).

### Notas de design

- **Pre-order tree walk** + variable-length text per node. Compacto sem ser binário ofuscado.
- **Little-endian explícito.** Byte primitives (write_u16, write_u32, write_i32) usam shifts; idem para read. Portabilidade cross-host gratuita.
- **FNV-1a 32-bit checksum** sobre o body (após o header). Deterministic, zero-state, sem divisão. Validado **antes** de qualquer alocação no deserialize.
- **Fail-closed em deserialize:** bad magic (≠ "CUIS"), version (≠ 1), reserved (≠ 0), checksum mismatch, `text_len > 255`, `child_count > CAPY_WIDGET_MAX_CHILDREN`, type byte > `CAPY_WIDGET_TABS`, capacity overflow, trailing bytes além do esperado → todos retornam `-1`.
- **Zero leak em falha mid-tree.** Cada `deserialize_node` recursivo destroi seu próprio widget (e seus children já adicionados) antes de propagar `-1`. Caller só vê `out_root == NULL`.
- **O que NÃO é preservado (deferred):** text_edit caret/seleção, anim ativas, theme tokens, locale, transform pointer, DnD config, IME state, layout cache, pool/slab state, callbacks (`on_click`/`on_change`/`on_submit`/`on_drop`), `user_data`, `font_id`, `image_id` — hosts re-conectam/rebind após deserialize. Mantém serialização pequena e focada no "minimal restorable state" para v1.10. Fatia futura aditiva pode expandir o formato (bump `format_version` para `2`) sem quebrar leitores 1.10.
- **Foundation para v2.x.** Time-travel debugging (v2.5) e undo/redo (v2.3) constroem sobre este formato.
- **Sem novo header.** As APIs vivem em `capy_widget.h` para não inflar o número de includes.

### Docs

- `docs/compatibility.md` ganhou bloco "State serialization (since 1.10)"; ABI line bumpada para `v1.10`.
- `docs/roadmap/long-term/v1.10-state-serialization.md` status = delivered (escopo conservador).
- `docs/roadmap/contracts/abi-versions.md` linha 1.10 promovida para **atual**; tag `v1.10.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.10.0 / `capy-ui-widget 1.10` / DL schema 6 / **168 testes** / fase 1.x linear **CONCLUÍDA**.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 1.10**.

## [1.9.0] — 2026-05-21

**Marco:** nono minor pós-freeze. Transforms 2D no display-list (`CAPY_DL_TRANSFORM_PUSH/POP`) com matriz 2×3 affine em Q16.16. **Segundo bump real de DL schema** desde 0.9 (primeiro foi 4→5 em v1.5; agora 5→6). Arbitrary-angle rotation e hit-test compensation deferidos para manter o core float-free.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `1/9/0`.
- 2 ops novas em `capy_dl_op` (aditivo, appended ao tail):
  - `CAPY_DL_TRANSFORM_PUSH` — carrega matriz em `cmd->transform`; stacks no transform stack do renderer.
  - `CAPY_DL_TRANSFORM_POP` — unwind do top do stack.
- `struct capy_dl_transform { int32_t m00, m01, m02, m10, m11, m12; }` declarada em `src/widget/capy_display_list.h`. 2×3 affine; `m00`/`m01`/`m10`/`m11` Q16.16 (linear), `m02`/`m12` integer pixels (translation). Identity = `{0x10000, 0, 0, 0, 0x10000, 0}`.
- Novo field `struct capy_dl_transform transform` em `capy_dl_cmd` (adiciona 24 bytes à `sizeof(capy_dl_cmd)`). Zero-fill para todas as ops exceto TRANSFORM_PUSH (garantido por `capy_dl_push`).
- Widget tail field `const struct capy_dl_transform *transform` em `capy_widget` (NULL=identity, sem emit).
- `CAPY_DISPLAY_LIST_SCHEMA_VERSION` bumpado de `5` para `6`.
- 6 APIs novas em `src/widget/capy_widget.c`:
  - `capy_transform_identity()` — diagonal de 1s em Q16.16.
  - `capy_transform_scale(sx_q16, sy_q16)` — diagonal scale.
  - `capy_transform_translate(tx, ty)` — pure translation; tx/ty integer pixels.
  - `capy_transform_rotate_quadrants(n)` — exato 0°/90°/180°/270° com `n` reduzido modulo 4. Sem LUT.
  - `capy_transform_multiply(a, b)` — Q16.16 matrix multiply com `int64_t` intermediates e saturating clamp a `INT32_MAX/MIN`. NULL args caem para identity.
  - `capy_widget_set_transform(w, t)` — caller-owned pointer; NULL clear.
- `capy_widget_emit_recursive` (em `capy_display_list.c`) agora wraps cada widget com TRANSFORM_PUSH (carregando `*w->transform`) antes do CLIP_PUSH e com TRANSFORM_POP após o CLIP_POP, quando `w->transform != NULL`. NULL transform = byte-equivalente a pre-1.9.
- GPU translator (`capy_dl_to_quads`, since 1.2) ganha case para TRANSFORM_PUSH/POP: skip (metadata para backend).
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_transform_identity_is_diagonal`
  2. `test_transform_scale_and_translate`
  3. `test_transform_rotate_quadrants_exact` (4 quadrantes + modulo)
  4. `test_transform_multiply_identity_neutral` (I*X = X*I = X; NULL→identity)
  5. `test_transform_multiply_composes` (scale*translate vs translate*scale demonstra não-comutatividade)
  6. `test_transform_widget_emits_push_pop` (DL: TRANSFORM_PUSH primeiro + TRANSFORM_POP último; clear volta byte-equivalente)
  7. `test_transform_gpu_translator_skips_transform_ops`
- 3 assertions schema-5 → schema-6 atualizadas (em `test_displaylist_emits_font_id` e no freeze marker).

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 8 → 9.
- `Makefile` banner para `1.9.0 (alpha.261) — ABI 1.x (nono minor pós-freeze)`.
- `VERSION` 1.8.0 → 1.9.0; `README.md` Version banner + URLs → `v1.9.0`.

### ABI

- `capy-ui-widget` **1.8 → 1.9** (aditivo: 1 struct nova, 2 ops novas, 1 field novo em `capy_dl_cmd`, 1 tail field novo em `capy_widget`, 6 APIs novas). Nenhum struct existente reordenado; nenhum campo removido.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **5 → 6**. Segundo bump real desde 0.9 (o primeiro foi 4→5 em v1.5). Consumers que leem `dl->version < 6` devem ignorar TRANSFORM_PUSH/POP.

### Notas de design

- **Float-free no core mantido.** `multiply` usa `int64_t` intermediates + truncate-toward-zero division (`/ 0x10000`) + saturating clamp. Determinístico cross-platform.
- **`m02`/`m12` em integer pixels** (não Q16.16) para conveniência do caller: passa screen coords diretas. `multiply` lida com a conversão internamente.
- **Arbitrary-angle rotation deferido.** Não incluímos `capy_transform_rotate(degrees)` porque exige `sin`/`cos` (CORDIC ou LUT) e o core fica float-free. Hosts que precisam de rotações não-quadrante computam `sin`/`cos` próprios e populam matriz via `multiply` ou direto.
- **Hit-test compensation deferida.** Widget bounds continuam axis-aligned em `capy_widget_handle_event`. Renderer aplica a matriz; eventos chegam em coords não-transformadas. Hosts que querem hit em widget rotacionado aplicam inverse transform externamente antes de `handle_event`. Documentado.
- **`sizeof(capy_dl_cmd)` cresce 24 bytes.** Hosts que stack-allocate arrays podem precisar recompilar. Necessário para carregar a matriz inline sem pointer indirection.
- **Byte-equivalência preservada** quando `widget->transform == NULL`: emit não introduz TRANSFORM_PUSH/POP. Existing tests passam sem mudança comportamental além das 3 assertions de schema literal.
- **GPU translator skip.** TRANSFORM_PUSH/POP são metadata; o backend host aplica a matriz no seu próprio matrix stack antes de consumir os quads. Análogo a DPI_SCOPE de v1.5.

### Docs

- `docs/compatibility.md` ganhou bloco "2D transforms (since 1.9)"; ABI line bumpada para `v1.9`; cobertura cita schema v6.
- `docs/roadmap/long-term/v1.9-transforms-2d.md` status = delivered; arbitrary-angle e hit-test documentados como deferidos.
- `docs/roadmap/contracts/abi-versions.md` linha 1.9 promovida para **atual**; tag `v1.9.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.9.0 / `capy-ui-widget 1.9` / DL schema **6** / **161 testes** / longo prazo 9/17.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 1.9** + alerta sobre segundo bump de schema.

## [1.8.0] — 2026-05-21

**Marco:** oitavo minor pós-freeze. Modelo IME rico no widget core: preedit text + candidate list + composition phase armazenados como tail fields em `capy_widget` (válidos para TEXTBOX), com 5 APIs para o host engine driver (mozc/anthy/hangul-engine/etc.). Engine real fica no CapyOS; widget core nunca aloca/copia/libera strings de IME.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `1/8/0`.
- 4 event types novos appended no tail de `capy_widget_event_type`:
  - `CAPY_WIDGET_EVENT_IME_COMPOSE_START`
  - `CAPY_WIDGET_EVENT_IME_PREEDIT_UPDATE`
  - `CAPY_WIDGET_EVENT_IME_COMMIT`
  - `CAPY_WIDGET_EVENT_IME_CANCEL`
- `struct capy_ime_state { const char *preedit; uint16_t preedit_len, cursor_pos; const char *const *candidates; uint16_t candidate_count, selected_index; uint8_t composition_phase; uint8_t reserved[3]; }` — snapshot struct.
- 9 widget tail fields aditivos em `struct capy_widget` (válidos só em TEXTBOX): `ime_preedit`, `ime_preedit_len`, `ime_cursor_pos`, `ime_candidates`, `ime_candidate_count`, `ime_selected_index`, `ime_composition_phase`, `ime_reserved[3]`.
- 5 APIs em `src/widget/capy_widget.c`:
  - `capy_ime_set_preedit(w, text, len, cursor)` — phase → 1 (composing); no-op em non-TEXTBOX.
  - `capy_ime_set_candidates(w, list, count, selected)` — phase → 2 (selecting) quando count>0; count=0 limpa candidates sem mudar phase.
  - `capy_ime_commit(w, final_text, len)` — chama `capy_textbox_insert` e clear state (mesmo se insert rejeitado). Retorna o resultado do insert.
  - `capy_ime_cancel(w)` — clear state sem inserir.
  - `capy_ime_get_state(w, &out)` — snapshot; non-TEXTBOX zero-fill; NULL-safe em ambos args.
- Roteamento: 4 IME event types são no-ops em `capy_widget_handle_event` (return 0). Host dispatcher usa para broadcast de phase transitions.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_ime_set_preedit_marks_composing`
  2. `test_ime_set_candidates_marks_selecting`
  3. `test_ime_commit_inserts_and_clears`
  4. `test_ime_cancel_preserves_textbox_content`
  5. `test_ime_apis_are_noop_on_non_textbox`
  6. `test_ime_event_types_routed_without_crash`
  7. `test_ime_get_state_snapshot`

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 7 → 8.
- `Makefile` banner para `1.8.0 (alpha.260) — ABI 1.x (oitavo minor pós-freeze)`.
- `VERSION` 1.7.0 → 1.8.0; `README.md` Version banner e URLs → `v1.8.0`.

### ABI

- `capy-ui-widget` **1.7 → 1.8** (aditivo: 4 event types novos, 1 struct nova, 9 widget tail fields, 5 APIs públicas). Nenhum struct existente tocado.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **5** (candidate window via `CAPY_DL_CANDIDATE_WINDOW` adiada para fatia futura).

### Notas de design

- **Engine fica no host.** CapyUI nunca implementa CJK/Hangul/Indic engines; apenas modela o state. Hosts plugam mozc, anthy, hangul-engine, libsamantha, etc.
- **Zero alloc no core.** Todas as strings (preedit, final_text, candidates) são caller-owned. Host mantém alive até a próxima chamada `capy_ime_*`.
- **APIs no-op em non-TEXTBOX.** Mantém invariante de que IME só opera em widgets de entrada de texto.
- **`commit` sempre limpa state**, mesmo quando `capy_textbox_insert` rejeita (textbox cheio, readonly, etc.) — host engine surface a rejeição via canal próprio.
- **`set_candidates(count=0)` não muda phase.** Permite limpar a lista (e.g. único candidato selecionado) sem voltar a "composing" desnecessariamente.
- **4 IME event types pass-through.** Host dispatcher os usa para sinalizar redraw/notificações; core só armazena o state via setters explícitos.
- **Stub `capy_widget_ime_compose` (0.4) preservado.** Callers que só precisam de preedit forwarding mínimo continuam funcionando.
- **Candidate window no display-list adiado.** Fatia futura pode introduzir `CAPY_DL_CANDIDATE_WINDOW` (DL schema 5 → ?). Em 1.8 o host desktop session renderiza popup externamente lendo `capy_ime_get_state`.

### Docs

- `docs/compatibility.md` ganhou bloco "Rich IME (since 1.8)"; ABI line bumpada para `v1.8`.
- `docs/roadmap/long-term/v1.8-ime-rich.md` status = delivered; candidate window deferida documentada.
- `docs/roadmap/contracts/abi-versions.md` linha 1.8 promovida para **atual**; tag `v1.8.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.8.0 / `capy-ui-widget 1.8` / DL schema 5 / **154 testes** / longo prazo agora com 8/17 entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 1.8** + alerta sobre 4 event types IME_*.

## [1.7.0] — 2026-05-21

**Marco:** sétimo minor pós-freeze. Performance tier 2 entregue com escopo conservador: **slab allocator** adicionado como companion ao bump allocator (`capy_widget_pool`, since 0.15). Display-list compression e hash measure-cache do plano original foram deferidos porque o trio existente `capy_widget_diff`/`capy_display_list_diff_incremental`/`layout_dirty` já cobre as necessidades práticas; futuras fatias aditivas podem completar o tier 2 sem novo bump de schema.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `1/7/0`.
- `struct capy_slab { void *buffer; uint32_t size, element_size, high_water; void *free_list; }` em `src/widget/capy_widget.h`. Fields são todos caller-inspecionáveis.
- 3 APIs em `src/widget/capy_widget.c`:
  - `void capy_slab_init(s, buf, size, element_size)` — particiona o buffer em `floor(size / element_size)` slots, zera `high_water` e `free_list`. Fail-closed em `element_size < sizeof(void *)`, NULL buffer ou size==0: `s->size` clampa para 0 e todos os `_alloc` subsequentes retornam NULL.
  - `void *capy_slab_alloc(s)` — pop LIFO da free-list (se houver); senão bump `high_water`; senão NULL. O(1).
  - `void capy_slab_free(s, ptr)` — push LIFO. Sem validação de range (caller-responsibility). O(1).
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_slab_init_partitions_buffer`
  2. `test_slab_alloc_returns_distinct_pointers_until_full`
  3. `test_slab_free_returns_to_lifo_pool` — reuse LIFO + bump após drain.
  4. `test_slab_degenerate_init_returns_null_on_alloc` (3 sub-cases).
  5. `test_slab_null_guards`
  6. `test_slab_alloc_pointer_alignment` — slots alinhados a `sizeof(void *)`; consecutivos exatamente `element_size` bytes adiante.
  7. `test_slab_determinism_same_addresses` — dois slabs sobre buffers gemelos consumindo a mesma sequência produzem os mesmos índices de slot.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 6 → 7 (`MAJOR=1`, `PATCH=0`).
- `Makefile` banner para `1.7.0 (alpha.259) — ABI 1.x (sétimo minor pós-freeze)`. `lint` e `version-check` exigem `VERSION == 1.7.0`.
- `VERSION` 1.6.0 → 1.7.0.
- `README.md` Version banner e URLs apontam para `v1.7.0`.

### ABI

- `capy-ui-widget` **1.6 → 1.7** (aditivo: 1 struct nova, 3 APIs novas). Nenhum struct existente tocado.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **5** (slab não emite no DL).

### Notas de design

- **Slab vs bump allocator.** `capy_widget_pool` (since 0.15) é bump-allocator monotônico (zero free per-element); ideal para arenas que crescem até reset. `capy_slab` é fixed-size com free LIFO; ideal para tight loops com reuse frequente (e.g. animation keyframe pool, gesture recognizer pool, transient layout nodes). Os dois coexistem; caller escolhe.
- **Free-list inline.** A free-list mora dentro dos slots liberados (cada slot livre guarda um `void *` para o próximo livre). Isso impõe `element_size >= sizeof(void *)`. Fail-closed via `s->size = 0` quando essa invariante quebra.
- **Sem validação de range em `_free`.** Mantém a operação O(1). Caller é responsável; isso é consistente com o resto do core (e.g. `capy_widget_destroy`, `capy_slab` peer).
- **Compression e hash measure-cache adiados.** O plano original previa também `capy_display_list_compress`/`decompress` e uma hash table de subtree → measure result. Adiei porque:
  - `capy_widget_diff` (0.8) e `capy_display_list_diff_incremental` (1.1) já produzem diff de display-list para transmissão; compression "true" só faria sentido para reduzir bytes em fio de rede, o que está fora do escopo do widget core.
  - `layout_dirty` + `layout_version` (0.15) + `dirty_version` (1.1) já cobrem invalidação coarse. Hash table por subtree adicionaria complexidade significativa por ganho marginal nas hot paths atuais.
- **Benchmarks 100k widgets em 16 ms.** Alvo do plano. Não executado neste workspace (política local read-only). Recomendado como gate externo de CI no CapyOS.

### Docs

- `docs/compatibility.md` ganhou bloco "Slab allocator (since 1.7)"; ABI line bumpada para `v1.7`; cobertura inclui slab e nota os items deferidos do plano.
- `docs/roadmap/long-term/v1.7-performance-tier-2.md` status = delivered (escopo conservador); testes catalogados; compression/hash-cache documentados como deferidos.
- `docs/roadmap/contracts/abi-versions.md` linha 1.7 promovida para **atual**, 1.6 demovida; tag `v1.7.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.7.0 / `capy-ui-widget 1.7` / DL schema 5 / **147 testes** / longo prazo agora com 7/17 entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 1.7**.

## [1.6.0] — 2026-05-21

**Marco:** sexto minor pós-freeze. Drag and drop com modelo de dados completo + roteamento de DROP no widget core. DRAG_BEGIN/MOVE/END são pass-through (no-op) — host dispatcher possui o ciclo de preview/cancel. Zero alloc no core; payload e filter array são caller-owned.

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `1/6/0`.
- 4 event types novos appended no tail de `capy_widget_event_type` (`src/widget/capy_widget.h`):
  - `CAPY_WIDGET_EVENT_DRAG_BEGIN`
  - `CAPY_WIDGET_EVENT_DRAG_MOVE`
  - `CAPY_WIDGET_EVENT_DRAG_END`
  - `CAPY_WIDGET_EVENT_DROP`
- `struct capy_dnd_payload { char type[32]; const void *data; uint32_t len; }`. Type é NUL-terminated MIME-like com 32 bytes inline (zero-alloc). Data e len são informacionais — o core não lê através de `data`.
- 5 widget tail fields aditivos em `struct capy_widget` (todos NULL/zero por default):
  - `const struct capy_dnd_payload *drag_payload` (turns widget into drag source).
  - `const char *const *drop_accepted_types`
  - `uint32_t drop_types_count`
  - `int (*on_drop)(target, payload, user_data)`
  - `void *drop_user_data`
- 3 setter APIs:
  - `capy_widget_set_draggable(w, payload)` — NULL-safe; pass NULL para clear.
  - `capy_widget_set_drop_target(w, types, count)` — NULL array clampa count a zero.
  - `capy_widget_set_on_drop(w, fn, user_data)`.
- 2 matching helpers:
  - `int capy_dnd_type_matches(accepted, type)` — suporta `"*"` (any), `"prefix/*"` (prefix), exact. Case-insensitive em ASCII; bytes acima de 0x7F comparados literalmente. Sem char classes, sem regex.
  - `int capy_widget_dnd_accepts(w, payload)` — OR sobre o filter array do widget.
- Roteamento em `capy_widget_handle_event`:
  - `CAPY_WIDGET_EVENT_DROP` walks children deepest-first; primeiro widget sob `(event->x, event->y)` com `on_drop` setado E filter que aceita o payload type wins. Callback retorna `1` para parar propagação ou `0` para continuar.
  - `DRAG_BEGIN/MOVE/END` são silenciosamente aceitos (return 0). Host dispatcher possui o stado entre BEGIN e END (cursor pos, preview, ESC cancel).
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `test_dnd_set_draggable_attaches_payload`
  2. `test_dnd_set_drop_target_attaches_filter`
  3. `test_dnd_type_match_exact_wildcard_prefix`
  4. `test_dnd_drop_invokes_callback_when_compatible`
  5. `test_dnd_drop_silently_ignored_when_incompatible`
  6. `test_dnd_event_types_routed_without_crash`
  7. `test_dnd_dnd_accepts_helper`
  8. `test_dnd_drop_deepest_first`

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 5 → 6 (`MAJOR=1`, `PATCH=0`).
- `Makefile` banner para `1.6.0 (alpha.258) — ABI 1.x (sexto minor pós-freeze)`. `lint` e `version-check` exigem `VERSION == 1.6.0`.
- `VERSION` 1.5.0 → 1.6.0.
- `README.md` Version banner e URLs apontam para `v1.6.0`.

### ABI

- `capy-ui-widget` **1.5 → 1.6** (aditivo: 4 event types novos no tail do enum, 1 struct nova, 5 widget tail fields, 5 APIs públicas). Nenhum struct existente reordenado; nenhum campo removido.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema permanece **5** (gesture/DnD não emitem ops novas neste minor).

### Notas de design

- **Zero alloc no core.** Payload e filter array são caller-owned; CapyUI nunca copia, aloca nem libera. Lifetime é responsabilidade do host.
- **DRAG_BEGIN/MOVE/END como no-ops no core.** O host dispatcher carrega o stado completo de drag (cursor, preview, cancel via ESC). O core só age no DROP terminal. Isso mantém o widget core determinístico sem precisar de stado persistente para a operação de drag.
- **Roteamento deepest-first.** Mesma estratégia do POINTER_DOWN — children primeiro, then self. Preserva a expectativa do usuário (drop em um item dentro de um painel vai para o item, não para o painel).
- **Pattern language minimalista.** `"*"`/`"prefix/*"`/exact (case-insensitive ASCII). Sem char classes, sem regex, sem suffix matching. Mantém o matcher pequeno e determinístico.
- **Preview no display-list adiado.** O plano original previa preview translúcido emitido durante drag pelo widget source. Adiei para uma fatia futura aditiva que pode introduzir `CAPY_DL_DRAG_PREVIEW` (DL schema 5 → ?) sem quebrar nada existente. Em 1.6 o host renderiza preview através do compositor.
- **Cancel via ESC adiado.** Mesma justificativa: o ciclo de cancel é do dispatcher. Quando o host quiser cancelar, simplesmente não emite DROP após DRAG_END.

### Docs

- `docs/compatibility.md` ganhou bloco "Drag and drop (since 1.6)"; ABI line bumpada para `v1.6`; cobertura inclui DnD.
- `docs/roadmap/long-term/v1.6-drag-and-drop.md` status = delivered (modelo de dados + roteamento DROP; preview e cancel deferidos).
- `docs/roadmap/contracts/abi-versions.md` linha 1.6 promovida para **atual**, 1.5 demovida; tag `v1.6.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.6.0 / `capy-ui-widget 1.6` / DL schema 5 / **140 testes** / longo prazo agora com 6/17 entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 1.6**.

## [1.5.0] — 2026-05-21

**Marco:** quinto minor pós-freeze. Suporte a multi-display e DPI scaling com o **primeiro bump real do display-list schema desde 0.9** (`4 → 5`). A 1.5 é aditiva sobre o baseline congelado e preserva byte-equivalência do emit para qualquer caller que mantenha o scale default (`256` = 1.0×).

### Adicionado

- Macros: `CAPYUI_API_VERSION_MAJOR/_MINOR/_PATCH` agora `1/5/0`.
- `capy_widget_context.dpi_scale_x256` (tail field `uint16_t` + `reserved_dpi`): Q8.8 DPI scale, `256` = 1.0×, `384` = 1.5×, `512` = 2.0×. Seeded para `256` por `capy_widget_context_init` para preservar comportamento pre-1.5.
- `capy_display_list.dpi_scale_x256` (tail field `uint16_t` + `reserved_dpi`): mesmo formato; seeded para `256` por `capy_display_list_init`. O host mirrora o valor da context na display-list antes de `capy_widget_emit` quando quiser ativar o scoping.
- 3 APIs novas em `src/widget/capy_widget.h` + `capy_widget.c`:
  - `void capy_widget_set_dpi_scale(ctx, scale_x256)` — clampa `0 → 1` (nunca zero); NULL é no-op.
  - `uint16_t capy_widget_get_dpi_scale(const ctx)` — retorna o valor armazenado; NULL e contextos zero-init retornam `256`.
  - `int32_t capy_widget_dpi_scale_dim(scale_x256, value)` — helper puro com `int64_t` intermediários, truncate-toward-zero, saturating clamp a `INT32_MAX/MIN`; identidade quando `scale == 256`; clampa scale `0 → 1`.
- Op aditiva `CAPY_DL_DPI_SCOPE` em `capy_dl_op` (`src/widget/capy_display_list.h`):
  - `rect` = região escopada (root bounds em 1.5; futuras minors podem emitir per-subtree para multi-display).
  - `image_id` = `dpi_scale_x256` (reuso aditivo do slot 32-bit existente — sizeof `capy_dl_cmd` inalterado).
- `CAPY_DISPLAY_LIST_SCHEMA_VERSION` bumpado de `4` para `5`.
- `capy_widget_emit` prepende um `CAPY_DL_DPI_SCOPE` op no início do display-list **quando `dl->dpi_scale_x256 != 256`**. Quando o scale é o default 256, nenhum op novo é emitido — preserva byte-equivalência com pre-1.5 para o mesmo widget tree.
- GPU translator (`capy_dl_to_quads`, since 1.2) ganha case explícito para `CAPY_DL_DPI_SCOPE`: skip (sem emit de quad); host backend lê o op separadamente para escolher rasterizer/viewport.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_dpi_default_is_256` — context inicializa com scale 256.
  2. `test_dpi_set_get_roundtrip` — set 384/512 roundtrip; zero clampa para 1; NULL guards.
  3. `test_dpi_scale_dim_helper` — identidade a 1×, dobro a 2×, truncate toward zero a 1.5× (positivo e negativo); saturating clamp em `INT32_MAX/MIN`.
  4. `test_dpi_scope_op_emitted_when_non_default` — `dl.dpi_scale_x256 = 384` → primeiro op é DPI_SCOPE com `image_id == 384` e `rect == root.bounds`; segundo op é CLIP_PUSH usual.
  5. `test_dpi_scope_op_omitted_at_default` — `dl.dpi_scale_x256 == 256` → nenhum DPI_SCOPE no display-list; primeiro op é CLIP_PUSH (byte-equivalente a pre-1.5).
  6. `test_dpi_independent_contexts` — dois contextos com scales diferentes mantêm estado independente.
  7. `test_dpi_gpu_translator_skips_dpi_scope` — `capy_dl_to_quads` consome DPI_SCOPE + RECT sintetizado e emite exatamente 1 quad (para o RECT).
- Atualização de 3 assertions schema-4 → schema-5 nos testes existentes (`test_displaylist_emits_font_id` e o freeze marker).

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 4 → 5 (`MAJOR=1`, `PATCH=0`).
- `Makefile` banner para `1.5.0 (alpha.257) — ABI 1.x (quinto minor pós-freeze)`. `lint` e `version-check` exigem `VERSION == 1.5.0`.
- `VERSION` 1.4.0 → 1.5.0.
- `README.md` Version banner e URLs apontam para `v1.5.0`.

### ABI

- `capy-ui-widget` **1.4 → 1.5** (aditivo: 2 tail fields, 3 APIs novas, 1 op novo no enum, 1 schema bump). Nenhum struct existente reordenado; nenhum campo removido; `sizeof(capy_dl_cmd)` inalterado (reuso de `image_id` para o scale).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **4 → 5**. Primeiro bump real desde 0.9. Consumers que leem `dl->version < 5` devem ignorar o novo op `CAPY_DL_DPI_SCOPE` ao caminhar produtores mais novos.

### Notas de design

- **Byte-equivalência preservada a `scale=256`.** Existing 125 testes passam sem mudança comportamental porque emit não introduz DPI_SCOPE quando o scale é o default. Apenas 3 assertions de schema literal precisaram bumpar.
- **Layout auto-scale deferido.** O slice doc original previa que `measure`/`arrange` escalassem `padding`/`gap`/`min_w`/`max_w` automaticamente. Adiei porque (a) tocaria capy_widget_measure (já complexo) e correria risco de quebrar diff/incremental tests; (b) a invariante de byte-equivalência foi privilegiada para esta minor. Hosts que querem scaling agora podem aplicar `capy_widget_dpi_scale_dim` aos próprios constants antes de criar widgets — totalmente determinístico.
- **Reuso aditivo de `image_id` para o scale.** Em vez de aumentar `sizeof(capy_dl_cmd)` adicionando um novo campo `scale_x256` (que quebraria layout para callers que stack-allocate arrays), reuso o slot 32-bit existente. Documentação em `capy_display_list.h` explica que esse slot tem semântica polimórfica por op.
- **Field `reserved_dpi` no display-list.** Forward-compat: uma minor futura pode introduzir `font_size_scale_x256` separado do `dim_scale` sem outro bump de schema (apenas semântica diferente para o reserved slot).
- **Schema bump cross-repo.** Esta é a primeira mudança que mexe no display-list schema desde 0.9; o `cross-repo-sync-pending.md` foi atualizado para destacar isso no audit CapyOS (a matrix de compatibilidade do CapyOS precisa mencionar schema 4 → 5 explicitamente; smokes que decodificam DL precisam tolerar ops 0..8 incluindo DPI_SCOPE como skip).

### Docs

- `docs/compatibility.md` ganhou bloco "Multi-display & DPI scaling (since 1.5)"; ABI line bumpada para `v1.5`; lista de coberturas inclui o módulo DPI; schema citado como `v5`.
- `docs/roadmap/long-term/v1.5-multi-display-dpi.md` status = delivered (escopo conservador); testes catalogados; layout auto-scale documentado como deferido.
- `docs/roadmap/contracts/abi-versions.md` linha 1.5 promovida para **atual**, 1.4 demovida; tag `v1.5.0` adicionada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.5.0 / `capy-ui-widget 1.5` / DL schema **5** / **132 testes** / longo prazo agora com 5/17 entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado com bloco **since 1.5** + alerta explícito sobre o schema bump.

## [1.4.0] — 2026-05-21

**Marco:** quarto minor pós-freeze. Recognizer de gestos determinístico single-touch sobre o touch payload introduzido em 0.10, mantendo zero float e zero malloc.

### Adicionado

- `enum capy_gesture_kind` em `src/widget/capy_widget.h` com 13 entradas: `NONE=0`, `TAP`, `DOUBLE_TAP`, `LONG_PRESS`, `SWIPE_LEFT/RIGHT/UP/DOWN`, `PINCH_IN/OUT` (reservados, não emitidos em 1.4), `ROTATE_CW/CCW` (reservados, não emitidos em 1.4), `DRAG`. Multi-touch (PINCH/ROTATE) ficará para fatia futura aditiva sem alterar a numeração.
- `struct capy_gesture { uint8_t kind; uint8_t reserved[3]; capy_ui_point start, end; int32_t magnitude; uint32_t duration_ms; }` — payload de saída. `magnitude` é o delta do eixo dominante para SWIPE/DRAG; permanece reservado para PINCH (percent) e ROTATE (deg×256).
- `struct capy_gesture_recognizer` (≈80 bytes, caller-embedded; sem malloc) com thresholds caller-tunáveis e queue de saída depth-1.
- 4 APIs públicas em `src/widget/capy_widget.c`:
  - `capy_gesture_recognizer_init(r)` — seeds thresholds default (tap 200 ms / 10 px, dbl-tap gap 300 ms, long-press 500 ms, swipe 50 px) e zera estado.
  - `capy_gesture_feed(r, ev, now)` — consome TOUCH_BEGIN/MOVE/END (outros tipos retornam 0); detecta DRAG no MOVE (uma vez por touch); detecta TAP/DOUBLE_TAP/SWIPE no END. Retorna `1` se emitiu, `0` se só atualizou estado, `-1` em NULL. Segunda TOUCH_BEGIN concorrente com `touch_id` diferente é ignorada.
  - `capy_gesture_tick(r, now)` — emite LONG_PRESS quando `now - start_tick >= long_press_min_ms` e o dedo não saiu de `tap_max_distance_px`. Retorna `1` no emit, `0` caso contrário, `-1` em NULL.
  - `capy_gesture_poll(r, &out)` — drena a queue depth-1. Retorna `1` e copia para `out` no sucesso, `0` se vazio, `-1` em NULL.
- Distância calculada via Chebyshev (`max(|dx|, |dy|)`) para evitar `sqrt` e qualquer float — mantém a invariante "zero float em `src/widget/`".
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_gesture_tap_emits_once` — BEGIN→END em 80 ms sem movimento emite TAP; poll subsequente retorna 0.
  2. `test_gesture_double_tap_within_gap` — dois taps com 150 ms de gap emitem TAP + DOUBLE_TAP em ordem.
  3. `test_gesture_long_press_via_tick` — BEGIN seguido de `tick(600 ms)` emite LONG_PRESS; release subsequente não emite TAP.
  4. `test_gesture_swipe_four_directions` — table-driven (RIGHT/LEFT/DOWN/UP) com magnitude 100; verifica o eixo dominante.
  5. `test_gesture_drag_emitted_at_threshold_cross` — pequeno move (no emit) + grande move (DRAG) + END (sem SWIPE).
  6. `test_gesture_determinism` — dois recognizers consumindo a mesma sequência produzem o mesmo emit byte-a-byte.
  7. `test_gesture_ignores_non_touch_and_extra_touch_ids` — POINTER/KEY/WHEEL/GAMEPAD ignorados; NULL guards retornam -1; segunda TOUCH_BEGIN com touch_id diferente é ignorada sem corromper o estado.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 3 → 4 (`MAJOR=1`, `PATCH=0`).
- `Makefile` banner para `1.4.0 (alpha.256) — ABI 1.x (quarto minor pós-freeze)`. `lint` e `version-check` exigem `VERSION == 1.4.0`.
- `VERSION` 1.3.0 → 1.4.0.
- `README.md` Version banner e URLs apontam para `v1.4.0`.

### ABI

- `capy-ui-widget` **1.3 → 1.4** (aditivo: 1 enum novo, 2 structs novas, 4 APIs novas). Estruturas existentes (`capy_widget`, `capy_widget_event`, `capy_touch_payload`, `capy_ui_point`, etc.) sem mudança.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** (gesture engine não emite no display-list).

### Notas de design

- **Single-touch apenas.** O plano original listava PINCH/ROTATE como obrigatórios; deferi para uma fatia futura porque exigem tracking de múltiplos `touch_id`s simultâneos (estrutura adicional, política de cancelamento, semântica de ângulo). Os valores ficam reservados no enum para que código que faça `switch` sobre `capy_gesture_kind` não quebre no dia em que começarem a emit.
- **Concrete struct em vez de opaco.** O cabeçalho original sugeria `struct capy_gesture_recognizer; /* opaco */` com buffer caller-provided. Escolhi struct concreta caller-embedded — alinhado com o resto do projeto (`capy_widget_pool`, `capy_anim`, `capy_spring`, etc.) e elimina a indireção. Tamanho ≈ 80 bytes, estável dentro do 1.x.
- **`now` no feed.** `capy_widget_event` (since 0.0) não carrega timestamp; o host fornece o tick monotônico ms via segundo parâmetro de `capy_gesture_feed`. Mesma convenção de `capy_anim_sample`.
- **`tick(now)` separado.** Necessário porque LONG_PRESS é definido por *tempo passado sem release*, sem novo evento. O host pode chamar `tick` no mesmo loop que `capy_widget_tick` (since 0.5).
- **Queue depth 1.** Um novo emit sobrescreve qualquer pendente. Aceitável porque o uso típico chama `poll` por frame; documentado.
- **Cross-repo:** o documento `tracking/cross-repo-sync-pending.md` foi atualizado para incluir a linha v1.4 nas ações de matrix CapyOS, integration contract e audit.

### Docs

- `docs/compatibility.md` ganhou bloco "Gesture engine (since 1.4)"; ABI line bumpada para `v1.4`; lista de coberturas inclui o módulo de gesture engine.
- `docs/roadmap/long-term/v1.4-gesture-engine.md` status = delivered (CapyUI side, single-touch); APIs/testes catalogados; notas explícitas sobre multi-touch deferido e mudança da assinatura de `feed` para incluir `now`.
- `docs/roadmap/contracts/abi-versions.md` linha 1.4 promovida para **atual**, 1.3 demovida para `tagged`; tag `v1.4.0` adicionada como `feature (aditivo, pós-freeze)`.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.4.0 / `capy-ui-widget 1.4` / DL schema 4 / **125 testes** / longo prazo agora com 4/17 entregues.
- `docs/roadmap/tracking/cross-repo-sync-pending.md` atualizado para refletir v1.4 nas linhas de matrix CapyOS, integration contract e audit consolidada.

## [1.3.0] — 2026-05-21

**Marco:** terceiro minor pós-freeze. Animação rica com keyframes, spring physics em fixed-point e curvas Bézier cúbicas inteiras — tudo respeitando a invariante de zero float no widget core.

### Adicionado

- `struct capy_anim_keyframe { uint32_t tick; int32_t value; uint8_t easing; uint8_t reserved; uint16_t reserved2; }` em `src/widget/capy_widget.h` — ponto numa timeline. `easing` reusa o enum `capy_anim_easing` (LINEAR, EASE_IN, EASE_OUT, EASE_IN_OUT) introduzido na 0.5.
- `struct capy_anim_track { capy_anim_keyframe *keyframes; uint32_t count, capacity; int32_t loop; }` — caller-owned array de keyframes ordenado por tick. Policy de loop: `0` = play once com clamp, `-1` = infinito (wrap por duração), `N>0` = repete N vezes e depois clampa para o último valor.
- `struct capy_spring { int32_t target, velocity, position; uint16_t stiffness, damping; }` — physics state. `velocity` em Q24.8, `stiffness`/`damping` em Q8.8, `position`/`target` em unidades plain int32.
- API pública `int capy_anim_track_sample(const struct capy_anim_track *track, uint32_t now, int32_t *out)` em `src/widget/capy_widget.c`:
  - Aplica loop policy → encontra segmento `[kf[i], kf[i+1]]` via linear search → calcula `t_q16 = seg_pos * 65536 / seg_len` → aplica easing da kf anterior reutilizando `capy_anim_apply_easing` (existente desde 0.5) → interpola com `capy_anim_interp`.
  - Retorna `0` em sucesso, `-1` em NULL args / `count == 0`.
- API pública `void capy_spring_step(struct capy_spring *s, uint32_t dt_ms)`:
  - 1 ms symplectic-Euler substeps. Per substep: `accel = (stiffness × disp)/256 - (damping × velocity)/65536`; `velocity += accel × 256`; `position += velocity/256`.
  - Saturating clamps a int32 para velocity e position.
  - Com `damping > 0` converge; com `damping == 0` oscila indefinidamente.
- API pública `int32_t capy_anim_bezier_eval(int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t t_q16)`:
  - De Casteljau 1D com t em Q16.16 (clampado em `[0, 0x10000]`). Endpoints exatos.
  - Monotônico para pontos de controle monotonicamente ordenados.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_anim_track_sample_keyframes_ordered` — 3 keyframes lineares; sample em t=0/50/100/150/1000; NULL guards.
  2. `test_anim_track_loop_infinite` — loop=-1 com período 100; t=50/150/250 equivalentes; t=100 wraps para 0.
  3. `test_anim_track_loop_finite_clamps` — loop=2; mid-loops batem; t=2×period clampa para last value.
  4. `test_anim_spring_converges_with_damping` — stiffness=64, damping=128, 500 substeps → |position - target| < 50.
  5. `test_anim_spring_oscillates_undamped` — damping=0, 200 substeps → posição overshoota e volta abaixo (oscila).
  6. `test_anim_bezier_monotonic` — 17 amostras monotônicas; endpoints exatos; t out-of-range clampa.
  7. `test_anim_track_determinism` — mesma `(now, track)` 2× → outputs idênticos sobre 20 valores de `now`.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 2 → 3 (`MAJOR=1`, `PATCH=0`).
- `Makefile` banner para `1.3.0 (alpha.255) — ABI 1.x (terceiro minor pós-freeze)`. `lint` e `version-check` exigem `VERSION == 1.3.0`.
- `VERSION` 1.2.0 → 1.3.0.
- `README.md` Version banner e URLs apontam para `v1.3.0`.

### ABI

- `capy-ui-widget` **1.2 → 1.3** (aditivo: 3 structs novas, 3 APIs novas). Estruturas existentes (`capy_widget`, `capy_widget_context`, `capy_anim`, etc.) sem mudança.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** (rich animation opera no modelo retido, não no display-list).

### Notas de design

- **Zero float, strict C11.** Spring e Bézier usam apenas `int64_t` intermediários. Todos os `>>` em valores signed foram substituídos por `/` e `*`: right-shift em negativo signed é impl-defined per C11 §6.5.7; left-shift de negativo signed é UB. Truncate-toward-zero division mantém o integrador determinístico entre compiladores.
- **`capy_widget_animate_property` adiado.** O plano original listava esse helper de alto nível como API. Implementá-lo agora requer slot de animação por widget integrado com `capy_widget_tick` e binding de property id (POS_X, POS_Y, WIDTH, HEIGHT, etc.). Fica para fatia futura aditiva.
- **DL schema 5 (do plano original) também adiado.** O cabeçalho do slice listava schema 5 mas rich animation não introduz nova op em `capy_dl_op`; o schema só bumpa quando o enum cresce. Mesma decisão da v1.2.

### Docs

- `docs/compatibility.md` ganhou bloco "Rich animation (since 1.3)"; ABI line bumpada para `v1.3`; lista de coberturas inclui o módulo de animação rica.
- `docs/roadmap/long-term/v1.3-rich-animation.md` status = delivered (CapyUI side); APIs/testes catalogados; notas sobre `animate_property` adiado e DL schema permanecer 4.
- `docs/roadmap/contracts/abi-versions.md` linha 1.3 promovida para **atual**, 1.2 demovida para `tagged`; tag `v1.3.0` adicionada como `feature (aditivo, pós-freeze)`.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.3.0 / `capy-ui-widget 1.3` / DL schema 4 / **118 testes** / longo prazo agora com 3/17 entregues.

## [1.2.0] — 2026-05-21

**Marco:** segundo minor pós-freeze. Novo módulo opcional `capy_dl_gpu` traduz display-list em quads para qualquer backend gráfico que o host CapyOS escolher (CPU rasterizer, Mesa, lavapipe, VirGL, OpenGL, Vulkan).

### Adicionado

- Novo header `src/widget/capy_dl_gpu.h` definindo:
  - `struct capy_gpu_quad { int32_t x0, y0, x1, y1; uint32_t color; uint32_t texture_id; int32_t uv_x0, uv_y0, uv_x1, uv_y1; }` — quad axis-aligned com cor sólida (`texture_id == 0`) ou textura indexada (`texture_id == cmd->image_id`) e UVs em `int32_t` para o backend normalizar.
  - Macro `CAPY_DL_GPU_CLIP_STACK_MAX = 16u` — limite interno do scissor stack.
  - API pública `int capy_dl_to_quads(const struct capy_display_list *dl, struct capy_gpu_quad *out, uint32_t cap, uint32_t *out_count)`.
- Novo source `src/widget/capy_dl_gpu.c` com a implementação:
  - Walk de `dl->cmds[]` decompondo ops em quads.
  - `CAPY_DL_RECT` → 1 quad. `CAPY_DL_BORDER` e `CAPY_DL_FOCUS_RING` → 4 quads (top/bottom/left/right strips com `cmd->border_width`). `CAPY_DL_TEXT` → 1 quad placeholder (texture_id=0; host re-traduz com font atlas). `CAPY_DL_IMAGE_REF` → 1 quad com `texture_id = cmd->image_id` e UVs (0,0)-(rect.w, rect.h).
  - `CAPY_DL_CLIP_PUSH`/`POP` → scissor stack interno. Cada push intersecta com o frame pai (zero-area collapse silencia os filhos). Cada quad emitido é intersectado contra o top do stack; empty intersections são droppadas.
  - `CAPY_DL_DIRTY_HINT` e `CAPY_DL_NONE` → ignorados.
  - Ops desconhecidas (fora do enum schema 4) → `-1` (fail-closed).
  - Stack unbalanced (CLIP_POP sem push, ou CLIP_PUSH ainda na stack ao final do walk) → `-1`.
  - Overflow de `cap` → `-1` com `*out_count` parcial (sem rollback). NULL `dl`/`out_count`/(`out` com `cap > 0`) → `-1`.
- `capy_dl_gpu.c` adicionado a `SRC_WIDGET` no `Makefile` — `make validate`/`security` agora compilam o novo source com as mesmas hardening flags do widget core (`-Wall -Wextra -Werror -pedantic -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE`).
- 7 novos testes em `tests/test_widget_contracts.c` (inclusão `#include "capy_dl_gpu.h"` adicionada no topo do arquivo):
  1. `test_gpu_rect_to_single_quad` — RECT (10,20,30,40,0xFFAABBCC) → 1 quad (10,20,40,60) com a cor.
  2. `test_gpu_border_to_four_quads` — BORDER 50×30 border_width=2 → 4 quads (top 50×2, bottom 50×2 a y=28, left 2×26 a y=2, right 2×26 a x=48).
  3. `test_gpu_clip_intersects_quads` — CLIP_PUSH (10,10,30,30) + RECT (0,0,100,100) → quad (10,10,40,40); pós-pop, novo RECT emite unclipped.
  4. `test_gpu_overflow_returns_minus_one` — cap=1 com 2 RECTs → -1, `*out_count = 1`; NULL guards (dl, out_count, out-com-cap>0).
  5. `test_gpu_unbalanced_clip_rejected` — CLIP_POP isolado → -1; CLIP_PUSH sem pop → -1.
  6. `test_gpu_image_ref_carries_texture_and_uvs` — IMAGE_REF (5,7,32,32,image_id=9001) → texture_id=9001, UVs (0,0)-(32,32).
  7. `test_gpu_translator_determinism` — panel+button real → emit → traduzir duas vezes → quads byte-idênticos campo-a-campo.

### Mudado

- Macros `CAPYUI_API_VERSION_MINOR` 1 → 2 (`CAPYUI_API_VERSION_MAJOR` permanece 1; `_PATCH` permanece 0).
- `Makefile` banner para `1.2.0 (alpha.254) — ABI 1.x (segundo minor pós-freeze)`. `lint` e `version-check` exigem `VERSION == 1.2.0`.
- `VERSION` 1.1.0 → 1.2.0.
- `README.md` Version banner 1.1.0 → 1.2.0; URLs de exemplo apontam para `v1.2.0`.

### ABI

- `capy-ui-widget` **1.1 → 1.2** (aditivo: 1 header novo, 1 source novo, 1 struct nova, 1 macro nova, 1 API nova). Sem mudança em estruturas existentes (`capy_widget`, `capy_widget_context`, `capy_dl_cmd`, etc.).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4**. O bump 4 → 5 do plano original (`v1.2-gpu-path-optional.md`) foi deliberadamente diferido — o tradutor emite quads em um buffer separado (`capy_gpu_quad[]`), não introduz nova op de DL. Schema só será bumpado quando uma op realmente nova for adicionada ao enum `capy_dl_op`.

### Notas de design

- **Por que não bumpar DL schema 4 → 5?** O contrato do display-list só bumpa schema quando o enum `capy_dl_op` ganha um valor novo. O tradutor v1.2 lê os ops existentes (schema 4) e produz uma sequência paralela de `capy_gpu_quad[]`. Não há op nova em DL.
- **Scissor stack bounded em 16.** Hosts que precisem de profundidade maior podem aumentar `CAPY_DL_GPU_CLIP_STACK_MAX` em fork, mas a árvore CapyUI raramente passa de 4-5 níveis de clip em prática (taskbar > popup > scroll-area > content).
- **TEXT vira placeholder.** CapyUI não tem font atlas; o host precisa re-traduzir o quad em N quads de glifo (1 por caractere) usando `font_id`+`text_offset`+`text_len` da `capy_dl_cmd` original. A v1.2 só estabelece o contrato de superfície.
- **CapyUI não toca GPU API.** Zero referência a OpenGL/Vulkan/Metal/DirectX. O módulo é matemática inteira pura sobre quads — qualquer backend gráfico do host consome o buffer.

### Docs

- `docs/compatibility.md` ganhou bloco "GPU translator (since 1.2)"; ABI line bumpada para `v1.2`; lista de coberturas inclui o tradutor opcional.
- `docs/roadmap/long-term/v1.2-gpu-path-optional.md` status = delivered (CapyUI side); APIs e testes catalogados; nota explícita sobre schema permanecer 4.
- `docs/roadmap/contracts/abi-versions.md` linha 1.2 promovida para **atual**, 1.1 demovida para `tagged`; tag `v1.2.0` adicionada como `feature (aditivo, pós-freeze)`.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.2.0 / `capy-ui-widget 1.2` / DL schema 4 / **111 testes** / longo prazo agora com 2/17 entregues.

## [1.1.0] — 2026-05-21

**Marco:** primeiro minor pós-freeze. ABI `capy-ui-widget` evolui aditivamente: damage tracking otimizado (`dirty_version` em widget + 4 APIs novas) e frame budget hint no contexto.

### Adicionado

- Campo aditivo `uint32_t dirty_version` no tail de `struct capy_widget` (since 1.1). Monotônico; bumpado por `capy_widget_invalidate` (existente desde 0.15) e por `capy_widget_invalidate_subtree` (novo). Overflow wraps silently — hosts amostram delta entre snapshots, não valor absoluto.
- Campo aditivo `uint32_t frame_budget_microseconds` no tail de `struct capy_widget_context` (since 1.1). Seeded em 0 pelo `capy_widget_context_init`.
- API pública `void capy_widget_invalidate_subtree(struct capy_widget *widget)` em `src/widget/capy_widget.c` — recursão pré-order que seta `layout_dirty = 1` e bumpa `dirty_version` em `widget` e em todos os descendentes. **Não** toca os ancestrais (diferente de `capy_widget_invalidate`, que walks-up). NULL é no-op.
- API pública `uint32_t capy_widget_dirty_version(const struct capy_widget *widget)` — accessor `const`-safe; NULL → 0.
- API pública `void capy_widget_frame_budget(struct capy_widget_context *ctx, uint32_t microseconds)` — setter do hint no contexto; NULL é no-op. CapyUI **não** enforce o budget — apenas armazena para que hosts queryem `ctx->frame_budget_microseconds` e degradem coopereativamente em loops próprios.
- API pública `int capy_display_list_diff_incremental(const struct capy_display_list *prev, const struct capy_display_list *next, struct capy_ui_rect *out_dirty, uint32_t out_cap)` em `src/widget/capy_display_list.c` — equivalente semântico de `capy_widget_diff` (since 0.8) com otimização de **leading-prefix skip**: enquanto `prev[i]` e `next[i]` forem byte-iguais, pula. A partir do primeiro índice divergente, roda o diff posicional idêntico ao `capy_widget_diff`. Saída byte-idêntica.
- Helper estático `capy_widget_invalidate_subtree_recurse` em `capy_widget.c`.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_invalidate_bumps_dirty_version` — invalidate(child) bumpa `dirty_version` em child + root; layout_dirty=1 preservado.
  2. `test_invalidate_subtree_walks_down` — invalidate_subtree(middle) bumpa middle + 2 leaves; root NÃO tocado.
  3. `test_dirty_version_accessor` — 0 → 1 → 2 após sucessivos invalidates; NULL → 0.
  4. `test_frame_budget_round_trip` — 0 default; set/clear; NULL guard.
  5. `test_diff_incremental_matches_diff_identical` — prev=next → 0 rects em ambas as funções.
  6. `test_diff_incremental_matches_diff_trailing_change` — sequência byte-idêntica ao `capy_widget_diff` quando só cor de bg muda.
  7. `test_diff_incremental_overflow_returns_minus_one` — cap=1 com 2 rects diferentes → -1; NULL out com cap>0 → -1; ambos NULL → 0.

### Mudado

- `capy_widget_invalidate` (existente desde 0.15) agora **também** bumpa `dirty_version` em cada widget que visita — comportamento aditivo, não quebra callers existentes (continuam observando `layout_dirty=1` propagado).
- Macros `CAPYUI_API_VERSION_MINOR` 0 → 1 (`CAPYUI_API_VERSION_MAJOR` permanece 1; `CAPYUI_API_VERSION_PATCH` permanece 0).
- `Makefile` banner para `1.1.0 (alpha.253) — ABI 1.x (first post-freeze aditivo)`. `lint` e `version-check` exigem `VERSION == 1.1.0`.
- `VERSION` 1.0.0 → 1.1.0.
- `README.md` Version banner 1.0.0 → 1.1.0; URLs de exemplo apontam para `v1.1.0`.

### ABI

- `capy-ui-widget` **1.0 → 1.1** (aditivo: 1 campo no tail de `capy_widget`, 1 campo no tail de `capy_widget_context`, 4 APIs novas, semântica de `capy_widget_invalidate` estendida).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4**.

### Notas de design

- **Por que prefix-skip e não suffix-skip?** `capy_widget_diff` é posicional (pareia `prev[i]` com `next[i]`) e emite os rects trailing incondicionalmente quando as listas têm tamanhos diferentes. Um suffix skip re-alinharia prev/next e descartaria esses rects, gerando saída diferente. Para preservar a equivalência byte-identical declarada no contrato, apenas a prefix-skip é aplicada.
- **Frame budget é hint, não enforcement.** Adicionar enforcement (callbacks, throttling, log) exigiria infraestrutura de logging no portable core ou hooks de host adicionais — desnecessário para a 1.1; pode chegar como API additivamente em minor futuro.
- **`emit` ainda não consulta `dirty_version`.** A infraestrutura (campo + acessor) está disponível para uma fatia futura de emit caching sem novo ABI bump.

### Docs

- `docs/compatibility.md` ganhou bloco "Damage tracking (since 1.1)"; ABI line bumpada para `v1.1`; lista de coberturas inclui damage tracking; cabeçalho marca `v1.1` como aditivo sobre o baseline 1.0 congelado.
- `docs/roadmap/long-term/v1.1-damage-tracking.md` status = delivered (CapyUI side); APIs e testes catalogados; nota explícita sobre frame-budget como hint e prefix-only skip.
- `docs/roadmap/contracts/abi-versions.md` linha 1.1 promovida para **atual**, 1.0 marcada como `tagged (CONGELADO, baseline)`; tag `v1.1.0` adicionada com tipo `feature (aditivo, pós-freeze)`.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.1.0 / `capy-ui-widget 1.1` / DL schema 4 / **104 testes** / longo prazo iniciado (1/17 entregue).

## [1.0.0] — 2026-05-21 — **CONGELAMENTO DA ABI**

**Marco:** `capy-ui-widget` é frozen no major `1`. A política de deprecation formal entra em vigor (`docs/roadmap/contracts/deprecation-policy.md`). Não há código de feature novo nesta release — apenas markers de freeze, política de segurança expandida, atualização da matriz de compatibilidade e consolidação do surface.

### Adicionado

- Macros públicos em `src/widget/capy_widget.h`:
  - `CAPYUI_API_VERSION_MAJOR = 1`, `CAPYUI_API_VERSION_MINOR = 0`, `CAPYUI_API_VERSION_PATCH = 0` — espelham `VERSION`/`Makefile`. Consumidores podem gate em `CAPYUI_API_VERSION_MAJOR >= 1` para saber que o ABI estável está disponível.
  - `CAPYUI_API_DEPRECATED(msg)` — expande para `__attribute__((deprecated(msg)))` em GCC/Clang, `__declspec(deprecated(msg))` em MSVC, vazio nos demais. Nenhum símbolo anotado em 1.0; macro publicada para que call sites possam usá-la sem `#ifdef` quando deprecation chegar (mínimo 2 minors antes de remoção em major).
- Bloco "ABI 1.0 freeze marker" em `capy_widget.h` documentando que `0.0`–`0.15` (com `0.7` e `0.12` reservadas) compõem o surface frozen 1.x.
- `tests/test_widget_contracts.c`:
  - Helper `capyui_v1_freeze_canary` (canary marcado `CAPYUI_API_DEPRECATED`) — exercita o atributo em runtime sem poluir o surface público.
  - `test_v1_freeze_markers` — confirma `CAPYUI_API_VERSION_*`, expansão de `CAPYUI_API_DEPRECATED`, e a presença de uma âncora simbólica por slice de 0.0 a 0.15 (BUTTON, LAYOUT_STACK, DL_RECT, schema=4, KEY_TAB, MOD_SHIFT, MOD_CAPSLOCK, ANIM_EASE_IN_OUT, TOKEN_COUNT, THEME_FORMAT_VERSION=2, A11Y_FOCUSED, PLURAL_OTHER, EVENT_GAMEPAD). Diagnóstico `-Wdeprecated-declarations` é suprimido em GCC/Clang ao redor da chamada do canary para passar `-Werror`.

### Mudado

- `SECURITY.md` reescrita: política de divulgação responsável (90d coordinated, exceção security para janela compacta), versionamento suportado (`1.x` recebe security patches por **≥12 meses** pós-`2.0`), release gate (determinismo byte-a-byte tratado como invariante de segurança, zero float em `src/widget/`, zero include CapyOS), hardening flags, supply chain (Ed25519 signer pinned NULL durante alpha streaming-buffer window).
- `Makefile` banner para `1.0.0 (alpha.252) — ABI FROZEN`. `lint` e `version-check` exigem `VERSION == 1.0.0`.
- `VERSION` 0.15.0 → 1.0.0.
- `README.md` Version banner 0.15.0 → 1.0.0 e nota explícita sobre o freeze; URLs de exemplo apontam para `v1.0.0`.
- `docs/compatibility.md`: ABI `capy-ui-widget` marcado como **`v1.0` (frozen, LTS)** na tabela de Owned ABIs; bloco "covers" renomeado para "covers (frozen surface)" e inclui as macros de freeze.

### ABI

- `capy-ui-widget` **0.15 → 1.0 (CONGELADO)**. Sem remoções ou renomeações; o surface é a união das 14 minors aditivas 0.0–0.15 (com 0.7 e 0.12 reservadas).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4**.

### Docs

- `docs/roadmap/medium-term/v1.0-abi-stability.md` status = delivered (CapyUI side); gates externos (CI 30d, soak 1000×, audit cross-repo, tag assinada, comunicado público) destacados como follow-up operacional do release pipeline.
- `docs/roadmap/contracts/abi-versions.md`: linha 1.0 promovida para **`CONGELADO (atual)`**; 0.15 demovida para `tagged`; tag `v1.0.0` adicionada com tipo `freeze` na timeline.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 1.0.0 / `capy-ui-widget 1.0` (frozen) / DL schema 4 / **97 testes** / fase média marcada COMPLETA.

### Guia de migração 0.x → 1.x

**Não é necessária migração de código.** Toda surface 0.x continua válida em 1.0:

- Header includes: `#include "capy_widget.h"`, `#include "capy_display_list.h"`, `#include "capy_layout.h"` permanecem.
- Toda API pública entregue entre `0.0.1` e `0.15.0` continua disponível com a mesma assinatura e semântica.
- Todos os 17 theme tokens, 8 ops do display-list, 5 plural rules, 8 a11y flags, 6 modifier flags, 11 event types, 4 layout kinds, 4 anim easings, 13 widget types permanecem no surface 1.0.
- `CAPY_DISPLAY_LIST_SCHEMA_VERSION` permanece **4**; consumidores que aceitam versões maiores e ignoram ops desconhecidas continuam compatíveis.
- `CAPY_THEME_FORMAT_VERSION` permanece **2**; arquivos `theme.conf` produzidos em 0.14/0.15 são lidos em 1.0 sem mudança.

A novidade é opt-in: consumidores podem agora gate em `CAPYUI_API_VERSION_MAJOR >= 1` ou anotar futuras APIs com `CAPYUI_API_DEPRECATED(...)` quando suas próprias minor releases as obsoletarem.

### Política pós-1.0 (em vigor desde aqui)

- Mudanças quebradoras requerem ADR em `docs/roadmap/tracking/DECISIONS.md` + major bump (`2.0.0`).
- Deprecation: marcar com `CAPYUI_API_DEPRECATED` em minor N → manter por **≥2 minor releases** → warning de compilação em minor N+2 → remoção permitida só em major.
- Display-list schema só cresce (novas ops no final); breaking schema bump apenas em major.
- LTS de 1.x: **≥12 meses** após `2.0.0` ser lançado.

## [0.15.0] — 2026-05-21

**Marco:** sétima e última fatia portátil de médio prazo — `capy-ui-widget` 0.14 → 0.15. Pool allocator opcional + measure cache fecham o conjunto de infra-pré-congelamento. Janela aberta para **v1.0**.

### Adicionado

- `struct capy_widget_pool { void *buffer; uint32_t size; uint32_t used; uint32_t high_water; }` em `src/widget/capy_widget.h`.
- API pública `void capy_widget_pool_init(struct capy_widget_pool *p, void *buf, uint32_t size)` — anexa um buffer caller-owned (NULL `buf` clampa `size` para 0).
- API pública `void capy_widget_pool_reset(struct capy_widget_pool *p)` — zera `used`, **preserva** `high_water` para diagnósticos.
- API pública `struct capy_widget_allocator capy_widget_allocator_from_pool(struct capy_widget_pool *p)` — wrapper que entrega allocator pronto para `capy_widget_context_init`. `alloc` é bump-allocator alinhado a **16 bytes** (cobre `_Alignof(struct capy_widget)` em todos os targets CapyUI); `free` é no-op (pool é bulk-managed).
- API pública `void capy_widget_invalidate(struct capy_widget *widget)` — walks-up via `parent` setando `layout_dirty = 1` em cada ancestral (e em `widget` mesmo). NULL é no-op.
- Campos aditivos no tail de `struct capy_widget`: `uint8_t layout_dirty` + `uint32_t layout_version`. `capy_widget_create` seeds `layout_dirty = 1` para garantir 1ª computação.
- Helpers internos em `capy_widget.c`: `capy_widget_pool_align_up`, `capy_widget_pool_alloc`, `capy_widget_pool_free`. Macro interna `CAPY_WIDGET_POOL_ALIGN = 16u`.
- Short-circuit em `capy_widget_measure` (em `src/widget/capy_layout.c`): quando `layout_dirty == 0` E `bounds.width/height` já valem o input clampado → retorna 0 sem mexer em estado. Caso contrário, escreve bounds, limpa `layout_dirty`, incrementa `layout_version` (monótono).
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_pool_sequential_allocations` — duas allocs sequenciais; pointers ordenados, gap respeita alinhamento, `used`/`high_water` consistentes; `free` no-op não corrompe.
  2. `test_pool_overflow_returns_null` — buffer 64 bytes, alloc 32 + 64 → segundo retorna NULL com pool intacto; NULL/zero edge cases retornam NULL.
  3. `test_pool_reset_keeps_high_water` — reset zera `used` mas mantém `high_water`; nova alloc após reset funciona.
  4. `test_pool_alignment_16` — pointers retornados de allocs de tamanho 1/7/15 são todos 16-byte aligned via `((uintptr_t)p & 15u) == 0u`.
  5. `test_pool_backs_widget_create` — pool alimenta `capy_widget_create`; `destroy` não mexe em `used` (free no-op); reset reclama tudo para árvore nova.
  6. `test_layout_measure_cache_short_circuit` — primeira measure bumpa `layout_version` para 1; segunda com args iguais NÃO bumpa (cache hit); mudar `avail_w` força recompute.
  7. `test_invalidate_walks_up_parents` — invalidate em folha marca folha + middle + root todos dirty; measure subsequente recomputa; NULL guard.

### Mudado

- `capy_widget_create` agora explicita `widget->layout_dirty = 1u` (era 0 por zero-init).
- `capy_widget_measure` agora honra o cache e bumpa `layout_version` em recompute reais.
- `Makefile` banner e guards bumpados para `0.15.0` (alpha.251). `lint` e `version-check` exigem `VERSION == 0.15.0`.
- `VERSION` 0.14.0 → 0.15.0.
- `README.md` Version banner 0.14.0 → 0.15.0; URLs de exemplo apontam para tag `v0.15.0`.

### ABI

- `capy-ui-widget` **0.14 → 0.15** (aditivo: 1 struct nova `capy_widget_pool`, 4 APIs novas, 2 campos no tail de `capy_widget`). **Última fatia de médio prazo entregue** antes da janela v1.0.
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** — performance tier 1 age no allocator e no cache de measure, não no render.

### Docs

- `docs/compatibility.md` ganhou bloco "Performance tier 1 (since 0.15)"; ABI line bumpada para `v0.15`; lista de coberturas inclui pool/cache.
- `docs/roadmap/medium-term/v0.15-performance-tier-1.md` status = delivered (CapyUI side); benchmark CI fica como follow-up opt-in externo.
- `docs/roadmap/contracts/abi-versions.md` linha 0.15 promovida para **atual**, 0.14 demovida para `tagged`; tag `v0.15.0` adicionada na timeline; nota sobre janela v1.0 explicitada.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.15.0 / `capy-ui-widget 0.15` / DL schema 4 / 96 testes / 7 fatias de médio prazo. Próxima janela agora é **v1.0**.

## [0.14.0] — 2026-05-21

**Marco:** sexta fatia portátil de médio prazo entregue — `capy-ui-widget` 0.13 → 0.14. Formato textual canônico para `capy_theme` (key=value line-oriented), permitindo persistência, auditoria, edição manual e troca em runtime.

### Adicionado

- Constante pública `CAPY_THEME_FORMAT_VERSION = 2u` em `src/widget/capy_widget.h`.
- API pública `int capy_theme_serialize(const struct capy_theme *t, char *out, uint32_t cap)`:
  - Ordem canônica fixa: `version=2`, `variant=light|dark|high_contrast`, `high_contrast=0|1`, depois cada token (`bg_base`, `bg_raised`, `bg_sunken`, `fg_primary`, `fg_muted`, `fg_inverse`, `accent`, `accent_hover`, `border`, `border_focus`, `focus_ring`, `danger`, `warning`, `success`, `info`, `disabled`) como `name=0xAARRGGBB` em hex uppercase zero-padded.
  - Determinístico byte-a-byte: mesma `theme` → mesma string em runs independentes.
  - Retorna bytes escritos (excl. NUL) em sucesso; `-1` em overflow / `NULL t/out` / `cap=0` / `variant` inválido.
  - Saída sempre NUL-terminada em sucesso.
- API pública `int capy_theme_deserialize(struct capy_theme *out, const char *in, uint32_t len)`:
  - Parser line-oriented tolerante: trim leading/trailing whitespace ao redor de chave e valor, `\r` no fim de linha aceito, linhas em branco e comentários `#` ignorados.
  - `0x`/`0X` opcional no prefixo hex (até 8 dígitos), case-insensitive `[0-9A-Fa-f]`.
  - Versão decimal obrigatória; `version=0` ou `version > CAPY_THEME_FORMAT_VERSION` → `-1`. Linha `version=` ausente → `-1`.
  - Chaves desconhecidas (tokens futuros) **silenciosamente ignoradas** → forward-compat com schemas que crescem.
  - Tokens conhecidos ausentes ficam zero em `out`; callers podem seedar `out` com `capy_theme_default_light/dark/high_contrast` antes para herdar defaults.
  - Retorna `0` em sucesso, `-1` em: NULL args, malformação (linha sem `=`, hex inválido, `variant` desconhecido, `high_contrast` ≠ 0/1, key vazia).
- Helpers internos estáticos em `capy_widget.c`: `capy_theme_token_name`, `capy_theme_append_str`, `capy_theme_append_hex32` (8-dígitos zero-padded uppercase), `capy_theme_range_eq_literal`, `capy_theme_token_from_range`, `capy_theme_parse_hex32`, `capy_theme_parse_decimal_u32`, `capy_theme_parse_bool`.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_theme_serialize_round_trip` — `default_dark` → serialize → deserialize → tokens/variant/high_contrast equivalentes; NUL-terminação e versão restaurada confirmadas.
  2. `test_theme_serialize_determinism` — duas chamadas independentes produzem buffers byte-iguais.
  3. `test_theme_serialize_overflow_returns_minus_one` — buffer pequeno, `NULL t/out`, `cap=0`, variant inválido (`99`) todos retornam `-1`.
  4. `test_theme_deserialize_rejects_future_version` — `version=3` rejeitada.
  5. `test_theme_deserialize_rejects_malformed` — linha sem `=`, hex inválido (`NOTHEX`), variant desconhecida (`fuchsia`), bool inválido (`2`), `version=` ausente, NULL args todos `-1`.
  6. `test_theme_deserialize_ignores_unknown_keys` — chave `future_token_v9=0xDEADBEEF` ignorada sem afetar `bg_base`/`fg_primary` ao redor; tokens ausentes ficam zero.
  7. `test_theme_deserialize_tolerates_whitespace_and_comments` — comentários `#`, linhas em branco, e espaços ao redor de chave/`=`/valor todos tolerados.
- Helpers de teste `theme_tokens_equal` e `theme_buf_streq` para comparar conteúdo sem depender de strcmp libc.

### Mudado

- `capy_theme_default_light()`, `capy_theme_default_dark()`, `capy_theme_high_contrast()` agora retornam `version = (uint16_t)CAPY_THEME_FORMAT_VERSION` (= 2) em vez de `version = 1u`. Alinha o struct com o formato textual publicado.
- `test_theme_defaults_snapshot` atualizado para asserir `light.version == (uint16_t)CAPY_THEME_FORMAT_VERSION` (substitui `== 1u` hard-coded), refletindo o bump deliberado.
- `Makefile` banner e guards bumpados para `0.14.0` (alpha.250). `lint` e `version-check` exigem `VERSION == 0.14.0`.
- `VERSION` 0.13.0 → 0.14.0.
- `README.md` Version banner 0.13.0 → 0.14.0; URLs de exemplo apontam para tag `v0.14.0`.

### ABI

- `capy-ui-widget` **0.13 → 0.14** (aditivo: 1 macro nova `CAPY_THEME_FORMAT_VERSION`, 2 APIs novas, default themes com `version` atualizada).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** — serialização de tema age fora do render.

### Docs

- `docs/compatibility.md` ganhou bloco "Theme serialization (since 0.14)"; ABI line bumpada para `v0.14`; lista de coberturas inclui serialize/deserialize.
- `docs/roadmap/contracts/theme-serialization.md` status = delivered (CapyUI side); semântica expandida com regras precisas de erro, fallback e forward-compat; 7 testes catalogados; histórico atualizado (defaults version 1→2).
- `docs/roadmap/contracts/abi-versions.md` linha 0.14 promovida para **atual**, 0.13 demovida para `tagged`; tag `v0.14.0` adicionada na timeline.
- `docs/roadmap/medium-term/v0.14-theming-ux.md` status = delivered (CapyUI side); ABI bump 0.13 → 0.14 confirmado.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.14.0 / `capy-ui-widget 0.14` / DL schema 4 / 89 testes / 6 fatias de médio prazo.

## [0.13.0] — 2026-05-21

**Marco:** quinta fatia portátil de médio prazo entregue — `capy-ui-widget` 0.11 → 0.13 (pulando 0.12, que segue reservada para `shell integration` gated por Etapa 6 do CapyOS). Suporte completo a locale, plural rules CLDR e layout RTL via mirror pós-layout determinístico.

### Adicionado

- `enum capy_plural_rule` em `src/widget/capy_widget.h`: `CAPY_PLURAL_EN`, `CAPY_PLURAL_PT`, `CAPY_PLURAL_AR`, `CAPY_PLURAL_RU`, `CAPY_PLURAL_OTHER`.
- `struct capy_locale` em `src/widget/capy_widget.h`: `char tag[16]` (BCP 47 NUL-terminada), `uint8_t rtl`, `uint8_t plural_rule`, `uint16_t reserved`.
- Campo aditivo `locale` no tail de `struct capy_widget_context` (since 0.13). `capy_widget_context_init` agora seeda `capy_locale_default()` automaticamente.
- Campo aditivo `uint8_t rtl` no tail de `struct capy_layout_constraints` (since 0.13). Quando `rtl != 0`, `capy_widget_arrange` mirrors o `x` dos filhos diretos via `new_x = 2*ix + iw - old_x - width`.
- 4 APIs novas em `src/widget/capy_widget.c`:
  - `struct capy_locale capy_locale_default(void)` → `("en-US", LTR, EN)`.
  - `void capy_context_set_locale(ctx, l)` — NULL restaura o default.
  - `int capy_locale_plural(l, n)` — implementa as 5 regras CLDR (NULL locale cai em EN).
  - `int capy_locale_format(l, out, out_cap, fmt, ...)` — subset printf snprintf-like com `%d` (int32_t), `%u` (uint32_t), `%x` (uint32_t hex lowercase), `%s` (const char*), `%%` (literal). Variadic via `<stdarg.h>` (incluído em `capy_widget.h`). INT32_MIN tratado via `int64_t` para evitar UB. Saída sempre NUL-terminada em sucesso. Fail-closed em overflow / NULL out / cap=0 / NULL fmt / `%` final / especificador desconhecido.
- Helpers internos estáticos em `capy_widget.c`: `capy_fmt_append_char`, `capy_fmt_u32` (base 10 ou 16), `capy_fmt_i32` (sinal + UB-safe negation).
- `capy_widget_arrange` refatorada em `src/widget/capy_layout.c`: layout kinds (`arrange_stack`, `arrange_grid`, `arrange_flow`) não recursam mais nos filhos internamente; o dispatcher executa **(1) layout-specific bounds → (2) RTL mirror se rtl=1 → (3) recursão nos filhos** uniformemente. Behavior preservado para árvores LTR (mesmas posições que pré-0.13).
- Helper estático `capy_layout_mirror_child_x` em `capy_layout.c` aplica o mirror dentro do content rect `[ix, ix+iw)`.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `test_locale_default_seeds_context` — `capy_locale_default()` retorna `en-US/LTR/EN` e `capy_widget_context_init` propaga o mesmo.
  2. `test_locale_set_null_restores_default` — set custom (ar-EG/RTL/AR) → NULL → default novamente.
  3. `test_locale_plural_en_pt` — n=0,1,2,21 em EN/PT (1 → 0, else → 1); NULL locale também cai em EN.
  4. `test_locale_plural_ar` — n=0→zero, 1→one, 2→two, 5→few, 21→many, 100→other.
  5. `test_locale_plural_ru` — n=1,21 (mod10=1, mod100≠11) → one; n=2,4 → few; n=11 (special), 5, 0, 100 → many.
  6. `test_locale_format_subset` — `%d` (negativo, zero), `%u`, `%x` lowercase, `%s`, `%%`, e uma string com todos os specifiers em uma única chamada.
  7. `test_locale_format_overflow_returns_minus_one` — cap insuficiente, NULL out, cap=0, NULL fmt, `%` final, `%q` desconhecido todos retornam `-1`.
  8. `test_layout_rtl_mirrors_horizontal_stack` — STACK horizontal com rtl=1 produz posições espelhadas exatas (a:70, b:40 em panel 100w com filhos 30w); determinismo entre runs.
  9. `test_layout_rtl_vertical_stack_x_unchanged` — STACK vertical com rtl=1 mantém `x=0` porque `width=iw=100`.
- Helper estático `locale_streq` em testes (string compare independente da identidade de literais).

### Mudado

- `Makefile` banner e guards bumpados para `0.13.0` (alpha.249). `lint` e `version-check` agora exigem `VERSION == 0.13.0`.
- `VERSION` 0.11.0 → 0.13.0 (skip 0.12 alinhado com ABI reservada para shell integration).
- `README.md` Version banner 0.11.0 → 0.13.0; URLs de exemplo apontam para tag `v0.13.0`.
- `capy_widget_context_init` agora chama `capy_locale_default()` ao final para seedar o campo `locale` (não-disruptivo: pre-0.13 callers nunca leem esse campo).
- `<stdarg.h>` adicionado ao topo de `capy_widget.h` para a assinatura variádica de `capy_locale_format`.

### ABI

- `capy-ui-widget` **0.11 → 0.13** (aditivo: 1 enum novo, 1 struct nova, 1 campo no tail de `capy_widget_context`, 1 campo no tail de `capy_layout_constraints`, 4 APIs novas). 0.12 fica reservada para shell integration (gated por Etapa 6).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** — locale/RTL agem sobre layout, não sobre o display-list.

### Docs

- `docs/compatibility.md` ganhou bloco "Locale + RTL (since 0.13)"; ABI line bumpada para `v0.13`; lista de coberturas inclui locale/RTL.
- `docs/roadmap/contracts/locale-rtl.md` status = delivered (CapyUI side); seção "Semântica RTL" expandida com fórmula do mirror; format subset detalhado; 9 testes catalogados; histórico atualizado. Assinatura de `capy_locale_format` ajustada para a ordem snprintf-like `(l, out, out_cap, fmt, ...)`.
- `docs/roadmap/contracts/abi-versions.md` linha 0.13 promovida para **atual**, 0.11 demovida para `tagged`, 0.12 marcada explicitamente como `reservado (gate Etapa 6)`; tag `v0.13.0` adicionada na timeline.
- `docs/roadmap/medium-term/v0.13-i18n-rtl.md` status = delivered (CapyUI side); ABI bump 0.11 → 0.13 confirmado (com nota sobre o skip de 0.12).
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.13.0 / `capy-ui-widget 0.13` / DL schema 4 / 82 testes / 5 fatias de médio prazo.

## [0.11.0] — 2026-05-21

**Marco:** quarta fatia portátil de médio prazo entregue — `capy-ui-widget` 0.10 → 0.11. Árvore de acessibilidade paralela ao widget tree, snapshot determinístico host-testável para bridges de a11y do SO.

### Adicionado

- Struct pública `capy_a11y_node` em `src/widget/capy_widget.h`:
  ```c
  struct capy_a11y_node {
    uint32_t widget_id;       /* hash FNV-1a 32-bit do path */
    const char *role;         /* literal estático */
    const char *label;        /* widget->text ou "Untitled" */
    const char *description;  /* NULL em 0.11; reservado p/ aria-describedby */
    uint32_t state_flags;     /* bitmask CAPY_A11Y_* */
    uint32_t parent_id;       /* widget_id do pai; 0 no root */
  };
  ```
- 8 macros de state flags: `CAPY_A11Y_FOCUSED` (`0x1`), `CHECKED` (`0x2`), `DISABLED` (`0x4`), `HIDDEN` (`0x8`), `PRESSED` (`0x10`), `EXPANDED` (`0x20`), `SELECTED` (`0x40`), `READONLY` (`0x80`).
- API pública `int capy_widget_a11y_snapshot(struct capy_widget *root, struct capy_a11y_node *out, uint32_t cap);` em `src/widget/capy_widget.c`:
  - Walk pre-order do widget tree (visit self, then children).
  - Hash FNV-1a 32-bit: `CAPY_A11Y_FNV_OFFSET = 0x811C9DC5`, `CAPY_A11Y_FNV_PRIME = 0x01000193`. Root: `widget_id = OFFSET`. Filho na posição `i`: `widget_id = hash_step(parent_id, i)` mixando 4 bytes pelo prime. Path-stable: mesma posição → mesmo id entre runs.
  - Role mapping cobrindo os 12 valores do enum `capy_widget_type`: `label`, `button`, `textbox`, `checkbox`, `list`, `panel`, `scrollbar`, `menu`, `menuitem`, `dialog`, `progressbar`, `tab`. Default → `"none"`. Strings são literais estáticos — zero allocation.
  - State flags refletem o estado real: `focused → FOCUSED`, `checked → CHECKED`, `!enabled → DISABLED`, `!visible → HIDDEN`, `TEXTBOX && text_edit.readonly → READONLY`.
  - Label: `widget->text` quando não-vazio (aponta direto para o buffer do widget), senão literal `"Untitled"`.
  - `description` sempre NULL em 0.11.
  - Retornos: `int` ≥ 0 com nodes escritos em sucesso; `-1` em overflow de `cap` (partial fill, sem rollback), `NULL root`, `cap == 0`, ou `out == NULL && cap > 0`.
  - Snapshot **nunca** muta o widget tree.
- Helpers internos estáticos em `capy_widget.c`: `capy_a11y_hash_step`, `capy_a11y_role_for`, `capy_a11y_flags_for`, `capy_a11y_walk`.
- 7 novos testes em `tests/test_widget_contracts.c`:
  1. `test_a11y_snapshot_basic` — widget isolado: 1 nó, `parent_id=0`, `widget_id!=0`, role `"button"`, label do `widget->text`, `description=NULL`, flags `DISABLED|HIDDEN` zerados.
  2. `test_a11y_snapshot_hierarchy` — panel+button+checkbox: 3 nós em pre-order, `parent_id` dos filhos = `widget_id` do panel, ids dos irmãos divergem.
  3. `test_a11y_roles_cover_focusable_types` — BUTTON/TEXTBOX/CHECKBOX/LIST/MENU_ITEM/TABS recebem role não-NULL e correto.
  4. `test_a11y_widget_id_stable_across_snapshots` — dois snapshots independentes da mesma árvore produzem `widget_id`/`parent_id`/`role`/`label`/`state_flags` idênticos campo-a-campo.
  5. `test_a11y_state_flags_match_widget_state` — checkbox com `focused=1, checked=1` exibe `FOCUSED|CHECKED`; textbox com `text_edit.readonly=1` exibe `READONLY`; button com `enabled=0, visible=0` exibe `DISABLED|HIDDEN`.
  6. `test_a11y_overflow_partial_fill_returns_minus_one` — `cap=1` com 2-node tree → `-1` mas root preenchido em `out[0]`; `cap=0` → `-1`; `out=NULL` com `cap>0` → `-1`; `root=NULL` → `-1`.
  7. `test_a11y_label_untitled_fallback` — widget sem texto → label `"Untitled"`; setar texto → label muda para `widget->text`.
- Helper estático `a11y_streq` em testes (comparação de strings independente da identidade de literais).

### Mudado

- `Makefile` banner e guards bumpados para `0.11.0` (alpha.247). `lint` e `version-check` agora exigem `VERSION == 0.11.0`.
- `VERSION` 0.10.0 → 0.11.0.
- `README.md` Version banner 0.10.0 → 0.11.0; URLs de exemplo apontam para tag `v0.11.0`.

### ABI

- `capy-ui-widget` **0.10 → 0.11** (aditivo: 8 macros novas de state flags, 1 struct nova `capy_a11y_node`, 1 API nova `capy_widget_a11y_snapshot`).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** — a11y é paralelo ao render.

### Docs

- `docs/compatibility.md` ganhou bloco "Accessibility tree (since 0.11)"; ABI line bumpada para `v0.11`; lista de coberturas inclui a11y snapshot.
- `docs/roadmap/contracts/accessibility.md` status = delivered (CapyUI side); seção "Semântica" expandida com hash FNV-1a, role mapping completo, state-flag mapping; 7 testes catalogados; histórico atualizado.
- `docs/roadmap/contracts/abi-versions.md` linha 0.11 promovida para **atual**, 0.10 demovida para `tagged`; tag `v0.11.0` adicionada na timeline.
- `docs/roadmap/medium-term/v0.11-accessibility.md` status = delivered (CapyUI side); ABI bump 0.10 → 0.11 confirmado.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.11.0 / `capy-ui-widget 0.11` / DL schema 4 / 74 testes.

## [0.10.0] — 2026-05-21

**Marco:** terceira fatia portátil de médio prazo entregue — `capy-ui-widget` 0.9 → 0.10. Eventos abstratos cobrem agora WHEEL, TOUCH e GAMEPAD além de POINTER/KEY. Display-list schema permanece 4 (input não afeta render).

### Adicionado

- 5 novos valores aditivos em `enum capy_widget_event_type` (appended at the tail): `CAPY_WIDGET_EVENT_WHEEL`, `CAPY_WIDGET_EVENT_TOUCH_BEGIN`, `CAPY_WIDGET_EVENT_TOUCH_MOVE`, `CAPY_WIDGET_EVENT_TOUCH_END`, `CAPY_WIDGET_EVENT_GAMEPAD`.
- Struct `capy_ui_point` (`int32_t x`, `int32_t y`) em `src/widget/capy_widget.h` — usada pelo touch payload e por futuras superfícies que precisem de coordenada sem tamanho.
- Payloads tipados:
  - `struct capy_wheel_payload { int16_t delta_x; int16_t delta_y; }`.
  - `struct capy_touch_payload { uint32_t touch_id; struct capy_ui_point pos; uint32_t pressure_x256; }`.
  - `struct capy_gamepad_payload { uint16_t button_mask; int16_t axis[4]; }`.
- Campo aditivo `const void *payload` no tail de `struct capy_widget_event` (since 0.10). Tipo concreto determinado pelo `event->type`. CapyUI nunca escreve através do ponteiro.
- Constantes de modifier expandidas: `CAPY_MOD_CAPSLOCK = 0x10u` e `CAPY_MOD_NUMLOCK = 0x20u`. Disjuntas das `CTRL/ALT/SHIFT/META` pré-existentes — bitmask completo cobre `0x3Fu`.
- Helpers internos em `src/widget/capy_widget.c`:
  - `capy_widget_find_list_ancestor(w)` — sobe a cadeia de pais até o primeiro `CAPY_WIDGET_LIST`.
  - `capy_clamp_i32(v, lo, hi)` — clamp inteiro (no-op quando `lo == hi`).
  - `capy_widget_handle_wheel` — extrai `capy_wheel_payload`, localiza LIST ancestor sob o cursor, ajusta `value` clampado em `[min_value, max_value]` e dispara `on_change` quando o valor muda.
  - `capy_widget_handle_touch` — copia o evento, sintetiza `POINTER_DOWN/MOVE/UP`, sobrescreve `x/y` com `payload->pos` quando presente, zera `payload` na cópia e recurses em `capy_widget_handle_event`.
- Roteamento novo no `capy_widget_handle_event`:
  - `WHEEL` → `capy_widget_handle_wheel` (retorna 1 quando LIST absorve, 0 caso contrário).
  - `TOUCH_BEGIN/MOVE/END` → `capy_widget_handle_touch` (recursão via POINTER).
  - `GAMEPAD` → aceito silenciosamente (`return 0`); mapeamento configurável fica para fatia futura.
  - Tipos desconhecidos (futuros, > GAMEPAD) → percorrem a cascata padrão sem matchar nada e retornam 0 sem crash.
- 8 novos testes em `tests/test_widget_contracts.c`:
  1. `test_event_wheel_on_list_scrolls` — WHEEL em LIST ajusta `value` e dispara `on_change` exatamente uma vez.
  2. `test_event_wheel_clamps_to_range` — overflow positivo clampa em `max_value`; negativo clampa em `min_value`.
  3. `test_event_wheel_on_panel_returns_unhandled` — WHEEL fora de LIST retorna 0 sem mexer no `value` do PANEL.
  4. `test_event_touch_single_routes_as_pointer` — TOUCH_BEGIN sintético em coordenadas do botão aciona `on_click`.
  5. `test_event_touch_uses_payload_position_over_event_xy` — `event.x/y` apontam fora do botão, mas `payload->pos.x/y` apontam dentro → `on_click` dispara.
  6. `test_event_gamepad_no_crash` — GAMEPAD com `button_mask` e `axis` setados retorna 0 silenciosamente sobre widget focado.
  7. `test_event_unknown_type_ignored` — tipo fora do enum conhecido (`(int)GAMEPAD + 7`) retorna 0 sem crash.
  8. `test_event_modifier_flags_distinct` — os 6 bits de modifier são distintos par-a-par e a união cobre `0x3Fu`.
- Helper estático `change_count` em testes (espelha `click_count`) para verificar `on_change` em WHEEL→LIST.

### Mudado

- `capy_widget_handle_event` agora ramifica por tipo no topo (WHEEL/TOUCH/GAMEPAD) antes da cascata legacy de POINTER/KEY. POINTER_MOVE, POINTER_DOWN, KEY_DOWN etc. mantêm comportamento idêntico ao de 0.9.
- `Makefile` banner e guards bumpados para `0.10.0` (alpha.246). `lint` e `version-check` agora exigem `VERSION == 0.10.0`.
- `VERSION` 0.9.0 → 0.10.0.
- `README.md` Version banner 0.9.0 → 0.10.0; URLs de exemplo apontam para tag `v0.10.0`.

### ABI

- `capy-ui-widget` **0.9 → 0.10** (aditivo: 5 valores novos no enum de eventos, 1 struct nova `capy_ui_point`, 3 structs de payload, 1 campo `const void *payload` no tail de `capy_widget_event`, 2 constantes de modifier).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **permanece 4** — input não afeta o render.

### Docs

- `docs/compatibility.md` ganhou bloco "Input plumbing (since 0.10)"; ABI line bumpada para `v0.10`; lista de coberturas inclui input plumbing; eventos abstratos agora citam POINTER/KEY/WHEEL/TOUCH/GAMEPAD.
- `docs/roadmap/contracts/input.md` status = delivered (CapyUI side); seção "Roteamento" reescrita para refletir o comportamento implementado; 8 testes catalogados; histórico introduzido. `payload` documentado como `const void *` com lifetime garantido pelo host durante a chamada.
- `docs/roadmap/contracts/abi-versions.md` linha 0.10 promovida para **atual**, 0.9 demovida para `tagged`; tag `v0.10.0` adicionada na timeline.
- `docs/roadmap/medium-term/v0.10-input-plumbing.md` status = delivered (CapyUI side); ABI bump 0.9 → 0.10 confirmado.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.10.0 / `capy-ui-widget 0.10` / DL schema 4 / 67 testes.

## [0.9.0] — 2026-05-21

**Marco:** segunda fatia portátil de médio prazo entregue — `capy-ui-widget` 0.8 → 0.9. Display-list schema 3 → 4 com rename layout-compatível `reserved` → `font_id` em `capy_dl_cmd`. Hook opcional de métricas de texto fornecido pelo host com fallback determinístico.

### Adicionado

- Typedef pública `capy_text_metrics_fn` em `src/widget/capy_widget.h`: `int (*)(uint16_t font_id, uint8_t font_size, const char *text, uint16_t len, uint32_t *out_width, uint32_t *out_height, void *user_data)`.
- Campos aditivos em `struct capy_widget_context` (tail, since 0.9): `capy_text_metrics_fn text_metrics_fn; void *text_metrics_user;`. `capy_widget_context_init` zera ambos (host opta por setar).
- Campo aditivo `uint16_t font_id` em `struct capy_widget` (tail, since 0.9). Default `0` após `capy_widget_create`.
- API pública `void capy_widget_set_text_metrics_hook(struct capy_widget_context *ctx, capy_text_metrics_fn fn, void *user_data);` em `src/widget/capy_widget.c`. Passar `fn == NULL` limpa o hook.
- API pública `int capy_widget_measure_text(const struct capy_widget_context *ctx, uint16_t font_id, uint8_t font_size, const char *text, uint16_t len, uint32_t *out_width, uint32_t *out_height);`:
  - `out_width == NULL` ou `out_height == NULL` → `-1`.
  - `text == NULL` ou `len == 0` → `*out_width = 0`, `*out_height = effective_size` (hook **não** chamado; curto-circuito determinístico).
  - Hook setado e retornando `0`: outputs do hook.
  - Hook NULL ou hook retornando `-1`: fallback determinístico `*out_width = (effective_size / 2) * len`, `*out_height = effective_size`, com `effective_size = (font_size > 0 ? font_size : 16)`.
- Em `src/widget/capy_display_list.c`, `capy_widget_emit_recursive` copia `widget->font_id` para `cmd->font_id` em cada `CAPY_DL_TEXT`.
- 6 novos testes em `tests/test_widget_contracts.c`:
  1. `test_text_metrics_fallback_deterministic` — fallback exato, edge cases (font_size 0 → effective 16, len 0 → width 0, NULL out args → -1).
  2. `test_text_metrics_hook_called_when_set` — hook recebe `font_id`/`font_size`/`len` corretos e seus outputs propagam.
  3. `test_text_metrics_hook_cleared_restores_fallback` — `set_text_metrics_hook(ctx, NULL, NULL)` para de chamar o hook anterior; fallback volta.
  4. `test_text_metrics_hook_failure_falls_back` — hook retornando `-1` força fallback determinístico no mesmo call.
  5. `test_displaylist_emits_font_id` — schema=4; `widget->font_id = 42` propaga para `cmd->font_id` na op `CAPY_DL_TEXT`.
  6. `test_displaylist_font_id_default_zero` — `widget->font_id` é 0 após `capy_widget_create` e o emit reflete 0 no display-list.
- Dois testes da v0.8 (`test_displaylist_diff_identical_zero_rects`, `test_displaylist_dirty_hint_op_not_emitted`) tiveram assertions de versão hard-coded `== 3u` substituídas por `== CAPY_DISPLAY_LIST_SCHEMA_VERSION`/`>= 3u` para permanecerem válidas após bumps futuros.

### Mudado

- `CAPY_DISPLAY_LIST_SCHEMA_VERSION` 3 → 4 (aditivo; consumidores antigos ignoram o campo `font_id` por desconhecerem o significado, mas leem o mesmo layout binário porque `reserved` ocupava o slot).
- `struct capy_dl_cmd`: campo `uint16_t reserved` renomeado para `uint16_t font_id` (layout idêntico, semântica nova). Documentação atualizada em `docs/roadmap/contracts/display-list.md` para refletir que o slot reservado de 16 bits foi consumido — novos campos devem ser appended at the tail (após `image_id`).
- `Makefile` banner e guards bumpados para `0.9.0` (alpha.245). `lint` e `version-check` agora exigem `VERSION == 0.9.0`.
- `VERSION` 0.8.0 → 0.9.0.
- `README.md` Version banner 0.8.0 → 0.9.0; URLs de exemplo apontam para tag `v0.9.0`.

### ABI

- `capy-ui-widget` **0.8 → 0.9** (aditivo: novo typedef, dois novos campos no tail de `capy_widget_context`, um novo campo no tail de `capy_widget`, rename layout-compatível em `capy_dl_cmd`, duas APIs novas).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **3 → 4**.

### Docs

- `docs/compatibility.md` ganhou bloco "Text metrics hook (since 0.9)"; "Display-list schema" agora menciona o campo `font_id`; ABI line atualizada para `v0.9`; lista de coberturas inclui hook + propagação de `font_id`.
- `docs/roadmap/contracts/text-metrics-hook.md` status = delivered (CapyUI side); semântica expandida com short-circuit por `text == NULL`/`len == 0` e contrato de NULL out args; histórico introduzido.
- `docs/roadmap/contracts/display-list.md` schema atual = 4; struct `capy_dl_cmd` atualizado para `uint16_t font_id`; testes 0.9 catalogados; linha histórica 0.9.0; nota de evolução atualizada (slot reservado consumido).
- `docs/roadmap/contracts/abi-versions.md` linha 0.9 promovida para **atual**, 0.8 demovida para `tagged`; tag `v0.9.0` adicionada na timeline.
- `docs/roadmap/medium-term/v0.9-text-fonts-host.md` status = delivered (CapyUI side); ABI bump 0.8 → 0.9 confirmado.
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.9.0 / `capy-ui-widget 0.9` / DL schema 4 / 59 testes.

## [0.8.0] — 2026-05-21

**Marco:** primeira fatia portátil pós-curto-prazo entregue — `capy-ui-widget` 0.6 → 0.8 (pulando 0.7, reservado para futuro adapter metadata bump). Display-list schema 2 → 3 (`CAPY_DL_DIRTY_HINT`). API nova determinística `capy_widget_diff`.

### Adicionado

- `enum capy_dl_op` ganha entrada aditiva `CAPY_DL_DIRTY_HINT` (append no fim, schema 2 → 3) em `src/widget/capy_display_list.h`. Op é **opcional** e nunca emitida por `capy_widget_emit`; reservada para produtores higher-level (compositor adapter, ferramentas devtools).
- API pública `int capy_widget_diff(const struct capy_display_list *prev, const struct capy_display_list *next, struct capy_ui_rect *out_dirty, uint32_t out_cap)` em `src/widget/capy_display_list.{h,c}`:
  - Percorre `prev` e `next` em paralelo por índice e emite o rect de cada cmd que diferir.
  - Quando o rect também diferir entre as duas posições, emite ambos os rects.
  - Coalescing estável: rect idêntico ao último escrito é suprimido (deduplicação sequencial).
  - Cmds remanescentes em apenas uma das listas → emite seus rects.
  - Determinístico byte-a-byte: mesma `(prev, next)` → mesma sequência de rects em runs independentes.
  - Overflow de `out_cap` → retorna `-1` com `out_dirty` parcialmente preenchido (sem rollback). `out_dirty == NULL && out_cap > 0` → `-1`. `prev == NULL` ou `next == NULL` → lista vazia.
  - O(max(prev->count, next->count)), zero allocação.
- Helpers internos `capy_dl_cmd_equal` (memcmp seguro porque `capy_dl_push` zera padding), `capy_dl_rect_equal`, `capy_dl_dirty_append`.
- 6 novos testes em `tests/test_widget_contracts.c`:
  1. `test_displaylist_diff_identical_zero_rects` — duas emissões idênticas → 0 rects.
  2. `test_displaylist_diff_button_color_change` — troca de `bg_color` produz rect pequeno do botão, não tela inteira.
  3. `test_displaylist_diff_coalesces_adjacent` — `CLIP_PUSH` + `RECT` + `BORDER` compartilhando o mesmo rect e todos alterados ao mesmo tempo → 1 único rect coalescido.
  4. `test_displaylist_diff_overflow_returns_minus_one` — cap insuficiente → `-1`; `out_dirty == NULL` com cap>0 → `-1`.
  5. `test_displaylist_diff_determinism` — duas chamadas independentes → mesmos rects byte-a-byte.
  6. `test_displaylist_dirty_hint_op_not_emitted` — `CAPY_DISPLAY_LIST_SCHEMA_VERSION == 3`, `CAPY_DL_DIRTY_HINT` é maior que `CAPY_DL_FOCUS_RING`, e `capy_widget_emit` jamais produz a op nova.
- `Makefile` banner e guards bumpados para `0.8.0` (alpha.244). `lint` e `version-check` agora exigem `VERSION == 0.8.0`.

### Mudado

- `CAPY_DISPLAY_LIST_SCHEMA_VERSION` 2 → 3 (aditivo). Consumidores antigos que só reconhecem ops ≤ 7 ignoram `CAPY_DL_DIRTY_HINT` (regra "aceitar versão maior e ignorar ops desconhecidas").
- `README.md` Version banner 0.7.3 → 0.8.0; URLs de exemplo apontam para tag `v0.8.0`.
- `VERSION` 0.7.3 → 0.8.0.

### ABI

- `capy-ui-widget` **0.6 → 0.8** (aditivo, pulando 0.7 que fica reservada para o futuro adapter metadata bump quando Etapa 4 CapyOS fechar).
- `capy-ui-desktop-session` permanece **1**.
- Display-list schema **2 → 3**.

### Docs

- `docs/compatibility.md` ganhou bloco "Compositor diff (since 0.8)" e referências de schema/ABI bumpadas para v0.8/schema 3.
- `docs/roadmap/contracts/display-list.md` schema atual = 3; nova seção "`capy_widget_diff` semantics" + linha histórica 0.8.0; 6 testes novos catalogados.
- `docs/roadmap/contracts/compositor.md` status = delivered (CapyUI side).
- `docs/roadmap/contracts/abi-versions.md` linha 0.8 promovida para `**atual**`, 0.6 demovida para `tagged`; tag `v0.8.0` adicionada na timeline.
- `docs/roadmap/medium-term/v0.8-compositor-host.md` status = delivered (CapyUI side).
- `docs/roadmap/STATUS.md` e `tracking/ABSOLUTE.md` bumpados para 0.8.0 / `capy-ui-widget 0.8` / DL schema 3.

## [0.7.3] — 2026-05-20

### Mudado

- `Makefile` agora gera URLs versionadas em cada `.manifest` (`https://github.com/<owner>/<repo>/releases/download/v$(VERSION)/<asset>.bin`) em vez de depender do redirect `/releases/latest/<asset>.bin`. CapyOS first-boot pode pinar o `modules-index.txt` por tag semver sem hop extra.
- README "Automatic deploy" reescrito para refletir o pinning por tag.

### ABI

- `capy-ui-widget` permanece **0.6** (sem mudança).
- `capy-ui-desktop-session` permanece **1** (sem mudança).

## [0.7.2] — 2026-05-20

### Mudado

- Build e publish decouplados: `make validate` + `make package` rodam separadamente da publicação. Pipeline `.github/workflows/release-artifacts.yml` reorganizada para builds determinísticos.
- `modules-index.txt` agora é publicado junto com os assets na GitHub Release rolling `latest` (antes era gerado mas não anexado).

### ABI

- `capy-ui-widget` permanece **0.6**.
- `capy-ui-desktop-session` permanece **1**.

## [0.7.1] — 2026-05-20

### Adicionado

- Pipeline contínua: a cada push em `main`, `release-artifacts.yml` roda `make validate` + `make package`, move a tag rolling `latest`, e republica a GitHub Release `latest` com `.bin/.manifest/modules-index.txt` reconstruídos.
- Tags `v*` (ex.: `v0.7.1`) cortam frozen releases pelo mesmo workflow.

### ABI

- `capy-ui-widget` permanece **0.6**.
- `capy-ui-desktop-session` permanece **1**.

## [0.7.0] — 2026-05-20

**Marco:** migração de ownership da sessão de desktop, window manager e built-in apps do fallback in-tree do CapyOS para este repositório. CapyUI passa a publicar **dois** módulos capypkg.

### Adicionado

- `src/desktop/` (sessão de desktop, taskbar, launcher, system tray, desktop icons, wallpaper, notifications, smoke readiness, taskbar menu): `desktop.c`, `desktop_icons.c`, `desktop_icons_context.c`, `desktop_mouse.c`, `desktop_runtime.c`, `desktop_smoke_readiness.c`, `taskbar.c`, `taskbar_menu.c`, `taskbar_menu_input.c`, `internal/desktop_icons_internal.h`, `internal/taskbar_internal.h`.
- `src/window/` (window manager + dispatcher + notification): `notification.c`, `window_dispatcher.c`, `window_manager.c`.
- `src/apps/` (built-in apps): `calculator.c`, `file_manager.c`, `file_manager_dnd.c`, `file_manager_view.c`, `settings.c`, `settings_actions.c`, `settings_view.c`, `task_manager.c`, `text_editor.c`, `internal/file_manager_internal.h`, `internal/settings_internal.h`.
- `Makefile` ganha alvos `package-desktop-session`, `package-widget-core`, `package-index`, `package-clean`. `make package` agora produz `build/capypkg/org.capyos.ui.widget-core.{bin,manifest}`, `build/capypkg/org.capyos.ui.desktop-session.{bin,manifest}` e `build/capypkg/modules-index.txt`.
- Manifesto `org.capyos.ui.desktop-session` declara `depends=org.capyos.ui.widget-core` (enforced pelo adapter in-tree CapyOS).

### Mudado

- ABI `capy-ui-desktop-session` v1 agora é **propriedade deste repositório**. O fallback in-tree CapyOS (`CapyOS/src/gui/{desktop,window}/`, `CapyOS/src/apps/`) permanece apenas como migração-paridade; novos features pertencem aqui.
- `docs/compatibility.md` ganhou bloco "Desktop session contract (since 0.7, delivered in `alpha.241`)" e tabela "Owned ABIs" passa a listar `capy-ui-desktop-session`.

### ABI

- `capy-ui-widget` permanece **0.6** (núcleo retido portátil não muda).
- `capy-ui-desktop-session` **v1** estabelecida e entregue em `alpha.241`.
- Display-list schema permanece **2**.

### Docs

- `docs/compatibility.md` atualizado com seção "Desktop session contract" e bloco "Install/update boundary" expandido.
- `README.md` documenta os dois assets e o pinning por tag.

## [0.6.0] — 2026-05-19

**Marco:** fase de curto prazo (núcleo retido portátil) completa. Próximo trabalho (v0.7.0) aguarda gate CapyOS Etapa 4.

### Adicionado

- Sistema de tokens de tema v2 em `src/widget/capy_widget.c` e `.h`.
- `enum capy_token` com 17 entries: `CAPY_TOKEN_NONE` (reservado para "use literal"), `CAPY_TOKEN_BG_BASE/RAISED/SUNKEN`, `CAPY_TOKEN_FG_PRIMARY/MUTED/INVERSE`, `CAPY_TOKEN_ACCENT/ACCENT_HOVER`, `CAPY_TOKEN_BORDER/BORDER_FOCUS`, `CAPY_TOKEN_FOCUS_RING`, `CAPY_TOKEN_DANGER/WARNING/SUCCESS/INFO`, `CAPY_TOKEN_DISABLED`, `CAPY_TOKEN_COUNT`.
- Constantes de variant: `CAPY_THEME_VARIANT_LIGHT/DARK/HIGH_CONTRAST`.
- `struct capy_theme` (standalone): `tokens[CAPY_TOKEN_COUNT]`, `variant`, `high_contrast`, `version`.
- Campo aditivo `theme` em `struct capy_widget_context` (embutido por valor).
- Campos aditivos `bg_token`, `fg_token`, `border_token`, `reserved` em `struct capy_widget_style` (`uint8_t` cada).
- Ponteiro aditivo `const struct capy_theme *theme` em `struct capy_display_list` (NULL por default, set explícito para opt-in de theming).
- APIs públicas: `capy_theme_default_light`, `capy_theme_default_dark`, `capy_theme_high_contrast`, `capy_theme_apply`, `capy_theme_resolve`.
- Default themes documentados: light/dark com paleta moderna; high-contrast com BG=#000000 e FG=#FFFFFF (contraste 21:1, trivial WCAG AA).
- `capy_widget_emit_recursive` resolve cores via `capy_theme_resolve(dl->theme, w->style.*_token, w->style.*_color)` para RECT, BORDER, TEXT e caret RECT. FOCUS_RING mantém `style.active_color` literal.
- Backward compat: callers pré-0.6 (sem tokens setados, sem theme em dl) produzem display-list byte-idêntico ao pré-0.6.
- 7 novos testes em `tests/test_widget_contracts.c` cobrindo snapshot dos 3 defaults, apply em contexto, resolver edge cases (NULL/0/out-of-range), WCAG AA high-contrast, uso de theme quando token setado, backward compat com literal, troca de tema preservando estrutura de ops.

### ABI

- `capy-ui-widget` 0.5 → **0.6** (aditivo).
- Display-list schema permanece **2**. Theme é ponteiro opcional, não introduz nova op.

### Docs

- `docs/compatibility.md` ganhou bloco "Theme tokens v2 (since 0.6)".
- `docs/README.md` marca fatia 6 como entregue e curto prazo completo.
- `docs/roadmap/short-term/v0.6-theme-tokens-v2.md` marcado como `delivered`.

## [0.5.0] — 2026-05-19

### Adicionado

- Modelo de animação determinístico em `src/widget/capy_widget.c` e `.h`.
- `enum capy_anim_easing` com 4 valores: `CAPY_ANIM_LINEAR`, `CAPY_ANIM_EASE_IN`, `CAPY_ANIM_EASE_OUT`, `CAPY_ANIM_EASE_IN_OUT`.
- `struct capy_anim` standalone (não embutida em `capy_widget` em 0.5): campos `start_tick`, `duration_ticks`, `from`, `to`, `easing`, `active`, `reserved`.
- APIs: `capy_anim_start`, `capy_anim_sample`, `capy_widget_tick`.
- Easings implementadas em fixed-point 16.16 com intermediários `uint64_t`; **zero `float`/`double`** em qualquer hot path.
- Defesa contra clock retrocedido (`now <= start_tick` → retorna `from`) e duração zero (retorna `from`).
- `capy_widget_tick(root, now)` faz DFS walk determinístico; em 0.5 é no-op por widget (hook para futuras integrações).
- 10 novos testes em `tests/test_widget_contracts.c` cobrindo endpoints (start/end), inativo, duração zero, midpoint linear, monotonicidade das 4 easings, clock retrocedido, múltiplas animações independentes, determinismo e walk de árvore.

### ABI

- `capy-ui-widget` 0.4 → **0.5** (aditivo).
- Display-list schema permanece **2** (animação opera no modelo retido, fora do display-list).

### Docs

- `docs/compatibility.md` ganhou bloco "Animation and timing (since 0.5)".
- `docs/README.md` marca fatia 5 como entregue.
- `docs/roadmap/short-term/v0.5-animation-timing.md` marcado como `delivered`.

## [0.4.0] — 2026-05-19

### Adicionado

- Modelo de edição de texto em `src/widget/capy_widget.c` e `.h`.
- `struct capy_text_edit` (aditiva, embutida no final de `capy_widget`) com campos `caret`, `sel_start`, `sel_end`, `multiline`, `readonly`, `password`, `reserved`. Válida quando `type == TEXTBOX`.
- APIs: `capy_textbox_insert`, `capy_textbox_delete` (positivo = forward, negativo = backspace, ambos em UTF-8 chars), `capy_textbox_set_selection`, `capy_textbox_copy`, `capy_widget_ime_compose` (stub).
- Helpers internos: validador UTF-8 mínimo, `capy_utf8_prev_char_size`, `capy_utf8_char_size_at`, `capy_text_shift_left/right`, `capy_textbox_clamp_caret`.
- Selection ativa é substituída ao inserir; ao deletar com seleção, conteúdo selecionado é removido (count ignorado).
- `readonly = 1` bloqueia `insert` e `delete` (retornam `-1`); `copy` continua funcionando.
- `password = 1` emite `*` no display-list TEXT (um por caractere UTF-8); bytes originais nunca tocam o text_pool.
- Caret rendering: pequeno RECT (1 px de largura, altura = `font_size`) após `TEXT` para TEXTBOX `focused && !readonly && !password`. Posição usa fallback `caret * font_size / 2`.
- 10 novos testes em `tests/test_widget_contracts.c` cobrindo insert ASCII/UTF-8, overflow, delete de seleção, backspace UTF-8, password rendering, readonly, IME stub, UTF-8 inválido e determinismo.

### ABI

- `capy-ui-widget` 0.3 → **0.4** (aditivo).
- Display-list schema permanece **2** (password e caret usam `TEXT` e `RECT` existentes; sem op nova).

### Docs

- `docs/compatibility.md` ganhou bloco "Text editing (since 0.4)".
- `docs/README.md` marca fatia 4 como entregue.
- `docs/roadmap/short-term/v0.4-text-editing.md` marcado como `delivered`.

## [0.3.0] — 2026-05-19

### Adicionado

- Focus traversal em `src/widget/capy_widget.c` e `.h`.
- Campos aditivos em `struct capy_widget`: `uint8_t focusable`, `int16_t tab_index`.
- Campo aditivo em `struct capy_widget_event`: `uint32_t modifiers`.
- Constantes: `CAPY_KEY_TAB`, `CAPY_KEY_ENTER`, `CAPY_KEY_ESCAPE`, `CAPY_KEY_SPACE`, `CAPY_KEY_ARROW_UP/DOWN/LEFT/RIGHT`.
- Constantes de modificadores: `CAPY_MOD_NONE`, `CAPY_MOD_CTRL`, `CAPY_MOD_ALT`, `CAPY_MOD_SHIFT`, `CAPY_MOD_META`.
- APIs: `capy_widget_set_focusable`, `capy_widget_focus_next`, `capy_widget_focus_prev`, `capy_widget_clear_focus`, `capy_widget_get_focused`, `capy_widget_dispatch_key`.
- Algoritmo 2-pass DFS preorder com ordenação lexicográfica `(tab_index, dfs_order)` para travessia determinística.
- Defaults de focusable por tipo: `BUTTON`, `TEXTBOX`, `CHECKBOX`, `LIST`, `MENU_ITEM`, `TABS`.
- Op aditiva `CAPY_DL_FOCUS_RING` no display-list (schema 1 → 2). Emit após `BORDER` e antes de `TEXT` quando `widget->focused != 0`.
- 9 novos testes em `tests/test_widget_contracts.c` cobrindo defaults, traversal order, tab_index priority, skip non-traversable, prev wrap, dispatch TAB/SHIFT+TAB/ENTER/SPACE/ESC, e emissão de focus ring.

### ABI

- `capy-ui-widget` 0.2 → **0.3** (aditivo).
- Display-list schema 1 → **2** (aditivo).

### Docs

- `docs/compatibility.md` ganhou bloco "Focus traversal (since 0.3)".
- `docs/README.md` marca fatia 3 como entregue.
- `docs/roadmap/short-term/v0.3-focus-traversal.md` marcado como `delivered`.

## [0.2.0] — 2026-05-19

### Adicionado

- Display-list output em `src/widget/capy_display_list.h` e `.c`.
- `enum capy_dl_op` com 7 entries: `NONE`, `RECT`, `BORDER`, `TEXT`, `CLIP_PUSH`, `CLIP_POP`, `IMAGE_REF`.
- `struct capy_dl_cmd` e `struct capy_display_list` com schema versionado (`CAPY_DISPLAY_LIST_SCHEMA_VERSION = 1`).
- APIs `capy_display_list_init`, `capy_display_list_reset`, `capy_widget_emit`.
- Buffers de comandos e texto fornecidos pelo chamador (zero malloc interno).
- Emit determinístico com rollback em overflow.
- 4 novos testes em `tests/test_widget_contracts.c` cobrindo emit de árvore simples, balanço de clip push/pop, rollback em overflow e reset/texto.

### ABI

- `capy-ui-widget` 0.1 → **0.2** (aditivo).

### Docs

- `docs/compatibility.md` ganhou bloco "Display-list schema (since 0.2)".
- `docs/README.md` marca fatia 2 como entregue.

## [0.1.0] — 2026

### Adicionado

- Layout primitives em `src/widget/capy_layout.h` e `.c`.
- `enum capy_layout_kind`: `NONE`, `STACK`, `GRID`, `FLOW`.
- `struct capy_layout_constraints` e `capy_layout_node` embutidos em `capy_widget`.
- APIs `capy_widget_set_layout`, `capy_widget_measure`, `capy_widget_arrange`.
- 4 testes cobrindo stack vertical, grid 2x2, idempotência e constraints min/max.

### ABI

- `capy-ui-widget` 0.0 → **0.1** (aditivo).

### Docs

- `docs/compatibility.md` ganhou bloco "Layout contract (since 0.1)".

## [0.0.1] — 2026

### Adicionado

- Baseline do widget tree em `src/widget/capy_widget.h` e `.c`.
- Tipos de widget: `LABEL`, `BUTTON`, `TEXTBOX`, `CHECKBOX`, `LIST`, `PANEL`, `SCROLLBAR`, `MENUBAR`, `MENU_ITEM`, `DIALOG`, `PROGRESS`, `TABS`.
- Eventos abstratos: `POINTER_MOVE/DOWN/UP`, `KEY_DOWN/UP`.
- `capy_widget_style` com presets default/button/textbox.
- Allocator injetável `capy_widget_allocator`.
- Hit-testing `capy_widget_find_at` + roteamento `capy_widget_handle_event`.
- 2 testes baseline em `tests/test_widget_contracts.c`.
- `make validate` (lint + security + test + version-check) no Makefile.
- CI: `ci.yml` e `security.yml`.
- Compatibility doc: `docs/compatibility.md`, `docs/README.md`.

### ABI

- `capy-ui-widget` **0.0** (estabelecida).

## Formato

Seguir [Keep a Changelog](https://keepachangelog.com/) com seções `Adicionado`, `Mudado`, `Deprecado`, `Removido`, `Corrigido`, `Segurança` e bloco `ABI` ao final.

## Política

- Entradas imutáveis após release tag.
- Apenas a release de cabeçalho mais novo pode ser "Unreleased"; remover ao tagar.
- Cada entrada cita arquivos novos/modificados quando relevante.
