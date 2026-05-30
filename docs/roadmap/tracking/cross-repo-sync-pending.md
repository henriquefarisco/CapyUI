# Cross-repo sync pendente (CapyUI → CapyOS)

> Documento **temporário e vivo**. Lista as atualizações que precisam ser propagadas para repositórios sister (especificamente o `CapyOS`) por causa dos minors pós-freeze `v1.1.0`–`v1.10.0` (fase 1.x linear), do **major bump `v2.0.0`** e dos minors aditivos `v2.1.0` (advanced widgets), `v2.2.0` (virtualization plumbing), `v2.3.0` (undo/redo), `v2.4.0` (theme packs), `v2.5.0` (devtools/inspector), `v2.6.0` (display mode), `v2.7.0` (user management), `v2.8.0` (contrast/WCAG), `v2.9.0` (desktop icon arrangement), `v2.10.0` (file manager UX), `v2.11.0` (icon & thumbnail system), `v2.12.0` (wallpaper management) e `v2.13.0` (login screen plumbing — última do 2.x; fase 2.x COMPLETA 14/14) da ABI `capy-ui-widget`. Apagar este arquivo quando todas as linhas estiverem marcadas como `done` no lado CapyOS **e** uma `compatibility-audit-<data>.md` consolidada existir no CapyOS cobrindo até a tag corrente.

> **⚠️ Atenção especial em v1.5:** primeiro bump de display-list schema desde 0.9 (4 → **5**) com a op aditiva `CAPY_DL_DPI_SCOPE`. Smokes do CapyOS que decodificam o DL precisam tolerar ops 0..8 (incluindo DPI_SCOPE como skip); a compatibility matrix do CapyOS precisa citar o novo schema explicitamente.
>
> **⚠️ Atenção em v1.6:** 4 event types novos appended ao tail de `capy_widget_event_type` (`DRAG_BEGIN`/`DRAG_MOVE`/`DRAG_END`/`DROP`). O CapyOS dispatcher pode opcionalmente emitir esses eventos para habilitar drag-and-drop intra-app via core. Sem isso, o core continua se comportando exatamente como antes — nenhuma mudança obrigatória no dispatcher CapyOS.
>
> **⚠️ Atenção em v1.8:** mais 4 event types appended (`IME_COMPOSE_START`/`PREEDIT_UPDATE`/`COMMIT`/`CANCEL`) + 5 APIs `capy_ime_*`. CapyOS pode plugar engine real (mozc/anthy/hangul) via essas APIs para habilitar IME rico em TEXTBOX. Sem isso, o stub `capy_widget_ime_compose` de 0.4 continua funcionando.
>
> **⚠️ Atenção especial em v1.9:** **segundo bump de DL schema desde 0.9** (5 → **6**) com 2 ops aditivas `CAPY_DL_TRANSFORM_PUSH/POP`. `sizeof(capy_dl_cmd)` cresce 24 bytes (novo campo `transform`). Decodificadores de DL no CapyOS precisam tolerar ops 0..10 e ignorar ou aplicar TRANSFORM_PUSH/POP; smokes de compositor precisam recompilar contra o novo sizeof. Backends affine-aware aplicam a matriz; backends sem suporte podem skipar (render axis-aligned, visual degradado mas funcional).
>
> **🚨 MAJOR RELEASE v2.0.0:** primeiro **major bump** desde o freeze v1.0. **Quebra controlada conservadora**: nenhum símbolo do 1.x foi removido em 2.0.0, mas o major bump abre a porta para remoções em minors 2.x sob a `deprecation-policy.md`. **Terceiro bump de DL schema** (6 → **7**) com a op aditiva `CAPY_DL_PLUGIN_OP`. Nova superfície de plugin (`struct capy_plugin_descriptor`, `capy_plugin_register/unregister_all`, macros `CAPYUI_API_VERSION_TAG` e `CAPY_MAX_PLUGINS`). **`sizeof(capy_widget_context)` cresce** (8 pointer slots + count). **1.x permanece em LTS por ≥12 meses pós-2.0**.

**Última revisão:** 2026-05-29 (após `v2.19.0` — advanced widget state: chart dataset; 6ª fatia da trilha 2.14+ de estado dos advanced widgets)
**Tag CapyUI mais recente:** `v2.19.0` (ABI `capy-ui-widget` **2.19**, DL schema **7**)
**Tag CapyOS pinada no contrato CapyUI:** `0.8.0-alpha.261+20260529` (ver `docs/compatibility.md`; a matriz CapyOS carrega a row-resumo `capy-ui-widget` v2.19 / display-list schema v7 desde a sincronização cross-repo de 2026-05-29)

## Política

- Este workspace é **review/edit only** (ver memória `05-local-execution-policy.md`). Nenhum dos comandos abaixo deve ser invocado aqui — apenas documentados para um humano ou CI rodar no repositório alvo.
- A ABI `capy-ui-widget` **bumpou** dez vezes em 1.x (v1.1–1.10), **major-bumpou para 2.0** com plugin system + SDK público, **avançou para 2.1** com 8 advanced widget enum slots, **2.2** com virtualization data source plumbing, **2.3** com undo/redo per-context, **2.4** com theme packs binário canonical LE, **2.5** com devtools/inspector read-only, **2.6** com display mode plumbing + event `DISPLAY_MODE_CHANGED`, **2.7** com user management plumbing (`capy_user_account` + `capy_user_directory`), **2.8** com contrast/WCAG refinement (`capy_contrast_finding` + audit API + 4ª factory `default_dark_high_contrast`), **2.9** com desktop icon arrangement (`capy_desktop_layout` + `capy_desktop_icon_position` + blob `CDLA`), **2.10** com file manager UX plumbing (`capy_breadcrumb_truncate` + toolbar taxonomy), **2.11** com icon & thumbnail system (`capy_icon_provider` + `THUMBNAIL_READY` event), **2.12** com wallpaper management (blob `CWLP` + slideshow + fit modes + per-monitor), e **2.13** com login screen plumbing (`capy_login_choose_layout` + taxonomia layout/power). **Fase 2.x major CONCLUÍDA**; próximo marco é a fase 3.x (Vision platform). **Fase 1.x linear concluída; 1.x em LTS ≥12m pós-2.0**. **Fase 2.x COMPLETA com 14/14 fatias entregues** (sem remoções; deprecation policy ARMADA para minors 2.x). Por convenção do contrato (regra `30-validation-gates.md`), **toda mudança de versão de ABI** dispara: atualização do compatibility matrix CapyOS, atualização do integration contract CapyOS e uma audit row nova.
- v1.1–1.4 não tocaram no display-list schema (permaneceu `4`); o formato do manifest, o nome canônico dos módulos e o gate do kernel também não mudaram. **v1.5 introduziu a primeira mudança estrutural no schema desde 0.9 (4 → 5)** com a op aditiva `CAPY_DL_DPI_SCOPE`. **v1.6 adicionou 4 event types abstratos** (DRAG_BEGIN/MOVE/END/DROP) appended ao tail do enum `capy_widget_event_type` — hosts antigos que não emitem esses tipos não veem mudança comportamental. Manifest, kernel gate e nomes de módulo permanecem estáveis em todos os bumps.

## Ações requeridas no `CapyOS`

### 1. `docs/reference/integration/compatibility-matrix.md`

Adicionar (ou atualizar) a linha da ABI `capy-ui-widget` para refletir as três versões aditivas. Modelo:

