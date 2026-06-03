# Contrato: Widget model

**Since:** 0.0.1
**Status:** stable (aditivo até 1.0)

## Definição

Modelo de árvore retida com tipos de widget, eventos abstratos, allocator injetável, hit-test e roteamento.

## Tipos de widget (enum `capy_widget_type`)

```c
enum capy_widget_type {
  CAPY_WIDGET_NONE = 0,
  CAPY_WIDGET_LABEL,
  CAPY_WIDGET_BUTTON,
  CAPY_WIDGET_TEXTBOX,
  CAPY_WIDGET_CHECKBOX,
  CAPY_WIDGET_LIST,
  CAPY_WIDGET_PANEL,
  CAPY_WIDGET_SCROLLBAR,
  CAPY_WIDGET_MENUBAR,
  CAPY_WIDGET_MENU_ITEM,
  CAPY_WIDGET_DIALOG,
  CAPY_WIDGET_PROGRESS,
  CAPY_WIDGET_TABS
  /* aditivo: NOTIFICATION/TRAY_ICON em 0.12; TREE/TABLE/RICH_TEXT/CANVAS/CHART/COLOR_PICKER/DATE_PICKER/AUTOCOMPLETE em 2.1 */
};
```

## Struct principal

```c
struct capy_widget {
  enum capy_widget_type type;
  struct capy_ui_rect bounds;
  struct capy_widget_style style;
  char text[CAPY_WIDGET_MAX_TEXT];
  uint8_t visible;
  uint8_t enabled;
  uint8_t checked;        /* CHECKBOX */
  uint8_t reserved;
  struct capy_widget *parent;
  struct capy_widget **children;
  uint32_t child_count;
  uint32_t child_capacity;
  /* callbacks */
  capy_widget_callback_fn on_click;
  capy_widget_callback_fn on_change;
  void *user_data;
  /* aditivo since 0.1 */
  struct capy_layout_node layout;
  /* aditivo since 0.3 */
  uint8_t focusable;
  int16_t tab_index;
  /* aditivo since 0.4 */
  struct capy_text_edit text_edit;  /* válido se type==TEXTBOX */
  /* etc., sempre no final */
};
```

## Eventos

```c
enum capy_widget_event_type {
  CAPY_WIDGET_EVENT_NONE = 0,
  CAPY_WIDGET_EVENT_POINTER_MOVE,
  CAPY_WIDGET_EVENT_POINTER_DOWN,
  CAPY_WIDGET_EVENT_POINTER_UP,
  CAPY_WIDGET_EVENT_KEY_DOWN,
  CAPY_WIDGET_EVENT_KEY_UP
  /* aditivo: WHEEL/TOUCH_*/GAMEPAD em 0.10; IME_* em 1.8; DRAG_*/DROP em 1.6 */
};

struct capy_widget_event {
  enum capy_widget_event_type type;
  struct capy_ui_point point;
  uint32_t key;
  uint32_t timestamp;
  /* aditivo since 0.10 (planned) */
  uint32_t modifiers;
  void *payload;
};
```

## Allocato

```c
typedef void *(*capy_widget_alloc_fn)(size_t size, void *user_data);
typedef void  (*capy_widget_free_fn)(void *ptr, void *user_data);

struct capy_widget_allocator {
  capy_widget_alloc_fn alloc;
  capy_widget_free_fn free;
  void *user_data;
};
```

**Invariantes:**

- `alloc(0)` retorna `NULL` ou ponteiro válido — não pode crashar.
- `free(NULL)` é no-op.
- `alloc` pode retornar `NULL` para indicar OOM; callers devem checar.

## Contexto

```c
struct capy_widget_context {
  struct capy_widget_allocator allocator;
  /* aditivo since 0.6: tema */
  struct capy_theme theme;
  /* aditivo since 0.9: hook de métricas */
  capy_text_metrics_fn text_metrics;
  void *text_metrics_user_data;
  /* aditivo since 0.13: locale */
  struct capy_locale locale;
  /* aditivo since 0.15: pool */
  struct capy_widget_pool *pool;
  /* etc. */
};
```

## APIs públicas (since 0.0.1)

- `void capy_widget_context_init(struct capy_widget_context *ctx, struct capy_widget_allocator *alloc);`
- `struct capy_widget *capy_widget_create(struct capy_widget_context *ctx, enum capy_widget_type type);`
- `void capy_widget_destroy(struct capy_widget *w);`
- `int  capy_widget_add_child(struct capy_widget *parent, struct capy_widget *child);`
- `void capy_widget_set_bounds(struct capy_widget *w, int32_t x, int32_t y, uint32_t w_, uint32_t h_);`
- `void capy_widget_set_text(struct capy_widget *w, const char *utf8);`
- `void capy_widget_set_on_click(struct capy_widget *w, capy_widget_callback_fn fn, void *user_data);`
- `struct capy_widget *capy_widget_find_at(struct capy_widget *root, struct capy_ui_point p);`
- `int  capy_widget_handle_event(struct capy_widget *root, const struct capy_widget_event *ev);`

## Invariantes

- **Sem ciclos:** `add_child` rejeita se criaria ciclo.
- **Bounds locais:** `bounds` é em coordenadas do root; layout do children deve respeitar.
- **Hit-test:** `find_at` retorna folha mais profunda visível em `(x, y)`.
- **Roteamento:** `handle_event` despacha para `find_at(point)` ou para focused widget (since 0.3).
- **`visible=0`:** widget e descendentes não recebem eventos nem aparecem em display-list.
- **`enabled=0`:** widget recebe eventos mas `on_click/on_change` não disparam.

## Regras de evolução

- Tipos novos: append no final do enum, valores estáveis.
- Campos novos em `capy_widget`: append no final da struct.
- Eventos novos: append no final do enum.
- Macros como `CAPY_WIDGET_MAX_TEXT` são parte do contrato; mudança quebra ABI.

## Testes que cobrem o contrato

- `tests/test_widget_contracts.c::test_create_find_and_click` — criação, find, click roteado.
- `tests/test_widget_contracts.c::test_disabled_widget_ignores_input` — enabled=0 bloqueia callbacks.

## Histórico de mudanças

| Versão | Mudança |
|--------|---------|
| 0.0.1 | Baseline |
| 0.1.0 | Adicionado `capy_layout_node layout` |
| 0.2.0 | Sem mudança em widget; novo módulo display-list |
| 0.3.0 | Adicionado `focusable`, `tab_index`; event ganhou `modifiers` |
| 0.4.0 | Adicionado `text_edit` (válido para TEXTBOX); APIs textbox_insert/delete/set_selection/copy + ime_compose stub |
| 0.5.0 | Animação standalone (sem mudança em widget); `capy_widget_tick` walker adicionado |
| 0.6.0 | Style ganha `bg_token`, `fg_token`, `border_token`, `reserved`; context ganha `theme` embutido (struct capy_theme) |
| 0.10 (planned) | Event ganha `modifiers`, `payload` |
| 0.15 (planned) | Context ganha `pool`; widget ganha `layout_dirty`, `layout_version` |