```
| capy-ui-widget | 1.5 | additive over 1.0 frozen baseline | delivered |
  | + 1.1 damage tracking (dirty_version, frame_budget_microseconds,
  |   invalidate_subtree, dirty_version, frame_budget,
  |   display_list_diff_incremental)
  | + 1.2 GPU translator (capy_dl_gpu.h, capy_gpu_quad, dl_to_quads,
  |   internal scissor stack max depth 16, DL schema still 4)
  | + 1.3 rich animation (capy_anim_keyframe, capy_anim_track,
  |   capy_spring, anim_track_sample, spring_step, anim_bezier_eval,
  |   zero float, strict C11)
  | + 1.4 gesture engine single-touch (capy_gesture_kind enum w/ reserved
  |   PINCH/ROTATE, capy_gesture, capy_gesture_recognizer, recognizer_init,
  |   gesture_feed(ev, now), gesture_tick(now), gesture_poll, Chebyshev
  |   distance, zero float)
  | + 1.5 multi-display + DPI (capy_widget_context.dpi_scale_x256 + reserved_dpi,
  |   capy_display_list.dpi_scale_x256 + reserved_dpi, set/get/scale_dim APIs,
  |   new CAPY_DL_DPI_SCOPE op, **DL schema 4 → 5**, layout auto-scale deferred)
  | + 1.6 drag and drop (4 new event types DRAG_BEGIN/MOVE/END/DROP appended,
  |   capy_dnd_payload struct, 5 widget tail fields, set_draggable/set_drop_target/
  |   set_on_drop APIs, type_matches/dnd_accepts helpers, DROP routed deepest-first,
  |   DRAG_* no-ops in core; preview/cancel handled by host dispatcher)
  | + 1.7 performance tier 2 — slab allocator (struct capy_slab + init/alloc/free,
  |   LIFO free-list inline in freed slots; compression + hash measure-cache
  |   deferred to a future minor)
  | + 1.8 rich IME — struct capy_ime_state, 9 widget tail fields,
  |   5 APIs (set_preedit/set_candidates/commit/cancel/get_state),
  |   4 event types (IME_COMPOSE_START/PREEDIT_UPDATE/COMMIT/CANCEL),
  |   stub capy_widget_ime_compose preserved; candidate window deferred)
  | + 1.9 transforms 2D — struct capy_dl_transform (2×3 Q16.16 affine),
  |   field on capy_dl_cmd (+24 bytes), tail on capy_widget, 5 factories
  |   + multiply + set_transform, 2 new ops CAPY_DL_TRANSFORM_PUSH/POP,
  |   **DL schema 5 → 6**; arbitrary-angle rotation + hit-test deferred)
  | + 1.10 state serialization — new source capy_widget_serialize.c,
  |   3 APIs (serialize/deserialize/serialize_text), 8 public constants,
  |   canonical LE binary format with FNV-1a 32-bit checksum,
  |   crash-recovery foundation for hosts)

| capy-ui-widget | **2.0** | major bump conservador (sem remoções) | delivered |
  | Plugin system + SDK público:
  |   struct capy_plugin_descriptor + struct capy_plugin_context,
  |   capy_plugin_register / capy_plugin_unregister_all,
  |   CAPYUI_API_VERSION_TAG (0x00020000) + CAPY_MAX_PLUGINS (8),
  |   plugins[] + plugin_count tail fields no capy_widget_context.
  |   New DL op CAPY_DL_PLUGIN_OP (DL schema **6 → 7**, terceiro bump).
  |   SDK seed em sdk/ (README + plugin-template.c).
  |   LTS 1.x ≥12m sob deprecation-policy.md. |

| capy-ui-widget | **2.1** | aditivo (enum-only) | delivered |
  | Advanced widgets:
  |   8 enum slots (TREE/TABLE/RICH_TEXT/CANVAS/CHART/
  |   COLOR_PICKER/DATE_PICKER/AUTOCOMPLETE).
  |   Per-widget state e APIs dedicadas deferidos para fatias 2.x focadas.
  |   Deserialize (1.10) atualiza type-byte ceiling para AUTOCOMPLETE.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.2** | aditivo (plumbing-only) | delivered |
  | Virtualization data source:
  |   struct capy_virtual_data_source (typedefs get_count_fn/get_item_fn),
  |   tail field virtual_source em capy_widget (NULL = eager pre-2.2),
  |   3 APIs (set_virtual_source/virtual_count/virtual_get_item),
  |   event type CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE.
  |   Viewport-only emit + row recycling deferidos para fatia futura.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.3** | aditivo | delivered |
  | Undo/redo per-context:
  |   struct capy_undo_entry + struct capy_undo_stack,
  |   tail field undo_stack em capy_widget_context (NULL pre-init),
  |   6 APIs (init/push/undo/redo/can_undo/can_redo),
  |   FIFO eviction + branch-cut em push,
  |   coalesce_window_ms metadata (default 500).
  |   Coalescing automático + auto-hook TEXTBOX/RICH_TEXT deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.4** | aditivo | delivered |
  | Theme packs (escopo binário-de-cores):
  |   formato canonical LE ".captheme" (magic "CTHM", format_version=1),
  |   header 16B + entries 6B,
  |   2 APIs (capy_theme_pack_validate / capy_theme_pack_load),
  |   7 macros (MAGIC0..3, FORMAT_VERSION, HEADER_SIZE, ENTRY_SIZE),
  |   FNV-1a checksum + reserved fields fail-closed,
  |   unknown token_ids silently skipados (forward-compat).
  |   Hot reload, Ed25519 signature, font/icon refs, metadata textual deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.5** | aditivo | delivered |
  | Devtools/inspector (escopo introspecção read-only):
  |   struct capy_inspector_node + struct capy_perf_counters,
  |   2 APIs read-only (capy_widget_inspect / capy_perf_counters_snapshot),
  |   12 macros (CAPY_INSPECTOR_NO_PARENT + 11 flag bits),
  |   tree flatten depth-first pre-order espelha emit,
  |   caller-owned buffers, fail-closed em overflow com rollback.
  |   Dump textual de tree/DL, event recording/replay e headless harness deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.6** | aditivo | delivered |
  | Display mode (escopo plumbing):
  |   struct capy_display_mode + struct capy_display_controller,
  |   2 typedefs (enum_modes_fn, set_mode_fn),
  |   tail field display_controller em capy_widget_context (NULL = ausente),
  |   4 APIs (set_display_controller / enum_modes / set_mode / current_mode),
  |   event aditivo CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED,
  |   2 macros (FLAG_PREFERRED, FLAG_INTERLACED).
  |   Fractional refresh (Q24.8), HDR/VRR, rollback UI 15s e persistência per-user deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.7** | aditivo | delivered |
  | User management UI (escopo plumbing):
  |   struct capy_user_account + struct capy_user_directory,
  |   5 typedefs (list/create/update/del/set_avatar),
  |   tail field user_directory em capy_widget_context (NULL = ausente),
  |   6 APIs (set_user_directory / list / create / update / delete / set_avatar),
  |   2 macros (CAPY_USER_NAME_MAX=32, CAPY_USER_DISPLAY_NAME_MAX=64).
  |   CapyUI nunca armazena password; uid==0 (root) undeletable.
  |   Authentication, groups/roles e audit log deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.8** | aditivo | delivered |
  | Contrast & accessibility refinement (escopo WCAG-style ratios):
  |   struct capy_contrast_finding (fg_token + bg_token + ratio_x1000 + passes_aa/aaa),
  |   3 APIs (capy_theme_contrast_ratio_x1000 / capy_theme_audit_wcag /
  |     capy_theme_default_dark_high_contrast),
  |   3 macros (AA_X1000=4500, AAA_X1000=7000, VARIANT_DARK_HIGH_CONTRAST=3),
  |   cálculo zero-float γ≈2.0, ratio clamp [1000, 21000],
  |   audit walk 11 canonical pairs, dry-run via cap=0,
  |   4ª factory built-in com todos pairs ≥ 7:1 (AAA).
  |   WCAG 3 APCA, large-text threshold, per-locale palettes deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.9** | aditivo | delivered |
  | Desktop icon arrangement (escopo plumbing):
  |   struct capy_desktop_layout + struct capy_desktop_icon_position,
  |   3 APIs (capy_desktop_layout_validate / load / serialize),
  |   14 macros (MAGIC0..3, FORMAT_VERSION=1, HEADER_SIZE=24, ENTRY_SIZE=16,
  |     SORT_MANUAL..SIZE + COUNT=5, FLAG_ALIGN_RIGHT, FLAG_VERTICAL_FIRST),
  |   blob canonical LE "CDLA" com FNV-1a checksum + reserved fail-closed,
  |   short-buffer detection via *out_count > cap.
  |   Multi-monitor envelope, group folders, locale collation, drag hit-test in core deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.10** | aditivo | delivered |
  | File manager UX plumbing (escopo math + taxonomia):
  |   struct capy_breadcrumb_segment (id + text_offset/len + icon_image_id + is_clickable + reserved),
  |   1 API determinística (capy_breadcrumb_truncate — root + tail + dropdown flag, floor em 2),
  |   6 macros (CAPY_FILE_MGR_COMPACT_THRESHOLD_PX=600,
  |     CAPY_TOOLBAR_GROUP_NAV/VIEW/ACTION/LAYOUT/COUNT),
  |   cálculo zero-float, zero-alloc, mesmo input → mesma saída.
  |   Variable-width per-segment measurement, multi-line, locale-aware ellipsis,
  |     per-item toolbar descriptors deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.11** | aditivo | delivered |
  | Icon & thumbnail system (escopo plumbing):
  |   struct capy_icon_provider (resolve + thumbnail_request + user_data),
  |   2 typedefs (capy_icon_resolve_fn, capy_icon_thumbnail_request_fn),
  |   tail field icon_provider em capy_widget_context (NULL = host-less),
  |   4 APIs (set_icon_provider / icon_resolve / thumbnail_request / thumbnail_ready),
  |   event type aditivo CAPY_WIDGET_EVENT_THUMBNAIL_READY
  |     (event->x=request_id, event->y=image_id).
  |   Fallback determinístico image_id=0 quando sem provider/host -1.
  |   request_id=0 reservado.
  |   Icon theme cascade, vector icons, cancellation, group folder stacks deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.12** | aditivo | delivered |
  | Wallpaper management (escopo plumbing):
  |   struct capy_wallpaper_config + struct capy_wallpaper_slide,
  |   3 APIs (capy_wallpaper_validate / load / serialize),
  |   13 macros (4 magic "CWLP" + 3 sizes + 5 fit modes + COUNT=5 + 2 flags),
  |   blob canonical LE "CWLP" com FNV-1a checksum + reserved fail-closed,
  |   per-slide guards (image_id > 0, duration_sec > 0),
  |   short-buffer detection via *out_count > cap.
  |   Live wallpaper, per-locale defaults, transition effects deferidos.
  |   DL schema permanece 7. |

| capy-ui-widget | **2.13** | aditivo (última do 2.x) | delivered |
  | Login screen plumbing (escopo minimalista):
  |   12 macros (4 layout: SINGLE/GRID/LIST + COUNT;
  |     2 thresholds: GRID_THRESHOLD=2 / LIST_THRESHOLD=7;
  |     7 power: NONE/SHUTDOWN/REBOOT/SLEEP/LOCK/LOGOUT + COUNT),
  |   1 API determinística (capy_login_choose_layout(user_count) →
  |     SINGLE | GRID | LIST), total-function, zero-alloc, zero-float.
  |   Taxonomia compartilhada por login screen + lock screen + user switcher.
  |   Biometric/SSO, PIN keypad, locked-badge animation, wake-screen deferidos para 3.x.
  |   DL schema permanece 7. **FASE 2.x COMPLETA (14/14)**. |

| capy-ui-widget | **2.14** | aditivo (1ª fatia de estado dos advanced widgets) | delivered |
  | Advanced widget state — date picker:
  |   struct capy_date (year u16 / month u8 / day u8; 0 = unset),
  |   tail field date_value em capy_widget (válido só p/ DATE_PICKER; sizeof +4B),
  |   4 APIs (capy_date_is_valid, capy_widget_set_date/clear_date/get_date).
  |   Calendário Gregoriano proléptico (bissexto 4/100/400), fail-closed,
  |   zero-alloc/zero-float, total. capy_widget_serialize NÃO muda.
  |   Macros 2/14/0; CAPYUI_API_VERSION_TAG = 0x00020E00.
  |   DL schema permanece 7. +8 testes (273→281). |

| capy-ui-widget | **2.15** | aditivo (2ª fatia de estado dos advanced widgets) | delivered |
  | Advanced widget state — color picker:
  |   tail fields picker_color (0xAARRGGBB) + picker_color_set em capy_widget
  |   (válido só p/ COLOR_PICKER; sizeof 816→824),
  |   4 APIs (capy_color_pack, capy_widget_set_color/clear_color/get_color).
  |   Cast-before-shift (sem UB no byte de alpha), fail-closed por tipo,
  |   zero-alloc/zero-float, total. capy_widget_serialize NÃO muda.
  |   Macros 2/15/0; CAPYUI_API_VERSION_TAG = 0x00020F00.
  |   DL schema permanece 7. +8 testes (281→289). |

| capy-ui-widget | **2.16** | aditivo (3ª fatia de estado dos advanced widgets) | delivered |
  | Advanced widget state — table columns:
  |   tail fields table_column_widths (caller-owned const uint16_t*) +
  |   table_column_count em capy_widget (válido só p/ TABLE; sizeof 824→840),
  |   4 APIs (capy_widget_set_table_columns/clear_table_columns/
  |   table_column_count/table_column_width).
  |   Array caller-owned (zero-alloc, CapyUI nunca copia/aloca/libera),
  |   acessor bounds-checked / fail-closed (index >= count -> -1),
  |   zero-float, total. capy_widget_serialize NÃO serializa o modelo.
  |   Macros 2/16/0; CAPYUI_API_VERSION_TAG = 0x00021000.
  |   DL schema permanece 7. +8 testes (289→297). |

| capy-ui-widget | **2.17** | aditivo (4ª fatia de estado dos advanced widgets) | delivered |
  | Advanced widget state — autocomplete suggestions:
  |   tail fields autocomplete_items (caller-owned const char *const *) +
  |   autocomplete_count + autocomplete_selected (int32, -1 = nenhum) em
  |   capy_widget (válido só p/ AUTOCOMPLETE; sizeof 840→856),
  |   6 APIs (capy_widget_set_autocomplete/clear_autocomplete/
  |   autocomplete_count/autocomplete_item/set_autocomplete_selected/
  |   get_autocomplete_selected).
  |   Lista caller-owned (zero-alloc, idioma do drop_accepted_types),
  |   acessor bounds-checked + cursor de seleção com clamp/fail-closed
  |   (index < -1 ou index >= count -> -1; seleção clampada na leitura),
  |   zero-float, total. capy_widget_serialize NÃO serializa lista nem seleção.
  |   Macros 2/17/0; CAPYUI_API_VERSION_TAG = 0x00021100.
  |   DL schema permanece 7. +8 testes (297→305). |

| capy-ui-widget | **2.18** | aditivo (5ª fatia de estado dos advanced widgets) | delivered |
  | Advanced widget state — tree hierarchy:
  |   tail fields tree_collapsed (uint8, 0 = expandido default) +
  |   tree_depth (uint16, indent) em capy_widget (válido só p/ TREE;
  |   sizeof 856→864),
  |   5 APIs (capy_widget_set_tree_collapsed/tree_is_collapsed/
  |   set_tree_depth/tree_depth/tree_row_visible).
  |   tree_row_visible caminha a cadeia de pais (fail-closed): 0 se algum
  |   ancestral TREE está collapsed (só ancestrais escondem; o próprio fold
  |   do nó não o esconde), 1 caso contrário. Zero-alloc, zero-float, total.
  |   capy_widget_serialize NÃO serializa fold/depth (UI efêmero).
  |   Macros 2/18/0; CAPYUI_API_VERSION_TAG = 0x00021200.
  |   DL schema permanece 7. +8 testes (305→313). |

| capy-ui-widget | **2.19** | aditivo (6ª fatia de estado dos advanced widgets) | delivered |
  | Advanced widget state — chart dataset:
  |   tail fields chart_values (caller-owned const int32_t*) +
  |   chart_count em capy_widget (válido só p/ CHART; sizeof 864→880),
  |   5 APIs (capy_widget_set_chart_data/clear_chart_data/chart_count/
  |   chart_value/chart_range).
  |   Array caller-owned (zero-alloc, CapyUI nunca copia/aloca/libera),
  |   acessor bounds-checked + redução chart_range (scan single-pass do
  |   min/max assinado; 1 com dados, 0 vazio com out zerado, -1 erro),
  |   zero-float, total. capy_widget_serialize NÃO serializa o dataset.
  |   Macros 2/19/0; CAPYUI_API_VERSION_TAG = 0x00021300.
  |   DL schema permanece 7. +8 testes (313→321). |
```

(Adaptar à formatação real do arquivo no momento da edição. As entradas das ABIs 1.1 e 1.2 podem já existir mas com texto pré-1.3.)

> **Ação CapyOS para 2.14 + 2.15 + 2.16 + 2.17 + 2.18 + 2.19 (nenhuma mudança obrigatória de comportamento):**
> Todas as seis ABI puramente aditivas, DL schema inalterado (7), manifest/módulos/gate
> inalterados. Requer apenas: (1) bump da row-resumo `capy-ui-widget` para
> **v2.19** na `compatibility-matrix.md`; (2) opcionalmente listar no integration
> contract, sob "since 2.14".."2.19", os símbolos `capy_date_*`/
> `capy_widget_*_date` (2.14), `capy_color_pack`/`capy_widget_*_color` (2.15),
> `capy_widget_*_table_columns`/`capy_widget_table_column_*` (2.16),
> `capy_widget_*_autocomplete*`/`capy_widget_autocomplete_*` (2.17),
> `capy_widget_*_tree_*`/`capy_widget_tree_*` (2.18) e
> `capy_widget_*_chart_*`/`capy_widget_chart_*` (2.19);
> (3) nova audit row `compatibility-audit-2026-05-29.md` cobrindo até `v2.19.0`.
> `sizeof(capy_widget)` cresce de 808 (2.13) → 816 (2.14) → 824 (2.15) → 840 (2.16)
> → 856 (2.17) → 864 (2.18) → 880 (2.19) — recompilar consumidores; nenhum realloc de
> buffer de DL é necessário (o widget é alocado pelo allocator do host). Os modelos
> caller-owned da 2.16 (colunas), 2.17 (sugestões) e 2.19 (dataset do gráfico) seguem a
> posse do host; a 2.18 (fold/indent da árvore) é estado de UI interno ao widget.

### 2. `docs/reference/integration/capyui-widget-integration-contract.md`

Se o documento enumera símbolos públicos por bloco "since X.Y", anexar:

**since 1.1:**

- `dirty_version` (tail field em `capy_widget`).
- `frame_budget_microseconds` (tail field em `capy_widget_context`).
- `capy_widget_invalidate_subtree(w)`.
- `capy_widget_dirty_version(w)`.
- `capy_widget_frame_budget(ctx)`.
- `capy_display_list_diff_incremental(prev, next, out_dirty, cap)`.

**since 1.2:**

- Header `capy_dl_gpu.h`.
- `struct capy_gpu_quad`.
- Macro `CAPY_DL_GPU_CLIP_STACK_MAX` (= `16u`).
- `capy_dl_to_quads(dl, out, cap, out_count)`.

**since 1.3:**

- `struct capy_anim_keyframe`.
- `struct capy_anim_track` (loop policy: `0` once + clamp, `-1` infinito, `N>0` repetir N vezes + clamp).
- `struct capy_spring` (Q24.8 velocity, Q8.8 stiffness/damping, plain-units position/target).
- `capy_anim_track_sample(track, now, &out)`.
- `capy_spring_step(s, dt_ms)` (1 ms symplectic-Euler substeps; converge com damping > 0, oscila com damping == 0).
- `capy_anim_bezier_eval(p0, p1, p2, p3, t_q16)` (de Casteljau Q16.16; endpoints exatos; monotônico para pontos monotônicos).

**since 1.4:**

- `enum capy_gesture_kind` (13 entries: `NONE`, `TAP`, `DOUBLE_TAP`, `LONG_PRESS`, `SWIPE_LEFT/RIGHT/UP/DOWN`, `PINCH_IN/OUT` (reserved), `ROTATE_CW/CCW` (reserved), `DRAG`).
- `struct capy_gesture { uint8_t kind; uint8_t reserved[3]; capy_ui_point start, end; int32_t magnitude; uint32_t duration_ms; }`.
- `struct capy_gesture_recognizer` (caller-embedded, ≈80 bytes, depth-1 output queue, tunable thresholds).
- `capy_gesture_recognizer_init(r)` — defaults: tap 200 ms / 10 px, dbl-tap gap 300 ms, long-press 500 ms, swipe 50 px.
- `capy_gesture_feed(r, ev, now)` — single-touch detector sobre TOUCH_BEGIN/MOVE/END; outros tipos ignorados; segunda BEGIN concorrente ignorada.
- `capy_gesture_tick(r, now)` — emite LONG_PRESS sem novo evento.
- `capy_gesture_poll(r, &out)` — drena queue depth-1.

**since 1.5:**

- `capy_widget_context.dpi_scale_x256` (Q8.8 tail field, default `256` = 1.0×) + `reserved_dpi` slot.
- `capy_display_list.dpi_scale_x256` (Q8.8 tail field, default `256`) + `reserved_dpi` slot.
- `capy_widget_set_dpi_scale(ctx, scale_x256)` — clampa zero para 1; NULL no-op.
- `capy_widget_get_dpi_scale(const ctx)` — NULL ou zero retornam 256.
- `capy_widget_dpi_scale_dim(scale_x256, value)` — helper Q8.8 puro com saturating clamp int32.
- Op aditiva `CAPY_DL_DPI_SCOPE` em `capy_dl_op` (`rect` = scoped region; `image_id` = scale).
- **`CAPY_DISPLAY_LIST_SCHEMA_VERSION` bumpado 4 → 5**.
- `capy_widget_emit` prepende DPI_SCOPE no início do DL quando `dl->dpi_scale_x256 != 256`; quando `== 256`, byte-equivalente a pre-1.5.
- GPU translator (`capy_dl_to_quads`) reconhece DPI_SCOPE e não emite quad para ele.

**since 1.6:**

- 4 event types novos em `capy_widget_event_type` (appended tail): `CAPY_WIDGET_EVENT_DRAG_BEGIN`/`DRAG_MOVE`/`DRAG_END`/`DROP`.
- `struct capy_dnd_payload { char type[32]; const void *data; uint32_t len; }`.
- 5 widget tail fields aditivos (`drag_payload`, `drop_accepted_types`, `drop_types_count`, `on_drop`, `drop_user_data`).
- 3 setters: `capy_widget_set_draggable(w, payload)`, `capy_widget_set_drop_target(w, types, count)`, `capy_widget_set_on_drop(w, fn, user_data)`.
- 2 matchers: `capy_dnd_type_matches(accepted, type)` (case-insensitive `*`/prefix/exact), `capy_widget_dnd_accepts(w, payload)`.
- `capy_widget_handle_event` agora rota `DROP` deepest-first; DRAG_BEGIN/MOVE/END são no-ops no core (host dispatcher possui o ciclo de preview/cancel).

**since 1.7:**

- `struct capy_slab { void *buffer; uint32_t size, element_size, high_water; void *free_list; }`.
- `capy_slab_init(s, buf, size, element_size)`, `capy_slab_alloc(s)`, `capy_slab_free(s, ptr)`.
- Fail-closed em `element_size < sizeof(void *)`, NULL buffer, ou size==0 — `s->size` clampa para 0 e `_alloc` retorna NULL.
- Free-list LIFO inline (cada slot livre guarda um `void *` para o próximo).
- Companion ao `capy_widget_pool` (since 0.15): pool para arenas, slab para tight loops com reuse.
- Display-list compression e hash measure-cache do plano v1.7 original foram **adiados** (já cobertos parcialmente por `capy_widget_diff`/0.8, `capy_display_list_diff_incremental`/1.1 e `layout_dirty`/0.15).

**since 1.8:**

- `struct capy_ime_state` (preedit + candidates + composition_phase).
- 9 widget tail fields aditivos em `capy_widget` (válidos só em TEXTBOX): `ime_preedit`, `ime_preedit_len`, `ime_cursor_pos`, `ime_candidates`, `ime_candidate_count`, `ime_selected_index`, `ime_composition_phase`, `ime_reserved[3]`.
- 5 APIs: `capy_ime_set_preedit`, `capy_ime_set_candidates`, `capy_ime_commit`, `capy_ime_cancel`, `capy_ime_get_state`.
- 4 event types novos (appended): `CAPY_WIDGET_EVENT_IME_COMPOSE_START`/`PREEDIT_UPDATE`/`COMMIT`/`CANCEL` — pass-through no `capy_widget_handle_event`.
- Stub `capy_widget_ime_compose` (since 0.4) **preservado**; coexiste com o modelo de 1.8.

**since 1.9:**

- `struct capy_dl_transform { int32_t m00, m01, m02, m10, m11, m12; }` (declarada em `capy_display_list.h`).
- Novo field `struct capy_dl_transform transform` em `capy_dl_cmd` (24 bytes adicionais).
- Widget tail field `const struct capy_dl_transform *transform` (NULL=identity).
- 6 APIs: `capy_transform_identity`/`scale`/`translate`/`rotate_quadrants`/`multiply` + `capy_widget_set_transform`.
- 2 ops aditivas: `CAPY_DL_TRANSFORM_PUSH`, `CAPY_DL_TRANSFORM_POP`.
- **`CAPY_DISPLAY_LIST_SCHEMA_VERSION` bumpado 5 → 6**.
- Arbitrary-angle rotation e hit-test compensation **deferidos** — hosts populam matriz própria via `multiply` ou `set_transform`.

**since 1.10:**

- Novo source `src/widget/capy_widget_serialize.c` em `SRC_WIDGET`.
- 8 constantes públicas: `CAPY_STATE_MAGIC0..3`, `CAPY_STATE_FORMAT_VERSION` (1), `CAPY_STATE_HEADER_SIZE` (16), 5 flag bits (`VISIBLE`/`ENABLED`/`FOCUSED`/`FOCUSABLE`/`CHECKED`).
- 3 APIs: `capy_widget_serialize`, `capy_widget_deserialize`, `capy_widget_serialize_text`.
- Formato binário canonical LE (magic `"CUIS"` + version + reserved + FNV-1a checksum + node_count) com per-node pre-order.
- Foundation para crash recovery, hot reload, devtools snapshot, undo/redo (v2.3), time-travel debug (v2.5).

**since 2.13:**

- 12 macros novas: 4 layout (`CAPY_LOGIN_LAYOUT_SINGLE` = 0, `_GRID` = 1, `_LIST` = 2, `_COUNT` = 3); 2 thresholds (`_GRID_THRESHOLD` = 2, `_LIST_THRESHOLD` = 7); 7 power (`CAPY_LOGIN_POWER_NONE` = 0, `_SHUTDOWN` = 1, `_REBOOT` = 2, `_SLEEP` = 3, `_LOCK` = 4, `_LOGOUT` = 5, `_COUNT` = 6).
- 1 API: `uint8_t capy_login_choose_layout(uint32_t user_count)`.
- Total function: `0|1 → SINGLE`, `2..6 → GRID`, `≥7 → LIST`. Zero-alloc, zero-float, determinístico.
- Plumbing minimalista — widget core não autentica (v2.7), não compõe background (v2.12), não decode avatares (v2.11). Desktop session em `src/desktop/login.c` consome a math.
- Compartilhada por 3 surfaces (login, lock, switcher) via mesma API.
- **Biometric/SSO challenge UI, PIN keypad widget, locked-badge animation e wake-screen deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020D00`.
- **FASE 2.x COMPLETA com este bloco (14/14)**.

**since 2.12:**

- 13 macros novas: `CAPY_WALLPAPER_MAGIC0..3` (`'C','W','L','P'`), `CAPY_WALLPAPER_FORMAT_VERSION` (=1), `_HEADER_SIZE` (=24), `_ENTRY_SIZE` (=8), `CAPY_WALLPAPER_FIT_STRETCH/FILL/FIT/CENTER/TILE` + `_COUNT` (=5), `CAPY_WALLPAPER_FLAG_RANDOM` (=0x01), `_PER_MONITOR` (=0x02).
- 2 structs novas: `capy_wallpaper_config` (default_image_id, slideshow_interval_sec, monitor_index, fit_mode, flags, reserved), `capy_wallpaper_slide` (image_id, duration_sec, reserved).
- 3 APIs: `capy_wallpaper_validate(buf, len)`, `capy_wallpaper_load(buf, len, *out_config, *out_slides, cap, *out_count)`, `capy_wallpaper_serialize(*config, *slides, count, *out, cap)`.
- Blob canonical LE `CWLP` com FNV-1a 32-bit checksum sobre body. Header 24 bytes + slides 8 bytes.
- Per-slide guards: `image_id > 0` (reserva 0 = silhouette v2.11), `duration_sec > 0`, `reserved == 0`.
- `monitor_index = 0` = global, `>= 1` = per-monitor (compõe com v2.6 display mode).
- Plumbing-only: widget core nunca compõe pixels, nunca agenda timer, nunca lê arquivo.
- **Live wallpaper, per-locale defaults, transition effects e image format negotiation deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020C00`.

**since 2.11:**

- 2 typedefs novos: `capy_icon_resolve_fn(user_data, mime_type, extension, *out_icon_id) → int`, `capy_icon_thumbnail_request_fn(user_data, path, *out_request_id) → int`.
- 1 struct nova: `capy_icon_provider` (resolve, thumbnail_request, user_data).
- Tail field `struct capy_icon_provider *icon_provider` em `capy_widget_context` (NULL = host-less).
- 1 event type aditivo: `CAPY_WIDGET_EVENT_THUMBNAIL_READY` (appended ao tail do enum). Payload: `event->x = (int32_t)request_id`, `event->y = (int32_t)image_id`.
- 4 APIs: `capy_widget_set_icon_provider`, `capy_icon_resolve`, `capy_icon_thumbnail_request`, `capy_icon_thumbnail_ready`.
- `image_id = 0` reservado para silhouette default; `request_id = 0` reservado como sentinel.
- Plumbing-only: widget core nunca decode PNG/JPEG, nunca abre arquivo. Host implementa via callbacks.
- **Icon theme cascade, vector (SVG/PDF), cancellation, stack/group folder descriptors deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020B00`.

**since 2.10:**

- 6 macros novas: `CAPY_FILE_MGR_COMPACT_THRESHOLD_PX` (=600), `CAPY_TOOLBAR_GROUP_NAV` (=0), `_VIEW` (=1), `_ACTION` (=2), `_LAYOUT` (=3), `_COUNT` (=4).
- 1 struct nova: `capy_breadcrumb_segment` (id, text_offset, text_len, icon_image_id, is_clickable, reserved).
- 1 API: `capy_breadcrumb_truncate(in, in_count, available_width_px, segment_avg_px, out, cap, *out_count, *out_dropdown)`.
- Algoritmo: full-fit (copy all, dropdown=0) ou root+tail com dropdown=1 + floor em 2 segmentos quando budget apertado.
- Plumbing-only: widget core publica math + taxonomy; UX concreta no `src/apps/file_manager.c`.
- **Variable-width per-segment measurement, multi-line, locale-aware ellipsis e per-item descriptors deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020A00`.

**since 2.9:**

- 14 macros novas: `CAPY_DESKTOP_LAYOUT_MAGIC0..3` (`'C','D','L','A'`), `CAPY_DESKTOP_LAYOUT_FORMAT_VERSION` (=1), `_HEADER_SIZE` (=24), `_ENTRY_SIZE` (=16), `CAPY_DESKTOP_SORT_MANUAL/NAME/TYPE/MTIME/SIZE` + `_COUNT` (=5), `CAPY_DESKTOP_LAYOUT_FLAG_ALIGN_RIGHT` (=0x01), `_VERTICAL_FIRST` (=0x02).
- 2 structs novas: `capy_desktop_layout` (cell_w, cell_h, snap, auto_arrange, sort_by, flags), `capy_desktop_icon_position` (icon_id, x, y, grid_x, grid_y, pinned, reserved[3]).
- 3 APIs: `capy_desktop_layout_validate(buf, len)`, `capy_desktop_layout_load(buf, len, *out_layout, *out_positions, cap, *out_count)`, `capy_desktop_layout_serialize(*layout, *positions, count, *out, cap)`.
- Blob canonical LE `CDLA` com FNV-1a 32-bit checksum sobre body. Header 24 bytes + entries 16 bytes.
- `*out_count` sempre carrega declared count — callers detectam short buffer via `*out_count > cap`.
- Plumbing-only: widget core nunca abre arquivo, nunca dirige drag, nunca decide posição.
- **Multi-monitor envelope, group folders, locale collation e drag hit-test in core deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020900`.

**since 2.8:**

- 3 macros novas: `CAPY_CONTRAST_AA_X1000` (=4500), `CAPY_CONTRAST_AAA_X1000` (=7000), `CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST` (=3).
- 1 struct nova: `capy_contrast_finding` (fg_token, bg_token, ratio_x1000, passes_aa, passes_aaa, reserved[2]).
- 3 APIs: `capy_theme_contrast_ratio_x1000(fg, bg)`, `capy_theme_audit_wcag(theme, out, cap)`, `capy_theme_default_dark_high_contrast(void)`.
- Cálculo zero-float γ≈2.0: `lin = c * c * 65536 / 65025`; weighted luminance `(2126*R + 7152*G + 722*B) / 10000`; ratio `(L_max + 3277) * 1000 / (L_min + 3277)` clamp `[1000, 21000]`.
- Audit walk de 11 canonical pairs (FG_PRIMARY/BG_BASE, FG_PRIMARY/BG_RAISED, FG_MUTED/BG_BASE, FG_INVERSE/ACCENT, BORDER_FOCUS/BG_BASE, FOCUS_RING/BG_BASE, DANGER/BG_BASE, WARNING/BG_BASE, SUCCESS/BG_BASE, INFO/BG_BASE, DISABLED/BG_BASE).
- `cap == 0` dry-run retorna total de pairs (11) sem escrever — hosts sizing o buffer com 1 chamada.
- 4ª factory built-in `default_dark_high_contrast` (variant byte 3): BG=#000000, FG=#FFFFFF, todos pairs canon ≥ 7:1.
- **WCAG 3 APCA, large-text threshold (3.0), per-locale palettes e auto-fix suggestions deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020800`.

**since 2.7:**

- 2 macros novas: `CAPY_USER_NAME_MAX` (=32), `CAPY_USER_DISPLAY_NAME_MAX` (=64).
- 1 struct nova: `capy_user_account` (username, display_name, uid, is_admin, is_locked, avatar_image_id, last_login_ms).
- 5 typedefs novos (callbacks no host): `capy_user_list_fn`, `capy_user_create_fn`, `capy_user_update_fn`, `capy_user_delete_fn`, `capy_user_set_avatar_fn`.
- 1 struct nova: `capy_user_directory` (5 callbacks + user_data).
- Tail field `struct capy_user_directory *user_directory` em `capy_widget_context` (NULL = surface ausente).
- 6 APIs: `capy_widget_set_user_directory`, `capy_user_list`, `capy_user_create`, `capy_user_update`, `capy_user_delete`, `capy_user_set_avatar`.
- Plumbing-only: CapyUI nunca autentica, nunca hashea password, nunca toca `/etc/passwd`. Host CapyOS implementa callbacks via backend real.
- `password` em `create` é forwardado verbatim; widget core nunca o copia/armazena.
- Guards no widget core: empty `username[0]` (create), `uid==0` (delete root), `len>0 && png==NULL` (set_avatar).
- **Authentication (`capy_user_authenticate_fn` para v2.13 login), groups/roles ACLs e audit log completo deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020700`.

**since 2.6:**

- 2 macros novas: `CAPY_DISPLAY_MODE_FLAG_PREFERRED` (0x01u), `CAPY_DISPLAY_MODE_FLAG_INTERLACED` (0x02u).
- 1 struct nova: `capy_display_mode` (width, height, refresh_hz_q8 Q8.0, bpp, flags).
- 2 typedefs: `capy_display_enum_modes_fn(user_data, out, cap) → int`, `capy_display_set_mode_fn(user_data, *mode) → int`.
- 1 struct nova: `capy_display_controller` (callbacks + cached current_mode + has_current).
- Tail field `struct capy_display_controller *display_controller` em `capy_widget_context` (NULL = surface ausente).
- 1 event type aditivo: `CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED` com payload `event->x = (width<<16)|height`, `event->y = (bpp<<16)|refresh_hz_q8`.
- 4 APIs: `capy_widget_set_display_controller`, `capy_display_enum_modes`, `capy_display_set_mode`, `capy_display_current_mode`.
- Plumbing-only: CapyUI nunca toca framebuffer/DRM/KMS. Host CapyOS implementa callbacks via adapter de mode-setting.
- **Fractional refresh (Q24.8), HDR/VRR metadata, rollback/preview UI 15s e persistência per-user deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020600`.

**since 2.5:**

- 12 macros novas: `CAPY_INSPECTOR_NO_PARENT` (`0xFFFFFFFFu`) + 11 flag bits (`VISIBLE`, `ENABLED`, `FOCUSED`, `FOCUSABLE`, `CHECKED`, `HOVERED`, `LAYOUT_DIRTY`, `HAS_TRANSFORM`, `HAS_VIRTUAL_SRC`, `HAS_DRAG_PAYLOAD`, `HAS_DROP_TARGET`).
- 2 structs novas: `capy_inspector_node` (16 campos por node) e `capy_perf_counters` (snapshot do ctx + widget count).
- 2 APIs: `capy_widget_inspect(root, out_nodes, cap, text_pool, text_cap, &node_count, &text_used)`, `capy_perf_counters_snapshot(ctx, root, &out)`.
- Read-only sobre tree/ctx; zero allocation; caller-owned buffers.
- Tree flatten depth-first pre-order espelha `capy_widget_emit_recursive` — ordem estável cross-run.
- Fail-closed: NULL guards + capacity/text overflow → -1 com rollback de `*out_node_count`/`*out_text_used` para 0.
- **Dump textual de tree/DL, event recording/replay, headless harness CLI e IDE protocol deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020500`.

**since 2.4:**

- 7 macros novas: `CAPY_THEME_PACK_MAGIC0..3` (`'C','T','H','M'`), `CAPY_THEME_PACK_FORMAT_VERSION` (=1), `CAPY_THEME_PACK_HEADER_SIZE` (=16), `CAPY_THEME_PACK_ENTRY_SIZE` (=6).
- 2 APIs: `capy_theme_pack_validate(buf, len)`, `capy_theme_pack_load(buf, len, out)`.
- Formato binário canonical LE com FNV-1a 32-bit checksum (mesma função do `CUIS` v1.10).
- Caller seeds `*out` com `default_light/dark/high_contrast` antes; tokens ausentes preservados.
- Forward-compat: entries com `token_id >= CAPY_TOKEN_COUNT` silently skipped.
- **Hot reload, Ed25519 signature, font/icon refs, metadata textual deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020400`.

**since 2.3:**

- `struct capy_undo_entry` (caller-owned action_id + redo/undo data ptrs + lengths + timestamp_ms).
- `struct capy_undo_stack` (entries + capacity + count + redo_count + coalesce_window_ms).
- Tail field `undo_stack` em `capy_widget_context` (NULL pre-init).
- 6 APIs: `capy_undo_stack_init`, `capy_undo_push`, `capy_undo_undo`, `capy_undo_redo`, `capy_undo_can_undo`, `capy_undo_can_redo`.
- FIFO eviction + branch-cut em push; `coalesce_window_ms` informational (default 500).
- **Coalescing automático + auto-hook TEXTBOX/RICH_TEXT deferidos**.
- `CAPYUI_API_VERSION_TAG` agora `0x00020300`.

**since 2.2:**

- 2 typedefs: `capy_virtual_get_count_fn`, `capy_virtual_get_item_fn`.
- `struct capy_virtual_data_source { get_count, get_item, user_data; }`.
- Tail field `const struct capy_virtual_data_source *virtual_source` em `capy_widget` (NULL = eager rendering pre-2.2).
- 3 APIs: `capy_widget_set_virtual_source`, `capy_widget_virtual_count`, `capy_widget_virtual_get_item`.
- Event type aditivo `CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE` (host emite, core pass-through).
- **Viewport-only emit + row recycling deferidos** para fatia futura.
- `CAPYUI_API_VERSION_TAG` agora `0x00020200`.

**since 2.1:**

- 8 entradas aditivas em `enum capy_widget_type`: `CAPY_WIDGET_TREE`, `CAPY_WIDGET_TABLE`, `CAPY_WIDGET_RICH_TEXT`, `CAPY_WIDGET_CANVAS`, `CAPY_WIDGET_CHART`, `CAPY_WIDGET_COLOR_PICKER`, `CAPY_WIDGET_DATE_PICKER`, `CAPY_WIDGET_AUTOCOMPLETE`.
- `capy_widget_serialize.c` widens deserialize ceiling para `CAPY_WIDGET_AUTOCOMPLETE`.
- Per-widget state + APIs dedicadas deferidas para fatias 2.x focadas.
- `CAPYUI_API_VERSION_TAG` agora `0x00020100`.

**since 2.0:**

- `CAPYUI_API_VERSION_MAJOR` 1 → **2**; `_MINOR` 10 → 0; `_PATCH` permanece 0.
- Macro composta nova: `CAPYUI_API_VERSION_TAG` (`= 0x00020000`).
- Macro nova: `CAPY_MAX_PLUGINS` (`= 8`).
- `struct capy_plugin_descriptor` + `struct capy_plugin_context`.
- 2 APIs: `capy_plugin_register(ctx, &desc)` (valida ABI mismatch + duplicate + capacity; retorna slot index ou `-1`), `capy_plugin_unregister_all(ctx)` (chama destroy + reset; idempotente).
- 2 widget context tail fields: `plugins[CAPY_MAX_PLUGINS]` + `plugin_count`. `capy_widget_context_init` zera ambos.
- Op aditiva `CAPY_DL_PLUGIN_OP` em `capy_dl_op`; carrega 32-byte payload nos campos existentes; `image_id` é o plugin id channel.
- **`CAPY_DISPLAY_LIST_SCHEMA_VERSION` bumpado 6 → 7**.
- GPU translator (`capy_dl_to_quads`) recogniza PLUGIN_OP e skipa (metadata para host renderer).
- SDK seed em `sdk/`: `README.md` + `plugin-template.c`.

Macros bumpadas: `CAPYUI_API_VERSION_MAJOR=2`, `_MINOR=0`, `_PATCH=0`.

### 3. `docs/reference/integration/compatibility-audit-<YYYY-MM-DD>.md`

Criar audit nova consolidando as três bumps. Conteúdo mínimo:

- **Date:** data do audit.
- **CapyUI version audited:** `2.13.0` (cobre 1.0–1.10 + 2.0–2.13 — fase 2.x COMPLETA 14/14; v1.5 = 1º bump DL schema; v1.6 = DRAG_*; v1.7 = slab; v1.8 = IME_*; v1.9 = 2º bump DL schema; v1.10 = state serialization; **v2.0 = major bump conservador** com 3º bump DL schema + plugin surface; v2.1 = 8 advanced widget enums; v2.2 = virtualization data source plumbing + VIRTUAL_REQUEST_RANGE event; v2.3 = undo/redo per-context (caller-buffer-backed); v2.4 = theme packs binário canonical LE + FNV-1a; v2.5 = devtools/inspector read-only (inspector_node + perf_counters); v2.6 = display mode plumbing (capy_display_mode + capy_display_controller + DISPLAY_MODE_CHANGED event); v2.7 = user management plumbing (capy_user_account + capy_user_directory + 5 callbacks no host); v2.8 = contrast/WCAG refinement (capy_contrast_finding + capy_theme_contrast_ratio_x1000 + capy_theme_audit_wcag + capy_theme_default_dark_high_contrast factory); v2.13 = login screen plumbing (capy_login_choose_layout + power taxonomy); v2.12 = wallpaper management (capy_wallpaper_config/slide + CWLP blob); v2.11 = icon & thumbnail system (capy_icon_provider + THUMBNAIL_READY event); v2.10 = file manager UX plumbing (capy_breadcrumb_truncate + toolbar taxonomy); v2.9 = desktop icon arrangement (capy_desktop_layout + capy_desktop_icon_position + CDLA blob); 1.x em LTS ≥12m).
- **CapyOS version:** versão atual do `CapyOS` na hora do audit.
- **Result:** confirmação de que o adapter `services/capypkg` consome `org.capyos.ui.widget-core` versão `2.13.0` sem erro, que o desktop session continua ativando via marker existente e que os smokes `compositor-damage-track`, `apps-basic-roundtrip`, (se existir) `touch-gestures`, qualquer smoke novo que valide o `CAPY_DL_DPI_SCOPE`, e qualquer smoke que sintetize `CAPY_WIDGET_EVENT_DROP` continuam verdes.
- **Diffs revisados:**
  - `capy_widget` struct: +1 tail field (`dirty_version`, uint32_t, 1.1) — não impacta layout dos campos pré-1.0.
  - `capy_widget_context`: +1 tail field (`frame_budget_microseconds`, uint32_t, 1.1).
  - Novas structs em 1.3 (`capy_anim_keyframe`, `capy_anim_track`, `capy_spring`) e em 1.4 (`capy_gesture`, `capy_gesture_recognizer`) — todas caller-embedded ou caller-owned, não tocam structs pré-existentes.
  - Novo enum em 1.4 (`capy_gesture_kind`) com PINCH/ROTATE reservados — adições aditivas só no enum novo.
  - 1.5: tail fields aditivos `dpi_scale_x256` + `reserved_dpi` em `capy_widget_context` e `capy_display_list` (não impactam layout dos campos pré-existentes).
  - 1.5: nova op `CAPY_DL_DPI_SCOPE` (apended ao enum `capy_dl_op`) reusa o slot `image_id` para carregar o scale; **`sizeof(capy_dl_cmd)` inalterado**.
  - 1.6: 4 event types novos appended ao tail de `capy_widget_event_type` (DRAG_BEGIN/MOVE/END/DROP); `struct capy_dnd_payload` nova; 5 widget tail fields aditivos. `sizeof(capy_widget)` cresce, mas é caller-allocated, então hosts recompilam.
  - 1.7: `struct capy_slab` nova (caller-embedded; sem mudança em structs pré-existentes); 3 APIs novas; zero impact em call sites antigos.
  - 1.8: 9 widget tail fields aditivos (`ime_*`) + 4 event types novos + `struct capy_ime_state` nova; `sizeof(capy_widget)` cresce mas widget é caller-allocated.
  - 1.9: `struct capy_dl_transform` nova + novo field `transform` em `capy_dl_cmd` (+24 bytes), widget tail field `transform`, 2 ops novas, 6 APIs. **`sizeof(capy_dl_cmd)` cresce 24 bytes**.
  - 1.10: novo source `capy_widget_serialize.c` + 3 APIs + 8 constantes; sem mudança em structs existentes nem no DL.
  - **2.0: major bump conservador.** `struct capy_plugin_descriptor`/`capy_plugin_context` novas, 2 APIs novas (`capy_plugin_register`/`unregister_all`), 2 macros novas (`CAPYUI_API_VERSION_TAG`/`CAPY_MAX_PLUGINS`), 2 tail fields em `capy_widget_context` (`plugins[8]` + `plugin_count` — sizeof cresce). Op aditiva `CAPY_DL_PLUGIN_OP` (**DL schema 6 → 7**). **Sem remoções de APIs 1.x**; deprecation policy ARMADA para minors 2.x.
  - **2.1: aditivo enum-only.** 8 entradas novas no `enum capy_widget_type` (`TREE`/`TABLE`/`RICH_TEXT`/`CANVAS`/`CHART`/`COLOR_PICKER`/`DATE_PICKER`/`AUTOCOMPLETE`). Sem campos struct novos, sem APIs novas, sem mudanças no DL. Deserialize ceiling widened para `CAPY_WIDGET_AUTOCOMPLETE`.
  - **2.2: aditivo plumbing-only.** `struct capy_virtual_data_source` nova + 2 typedefs + tail field `virtual_source` em `capy_widget` + 3 APIs + event type `VIRTUAL_REQUEST_RANGE` (appended ao tail do enum). Sem mudanças no DL; sem rewriting do emit (viewport emit deferido para fatia futura).
  - **2.3: aditivo undo/redo.** `struct capy_undo_entry` + `struct capy_undo_stack` novas; tail field `undo_stack` em `capy_widget_context`; 6 APIs novas. Storage é caller-buffer; FIFO eviction quando cheio; branch cut em push; `coalesce_window_ms` metadata. Coalescing automático + auto-hook TEXTBOX/RICH_TEXT deferidos.
  - **2.4: aditivo theme packs.** 7 macros novas + 2 APIs novas (`capy_theme_pack_validate`/`capy_theme_pack_load`). Sem novos campos struct, sem mudanças no DL, sem novos events. Pack vive em buffer caller-owned. Forward-compat para token_ids futuros. Hot reload + Ed25519 + font/icon refs deferidos para minors futuros.
  - **2.5: aditivo devtools.** 12 macros + 2 structs (`capy_inspector_node`, `capy_perf_counters`) + 2 APIs read-only (`capy_widget_inspect`, `capy_perf_counters_snapshot`). Sem mudanças em structs existentes, no DL ou no enum de events. Buffers caller-owned. Read-only; nenhuma mutaria sobre tree/ctx. Dump textual + event recording/replay + headless harness deferidos.
  - **2.6: aditivo display mode.** 2 macros + 1 struct (`capy_display_mode`) + 2 typedefs + 1 struct (`capy_display_controller`) + 1 tail field em `capy_widget_context` (NULL pre-2.6) + 1 event type appended (`DISPLAY_MODE_CHANGED`) + 4 APIs. Plumbing-only; CapyUI nunca toca framebuffer/DRM. Cached `current_mode` + `has_current`. Fractional refresh + HDR/VRR + rollback UI + persistência per-user deferidos.
  - **2.7: aditivo user management.** 2 macros + 1 struct (`capy_user_account`) + 5 typedefs + 1 struct (`capy_user_directory`) + 1 tail field em `capy_widget_context` (NULL pre-2.7) + 6 APIs. Plumbing-only; CapyUI nunca autentica/hashea password. Guards: empty username, uid==0 (root), `len>0 && png==NULL`. Authentication + groups/roles + audit log deferidos.
  - **2.8: aditivo contrast/WCAG.** 3 macros + 1 struct (`capy_contrast_finding`) + 3 APIs + 1 nova factory built-in (`default_dark_high_contrast`, variant byte 3). Sem campos struct novos, sem mudanças em `capy_widget`/`capy_widget_context`, sem novos events. Cálculo zero-float; ratios clamp [1000, 21000]; audit walk de 11 canonical pairs. WCAG 3 APCA + large-text threshold + per-locale palettes deferidos.
  - **2.9: aditivo desktop icon arrangement.** 14 macros + 2 structs (`capy_desktop_layout`, `capy_desktop_icon_position`) + 3 APIs (`validate`/`load`/`serialize`). Sem campos struct novos em `capy_widget`/`capy_widget_context`, sem mudanças no DL, sem novos events. Blob `CDLA` (header 24B + entries 16B) com FNV-1a checksum. Plumbing-only; widget core nunca abre arquivo / nunca dirige drag. Multi-monitor envelope + group folders + locale collation + drag hit-test in core deferidos.
  - **2.10: aditivo file manager UX.** 6 macros + 1 struct (`capy_breadcrumb_segment`) + 1 API determinística (`capy_breadcrumb_truncate`). Sem mudanças em structs existentes, no DL ou em events. Plumbing-only; widget core publica math + taxonomy de toolbar groups. Variable-width measurement + multi-line + locale-aware ellipsis + per-item descriptors deferidos.
  - **2.11: aditivo icon & thumbnail.** 2 typedefs + 1 struct (`capy_icon_provider`) + 1 tail field em `capy_widget_context` (NULL pre-2.11) + 4 APIs + 1 event type appended (`THUMBNAIL_READY`). Sem mudanças em structs existentes ou no DL. Plumbing-only; widget core nunca decode PNG/JPEG. Fallback determinístico `image_id=0`. Icon theme cascade + vector + cancellation + stacks deferidos.
  - **2.12: aditivo wallpaper management.** 13 macros + 2 structs (`capy_wallpaper_config`, `capy_wallpaper_slide`) + 3 APIs (`validate`/`load`/`serialize`). Sem campos struct novos em `capy_widget`/`capy_widget_context`, sem mudanças no DL, sem novos events. Blob `CWLP` (header 24B + slides 8B) com FNV-1a checksum + reserved + per-slide guards. Plumbing-only; widget core nunca compõe pixels / nunca agenda timer / nunca lê arquivo. Live wallpaper + per-locale defaults + transition effects + image format negotiation deferidos.
  - **2.13: aditivo login screen.** 12 macros + 1 API determinística (`capy_login_choose_layout`). Sem campos struct novos em `capy_widget`/`capy_widget_context`, sem mudanças no DL, sem novos events. Plumbing minimalista — math pura compartilhada por 3 surfaces (login, lock, switcher). Biometric/SSO + PIN keypad + locked-badge animation + wake-screen deferidos para 3.x. **Última fatia da fase 2.x; fase 2.x COMPLETA 14/14**.
  - **Display-list schema: 5 → 6** em 1.9 (segundo bump desde 0.9). Consumers que leem `dl->version < 6` devem ignorar TRANSFORM_PUSH/POP.
  - **Display-list schema: 6 → 7** em 2.0 (terceiro bump desde 0.9). Consumers que leem `dl->version < 7` devem ignorar `CAPY_DL_PLUGIN_OP`.
  - Nenhum struct existente reordenado, nenhum campo removido, nenhum manifest field novo.
  - Manifest format permanece o de `alpha.241`.
- **Conclusão:** baseline 1.0 preservada; CapyOS pode subir o pin de `org.capyos.ui.widget-core` para `2.13.0` sem alteração de código no dispatcher (DRAG_*/IME_*/VIRTUAL_* events são no-ops; slab/IME/transform/virtual_source/undo_stack state são caller-embedded; plugin registry só ativa se host registrar; PLUGIN_OP só emite se host emit; advanced widget types 2.1 só usados se host criar; virtualization data source 2.2 só ativa se host attach; undo stack 2.3 só ativa se host chama `_init`; theme packs 2.4 só ativa se host chama `capy_theme_pack_load`; devtools 2.5 é read-only e só corre se host chama `_inspect`/`_snapshot`; display mode 2.6 só ativa se host chama `set_display_controller` com callbacks próprios; user management 2.7 só ativa se host chama `set_user_directory` com backend de autenticação; contrast/WCAG 2.8 é read-only sobre theme tokens, sem mudanças comportamentais em hosts que não chamam `audit_wcag`; login screen 2.13 é math pura + taxonomy de constantes — hosts só ativam se chamarem `capy_login_choose_layout`; wallpaper management 2.12 é byte-blob round-trip puro — hosts só ativam se chamarem `_serialize`/`_load`; icon & thumbnail 2.11 só ativa se host chama `set_icon_provider` (resolve cai em `image_id=0` sem provider, thumbnail_request retorna -1); file manager UX 2.10 é math pura + taxonomy de constantes — hosts só ativam se chamarem `capy_breadcrumb_truncate`; desktop icon arrangement 2.9 é byte-blob round-trip puro — hosts só ativam se chamarem `_serialize`/`_load`). Major bump 1.x→2.0 é source-compatível modulo 3 asserts de schema literal; 2.0→2.1→2.2→2.3→2.4→2.5→2.6→2.7→2.8→2.9 puramente aditivo. **1.x permanece em LTS por ≥12 meses**: CapyOS pode pinar v1.10.0 enquanto integra plugin system + advanced widgets gradualmente em branch de testing. Decodificadores de DL no CapyOS precisam adicionar caso `DPI_SCOPE` (skip ou aplicar) **se** quiserem tirar proveito do scope; caso contrário o op é silenciosamente ignorado pelos translators portados (incluindo `capy_dl_to_quads`). Para habilitar drag-and-drop intra-app, o dispatcher CapyOS sintetiza `CAPY_WIDGET_EVENT_DROP` com `event->payload = const struct capy_dnd_payload *`.

### 4. (Opcional) `docs/reference/integration/external-core-repositories.md`

Se este documento lista a tag mais recente publicada pelo CapyUI (rolling `latest` + frozen semver), atualizar a linha de "frozen semver" para `v2.13.0` — última do major 2.x (a `latest` é automática via `release-artifacts.yml`). **CapyOS pode preferir pinar v1.10.0 (LTS) em vez de v2.x enquanto integra plugin system + advanced widgets + virtualization + undo/redo em branch de testing.**

## Gates externos a recomendar

Quando o humano/CI puxar este workspace e quiser fechar o ciclo, rodar **fora** desta máquina:

| Gate | Repo | Justificativa |
|---|---|---|
| `make validate` | CapyUI | Lint + security + **272 testes** + version-check. Obrigatório por release. |
| Push para `main` | CapyUI | Dispara `.github/workflows/release-artifacts.yml`, republica rolling `latest`. |
| Tag `v2.13.0` (anotada/assinada — última do 2.x) | CapyUI | Cuts frozen aditivo no GitHub. **Fecha o major 2.x**. CapyOS first-boot pode pinar v1.10.0 (LTS) **ou** v2.13.0 (última do 2.x com surface completa: plugin + advanced widgets + virtualization + undo/redo + theme packs + devtools + display mode + user mgmt + contrast/WCAG + desktop layout + file mgr UX + icon system + wallpaper + login). Próximo marco é v3.0.0 com quebra controlada. |
| Comunicado público de major release | CapyUI/CapyOS | Anuncia o major bump v2.0, deprecation policy ARMADA, e LTS de 1.x por ≥12 meses. |
| `make all64 PROFILE=full` | CapyOS | Compila CapyOS com CapyUI sibling em `../CapyUI` na nova tag. |
| `make all64 PROFILE=core-only` | CapyOS | Confirma que o gate `kernel_module_desktop_session_available()` continua retornando 0 sem o marker. |
| `make modules-index` | CapyOS | Só se a lista de módulos publicados muda. **Não muda em v1.1/v1.2/v1.3** — opcional. |
| `make test-capypkg` | CapyOS | Só se manifest format muda. **Não muda** — opcional. |
| `make smoke-x64-vmware-compositor-damage-track` | CapyOS | Recomendado por v1.1 (damage tracking) e v1.2 (GPU translator). |
| `make smoke-x64-vmware-apps-basic-roundtrip` | CapyOS | Recomendado para garantir que apps continuam roteando após bump. v1.4 também se beneficia (touch payload). |
| `make smoke-x64-vmware-touch-gestures` (se existir) | CapyOS | Recomendado para v1.4. Se ainda não existir, criar um smoke mínimo que sintetize TOUCH_BEGIN/MOVE/END pelo dispatcher e verifique que o recognizer emite os gestos esperados (multi-touch ainda **não**). |
| `make smoke-x64-vmware-dpi-scope` (se existir) | CapyOS | Recomendado para v1.5. Se ainda não existir, criar um smoke mínimo que ative `dpi_scale_x256 != 256` no DL e verifique que o decodificador do compositor vê o `DPI_SCOPE` como primeiro op com o scale correto. Caso o compositor ainda não suporte DPI, basta verificar que o decoder ignora a op silenciosamente (forward-compat). |
| `make smoke-x64-vmware-drag-and-drop` (se existir) | CapyOS | Recomendado para v1.6. Se ainda não existir, criar um smoke mínimo que sintetize `CAPY_WIDGET_EVENT_DROP` com um `capy_dnd_payload` sobre um target configurado e verifique que `on_drop` dispara. Sem isso, basta confirmar que dispatchers existentes (que não emitem DRAG_*) continuam funcionando byte-equivalente. |
| Benchmark 100k widgets / 16 ms | CapyOS (CI) | Alvo do plano v1.7. Mede `capy_widget_measure` + `arrange` + `emit` numa árvore de 100k widgets; gate de regressão ≤ 10% vs baseline v0.15. Não executável neste workspace; recomendado como gate de CI longo-prazo. |
| Engine IME real (mozc / anthy / hangul-engine / etc.) | CapyOS | Recomendado para v1.8. Bridge engine → `capy_ime_set_preedit`/`set_candidates`/`commit`/`cancel`. Sem isso, IME rico fica disponível no ABI mas não ativado no produto. |
| `make smoke-x64-vmware-transform-roundtrip` (se existir) | CapyOS | Recomendado para v1.9. Se ainda não existir, criar um smoke mínimo que atribua um `capy_dl_transform` a um widget, emite o DL, e verifica TRANSFORM_PUSH (primeiro op) + TRANSFORM_POP (último op) com matriz correta. Compositor affine-aware aplica; sem suporte, basta confirmar skip silencioso (forward-compat). |
| `make smoke-x64-vmware-state-roundtrip` (se existir) | CapyOS | Recomendado para v1.10. Se ainda não existir, criar um smoke mínimo que serializa uma árvore, escreve em `/var/state/`, reinicializa o desktop session e deserializa para confirmar crash recovery. Sem isso, basta validar que existing flows continuam funcionando byte-equivalente (as APIs novas são opt-in). |
| `make smoke-x64-vmware-plugin-register` (se existir) | CapyOS | Recomendado para v2.0. Se ainda não existir, criar um smoke mínimo que registra o `plugin-template.c` do SDK, valida `ctx.plugin_count == 1`, e chama `unregister_all`. Sem isso, basta confirmar que dispatchers existentes (que não registram plugins) continuam funcionando byte-equivalente. |
| `make smoke-x64-vmware-advanced-widgets` (se existir) | CapyOS | Recomendado para v2.1. Se ainda não existir, criar um smoke mínimo que cria um widget de cada tipo novo (TREE/TABLE/RICH_TEXT/CANVAS/CHART/COLOR_PICKER/DATE_PICKER/AUTOCOMPLETE), valida `widget->type` e emite o display-list sem crash. Atualizar também o a11y role mapper do CapyOS para mapear os 8 novos types ao invés de cair no default. |
| `make smoke-x64-vmware-virtualization-source` (se existir) | CapyOS | Recomendado para v2.2. Se ainda não existir, criar um smoke mínimo que attach um data source a um LIST com 1k items, chama `virtual_count` (== 1000), itera `virtual_get_item` num range e valida bytes. Emit O(viewport) chega quando o follow-up fatia entregar o rewriting do `capy_widget_emit_recursive`. |
| `make smoke-x64-vmware-undo-redo` (se existir) | CapyOS | Recomendado para v2.3. Se ainda não existir, criar um smoke mínimo que inicializa um undo stack em um buffer fixo, push + undo + redo no dispatcher CapyOS, valida `can_undo`/`can_redo` transitions. Auto-hook TEXTBOX/RICH_TEXT é follow-up fatia. |
| `make smoke-x64-vmware-theme-pack` (se existir) | CapyOS | Recomendado para v2.4. Se ainda não existir, criar um smoke mínimo que monta um pack binário com 3–5 tokens, valida via `capy_theme_pack_validate`, carrega via `capy_theme_pack_load` num `capy_theme_default_light()` e confirma que os tokens correspondentes foram sobrescritos enquanto outros permaneceram com defaults. Hot reload + Ed25519 ficam para follow-up. |
| `make smoke-x64-vmware-devtools-inspect` (se existir) | CapyOS | Recomendado para v2.5. Se ainda não existir, criar um smoke mínimo que constrói um tree pequeno (panel → button + label), chama `capy_widget_inspect` num buffer de 8 nodes + 64 bytes de texto, valida `node_count`, `text_used` e a ordem depth-first. Em paralelo, chama `capy_perf_counters_snapshot` e confirma que `widget_count` bate com o número real. Dump textual + recording/replay ficam para follow-up. |
| `make smoke-x64-vmware-display-mode` (se existir) | CapyOS | Recomendado para v2.6. Se ainda não existir, criar um smoke mínimo que: (a) atacha um `capy_display_controller` com callbacks mock que enumeram 2–3 modos (1920×1080@60, 2560×1440@144, 3840×2160@60); (b) chama `capy_display_enum_modes` e valida o catalog; (c) chama `capy_display_set_mode` com um mode válido e captura o event `DISPLAY_MODE_CHANGED` no root; (d) confirma que `capy_display_current_mode` retorna o mode aplicado. Adapter real de mode-setting (DRM/KMS no Linux, equivalente no microkernel CapyOS) fica para fatia subsequente. |
| `make smoke-x64-vmware-user-management` (se existir) | CapyOS | Recomendado para v2.7. Se ainda não existir, criar um smoke mínimo que: (a) atacha um `capy_user_directory` com callbacks mock backed por um array em memória; (b) `capy_user_list` enumera o roster; (c) `capy_user_create` adiciona uma conta nova; (d) `capy_user_delete(0)` rejeitado, `capy_user_delete(uid_normal)` aceito; (e) `capy_user_set_avatar` ok com bytes, ok com `(NULL, 0)`, falha com `(NULL, >0)`. Backend de autenticação real (PAM, libsodium ou equivalente CapyOS) + groups/roles + audit log ficam para fatia subsequente. |
| `make smoke-x64-vmware-contrast-audit` (se existir) | CapyOS | Recomendado para v2.8. Se ainda não existir, criar um smoke mínimo que: (a) cria os 4 themes built-in (`default_light`/`default_dark`/`high_contrast`/`default_dark_high_contrast`); (b) chama `capy_theme_audit_wcag(theme, NULL, 0)` para confirmar count==11 em cada; (c) chama `capy_theme_audit_wcag(default_dark_high_contrast, findings, 11)` e verifica `passes_aaa==1` em todos os pairs; (d) sanity check `capy_theme_contrast_ratio_x1000(0xFFFFFFFF, 0xFF000000) == 21000`. WCAG 3 APCA + auto-fix UI ficam para fatia subsequente. |
| `make smoke-x64-vmware-desktop-layout` (se existir) | CapyOS | Recomendado para v2.9. Se ainda não existir, criar um smoke mínimo que: (a) monta uma `capy_desktop_layout` com cell=96 + snap=1 + alguns `capy_desktop_icon_position` em memória; (b) serializa via `capy_desktop_layout_serialize`, escreve em `/var/state/capyui/desktop-layout.bin`; (c) reaplica via `capy_desktop_layout_load` e confirma round-trip byte-equivalente. Drag interativo + sort + auto-arrange ficam no app de desktop. |
| `make smoke-x64-vmware-breadcrumb-truncate` (se existir) | CapyOS | Recomendado para v2.10. Se ainda não existir, criar um smoke mínimo que: (a) monta um caminho 8-segmento com `capy_breadcrumb_segment[]` (Home > Documents > Projects > CapyOS > ...); (b) chama `capy_breadcrumb_truncate(in, 8, 300, 100, out, 8, &n, &dropdown)` e valida `dropdown==1` + `n==3` + `out[0]` = root + `out[2]` = current. Toolbar item descriptors + multi-line breadcrumb ficam para fatia subsequente. |
| `make smoke-x64-vmware-icon-thumbnail` (se existir) | CapyOS | Recomendado para v2.11. Se ainda não existir, criar um smoke mínimo que: (a) atacha um `capy_icon_provider` com callbacks mock (`resolve` retorna image_id por mime); (b) chama `capy_icon_resolve(ctx, "image/png", "png", &id)` e valida; (c) chama `capy_icon_thumbnail_request(ctx, "/x.png", &req)` e captura req_id; (d) chama `capy_icon_thumbnail_ready(ctx, root, req, image_id)` e valida event `THUMBNAIL_READY` recebido com payload correto. Pipeline real de decode PNG/JPEG + cache LRU + cancellation ficam para fatia subsequente. |
| `make smoke-x64-vmware-wallpaper` (se existir) | CapyOS | Recomendado para v2.12. Se ainda não existir, criar um smoke mínimo que: (a) monta uma `capy_wallpaper_config` com 3 slides (fit_mode=FILL, interval=30s); (b) serializa via `capy_wallpaper_serialize`, escreve em `/var/state/capyui/wallpaper.bin`; (c) reaplica via `capy_wallpaper_load` e confirma round-trip byte-equivalente. Live wallpaper + transitions + decode pipeline ficam para fatia subsequente. |
| `make smoke-x64-vmware-login-layout` (se existir) | CapyOS | Recomendado para v2.13. Se ainda não existir, criar um smoke mínimo que: (a) chama `capy_login_choose_layout(0)` → SINGLE, `(1)` → SINGLE, `(3)` → GRID, `(7)` → LIST, `(UINT32_MAX)` → LIST; (b) constrói o widget tree real do login screen via `src/desktop/login.c` e valida que o layout escolhido bate com a taxonomia. Biometric/SSO + PIN keypad ficam para fatia subsequente. |
| `pkg-install org.capyos.ui.desktop-session` | VM oficial | Lab end-to-end. |

## Não-ações (deixadas explícitas)

- **Não há mudança no kernel CapyOS.** `kernel/module_gate.c` continua testando o mesmo marker.
- **Não há mudança no `Makefile` CapyOS.** Sibling detection em `../CapyUI` continua.
- **Não há novo módulo Capy package.** A lista permanece `org.capyos.ui.widget-core` + `org.capyos.ui.desktop-session`.
- **Não há novo campo de manifest.** `depends=org.capyos.ui.widget-core` permanece a única dependência declarada.
- **Não há novo error code, activation class ou kernel API.**
- **DL schema bumpou 4 → 5 em v1.5 e 5 → 6 em v1.9**. As novas ops (CAPY_DL_DPI_SCOPE, CAPY_DL_TRANSFORM_PUSH/POP) são aditivas — decoders antigos podem ignorá-las silenciosamente.
- **4 event types novos em v1.6** (DRAG_BEGIN/MOVE/END/DROP) + **4 em v1.8** (IME_*). Hosts que não emitem esses tipos não veem mudança comportamental.
- **`sizeof(capy_dl_cmd)` cresce 24 bytes em v1.9** (novo field `transform`). Hosts que stack-allocate arrays de `capy_dl_cmd` precisam recompilar. (Em v1.5 o `DPI_SCOPE` reusou o slot `image_id` e não mudou sizeof; v1.9 é o primeiro bump que cresce o struct desde o freeze.)
- **`sizeof(capy_widget) cresce em v1.6**, mas widget é sempre allocator-owned e nunca passa por ABI binária estabilizada; hosts recompilam contra a nova header sem precisar relink ABI.

## Encerramento deste arquivo

Apagar (`git rm`) este arquivo **apenas quando**:

1. As três ações documentais acima estiverem aplicadas em `CapyOS` e mergeadas em `main` lá; **e**
2. `docs/reference/integration/compatibility-audit-<YYYY-MM-DD>.md` existir e cobrir até `v2.13.0` (ou tag CapyUI ainda mais recente) **e mencionar explicitamente: schema 4→5 (v1.5) + DRAG_* (v1.6) + slab (v1.7) + IME_* (v1.8) + schema 5→6 + TRANSFORM_* + sizeof(capy_dl_cmd) +24B (v1.9) + state serialization (v1.10) + major bump 2.0 + schema 6→7 + PLUGIN_OP + plugin surface + sizeof(capy_widget_context) cresce + LTS 1.x ≥12m (v2.0) + 8 advanced widget enum slots (v2.1) + virtualization data source plumbing + VIRTUAL_REQUEST_RANGE event (v2.2) + undo/redo per-context (v2.3) + theme packs binário + FNV-1a (v2.4) + devtools/inspector read-only (v2.5) + display mode plumbing + DISPLAY_MODE_CHANGED event (v2.6) + user management plumbing + capy_user_account/directory (v2.7) + contrast/WCAG refinement + capy_contrast_finding + default_dark_high_contrast factory (v2.8) + desktop icon arrangement + CDLA blob + FNV-1a (v2.9) + file manager UX plumbing + capy_breadcrumb_truncate + toolbar taxonomy (v2.10) + icon & thumbnail system + capy_icon_provider + THUMBNAIL_READY event (v2.11) + wallpaper management + CWLP blob + slideshow + per-monitor (v2.12) + login screen plumbing + capy_login_choose_layout + power taxonomy (v2.13) + FASE 2.x COMPLETA 14/14**; **e**
3. Não houver novo bump CapyUI pendente. **Fase 2.x major CONCLUÍDA (14/14)**. Próximo bump seria o major 3.0 (Vision platform) — fora do escopo deste documento; ao landar, este arquivo deve ser **apagado** (fechamento da migração cross-repo 1.x→2.x) e um novo `cross-repo-sync-pending-v3.md` criado. Quando 1.x sair de LTS (≥12 meses pós-2.0), criar uma seção final "LTS finalizada" antes de apagar.
