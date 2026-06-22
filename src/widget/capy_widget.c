#include "capy_widget.h"

static void capy_widget_zero(void *ptr, size_t len) {
  uint8_t *p = (uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

static void capy_widget_copy_text(char *dst, size_t dst_size, const char *src) {
  size_t i = 0u;
  if (!dst || dst_size == 0u) {
    return;
  }
  if (!src) {
    dst[0] = '\0';
    return;
  }
  while (i + 1u < dst_size && src[i]) {
    dst[i] = src[i];
    ++i;
  }
  dst[i] = '\0';
}

static int capy_widget_point_in_rect(int32_t x, int32_t y,
                                     const struct capy_ui_rect *rect) {
  if (!rect) {
    return 0;
  }
  return x >= rect->x && x < rect->x + (int32_t)rect->width &&
         y >= rect->y && y < rect->y + (int32_t)rect->height;
}

uint32_t capy_widget_abi_version(void) { return CAPYUI_API_VERSION_TAG; }

const char *capy_widget_type_name(enum capy_widget_type type) {
  switch (type) {
  case CAPY_WIDGET_NONE: return "NONE";
  case CAPY_WIDGET_LABEL: return "LABEL";
  case CAPY_WIDGET_BUTTON: return "BUTTON";
  case CAPY_WIDGET_TEXTBOX: return "TEXTBOX";
  case CAPY_WIDGET_CHECKBOX: return "CHECKBOX";
  case CAPY_WIDGET_LIST: return "LIST";
  case CAPY_WIDGET_PANEL: return "PANEL";
  case CAPY_WIDGET_SCROLLBAR: return "SCROLLBAR";
  case CAPY_WIDGET_MENUBAR: return "MENUBAR";
  case CAPY_WIDGET_MENU_ITEM: return "MENU_ITEM";
  case CAPY_WIDGET_DIALOG: return "DIALOG";
  case CAPY_WIDGET_PROGRESS: return "PROGRESS";
  case CAPY_WIDGET_TABS: return "TABS";
  case CAPY_WIDGET_TREE: return "TREE";
  case CAPY_WIDGET_TABLE: return "TABLE";
  case CAPY_WIDGET_RICH_TEXT: return "RICH_TEXT";
  case CAPY_WIDGET_CANVAS: return "CANVAS";
  case CAPY_WIDGET_CHART: return "CHART";
  case CAPY_WIDGET_COLOR_PICKER: return "COLOR_PICKER";
  case CAPY_WIDGET_DATE_PICKER: return "DATE_PICKER";
  case CAPY_WIDGET_AUTOCOMPLETE: return "AUTOCOMPLETE";
  }
  return "UNKNOWN";
}

struct capy_widget_style capy_widget_default_style(void) {
  struct capy_widget_style style;
  style.bg_color = 0xFF202428u;
  style.fg_color = 0xFFE8ECEFu;
  style.border_color = 0xFF4B5563u;
  style.hover_color = 0xFF374151u;
  style.active_color = 0xFF2563EBu;
  style.text_color = 0xFFE8ECEFu;
  style.border_width = 1u;
  style.padding = 4u;
  style.margin = 2u;
  style.font_size = 16u;
  return style;
}

struct capy_widget_style capy_widget_button_style(void) {
  struct capy_widget_style style = capy_widget_default_style();
  style.bg_color = 0xFF2563EBu;
  style.hover_color = 0xFF3B82F6u;
  style.active_color = 0xFF1D4ED8u;
  style.text_color = 0xFFFFFFFFu;
  style.padding = 8u;
  return style;
}

struct capy_widget_style capy_widget_textbox_style(void) {
  struct capy_widget_style style = capy_widget_default_style();
  style.bg_color = 0xFF111827u;
  style.border_color = 0xFF6B7280u;
  return style;
}

void capy_widget_context_init(struct capy_widget_context *ctx,
                              const struct capy_widget_allocator *allocator) {
  if (!ctx) {
    return;
  }
  capy_widget_zero(ctx, sizeof(*ctx));
  ctx->next_id = 1u;
  if (allocator) {
    ctx->allocator = *allocator;
  }
  /* Seed default locale so pre-0.13 callers observe sensible values. */
  ctx->locale = capy_locale_default();
  /* Seed DPI scale at 1.0× so pre-1.5 callers observe unchanged dimensions. */
  ctx->dpi_scale_x256 = 256u;
  /* Plugin registry starts empty (since 2.0). `capy_widget_zero` already
   * clears the pointer array and count, but writing them explicitly keeps
   * the init audit-friendly. */
  ctx->plugin_count = 0u;
}

/* ── v2.0: plugin ABI surface (register / unregister_all) ──────────────── */

int capy_plugin_register(struct capy_widget_context *ctx,
                         const struct capy_plugin_descriptor *desc) {
  uint32_t i;
  if (!ctx || !desc) {
    return -1;
  }
  /* Validate that the descriptor's required ABI version is not newer than
   * the running host. Older requirements are accepted: callers compiled
   * against a stricter pre-2.0 plugin shape would not even compile against
   * this header, so the only valid `capy_ui_abi_required` values are
   * <= `CAPYUI_API_VERSION_TAG`. */
  if (desc->capy_ui_abi_required > CAPYUI_API_VERSION_TAG) {
    return -1;
  }
  /* Reject duplicate registration of the same descriptor pointer. We do
   * not compare `id` strings — hosts that want stricter de-duplication
   * apply it themselves. */
  for (i = 0u; i < ctx->plugin_count; ++i) {
    if (ctx->plugins[i] == desc) {
      return -1;
    }
  }
  if (ctx->plugin_count >= CAPY_MAX_PLUGINS) {
    return -1;
  }
  ctx->plugins[ctx->plugin_count] = desc;
  return (int)ctx->plugin_count++;
}

void capy_plugin_unregister_all(struct capy_widget_context *ctx) {
  uint32_t i;
  if (!ctx) {
    return;
  }
  /* Invoke each `destroy` in registration order. The widget core does
   * not allocate or own per-plugin state, so there is nothing else to
   * release. `capy_plugin_context` is caller-managed; the host typically
   * stashes it inside the plugin's allocator. */
  for (i = 0u; i < ctx->plugin_count; ++i) {
    const struct capy_plugin_descriptor *d = ctx->plugins[i];
    if (d && d->destroy) {
      struct capy_plugin_context pc;
      pc.descriptor = d;
      pc.user_data = 0;
      d->destroy(&pc);
    }
    ctx->plugins[i] = 0;
  }
  ctx->plugin_count = 0u;
}

/* ── v2.2: virtualization (data source plumbing) ───────────────────────── */

void capy_widget_set_virtual_source(
    struct capy_widget *w, const struct capy_virtual_data_source *src) {
  if (!w) {
    return;
  }
  w->virtual_source = src;
}

int capy_widget_virtual_count(const struct capy_widget *w) {
  if (!w || !w->virtual_source || !w->virtual_source->get_count) {
    return -1;
  }
  return w->virtual_source->get_count(w->virtual_source->user_data);
}

int capy_widget_virtual_get_item(const struct capy_widget *w, uint32_t index,
                                 char *out_text, uint16_t cap) {
  if (!w || !w->virtual_source || !w->virtual_source->get_item || !out_text ||
      cap == 0u) {
    return -1;
  }
  return w->virtual_source->get_item(w->virtual_source->user_data, index,
                                     out_text, cap);
}

/* ── v2.3: undo/redo stack ─────────────────────────────────────────────── */

int capy_undo_stack_init(struct capy_widget_context *ctx, void *buf,
                         uint32_t buf_size) {
  struct capy_undo_stack *stack;
  uint8_t *bytes;
  uint32_t entry_bytes;
  if (!ctx || !buf) {
    return -1;
  }
  if (buf_size < (uint32_t)(sizeof(struct capy_undo_stack) +
                            sizeof(struct capy_undo_entry))) {
    return -1;
  }
  stack = (struct capy_undo_stack *)buf;
  bytes = (uint8_t *)buf;
  stack->entries =
      (struct capy_undo_entry *)(bytes + sizeof(struct capy_undo_stack));
  entry_bytes = buf_size - (uint32_t)sizeof(struct capy_undo_stack);
  stack->capacity = entry_bytes / (uint32_t)sizeof(struct capy_undo_entry);
  stack->count = 0u;
  stack->redo_count = 0u;
  /* Default coalesce window matches the slice plan even though the core
   * does not enforce it yet \u2014 hosts that want coalescing can read this
   * field. */
  stack->coalesce_window_ms = 500u;
  ctx->undo_stack = stack;
  return 0;
}

int capy_undo_push(struct capy_widget_context *ctx, const char *action_id,
                   const void *redo_data, uint32_t redo_len,
                   const void *undo_data, uint32_t undo_len) {
  struct capy_undo_stack *stack;
  struct capy_undo_entry *e;
  if (!ctx || !ctx->undo_stack) {
    return -1;
  }
  stack = ctx->undo_stack;
  /* Pushing a new entry discards any pending redo history \u2014 the user has
   * branched off the previous undo lineage. */
  stack->redo_count = 0u;
  if (stack->count == stack->capacity) {
    /* FIFO evict the oldest entry. */
    uint32_t i;
    for (i = 1u; i < stack->count; ++i) {
      stack->entries[i - 1u] = stack->entries[i];
    }
    --stack->count;
  }
  e = &stack->entries[stack->count];
  e->action_id = action_id;
  e->redo_data = redo_data;
  e->redo_len = redo_len;
  e->undo_data = undo_data;
  e->undo_len = undo_len;
  e->timestamp_ms = 0u;
  ++stack->count;
  return 0;
}

int capy_undo_undo(struct capy_widget_context *ctx,
                   struct capy_undo_entry *out) {
  struct capy_undo_stack *stack;
  if (!ctx || !ctx->undo_stack || ctx->undo_stack->count == 0u) {
    return -1;
  }
  stack = ctx->undo_stack;
  if (out) {
    *out = stack->entries[stack->count - 1u];
  }
  --stack->count;
  ++stack->redo_count;
  return 0;
}

int capy_undo_redo(struct capy_widget_context *ctx,
                   struct capy_undo_entry *out) {
  struct capy_undo_stack *stack;
  if (!ctx || !ctx->undo_stack || ctx->undo_stack->redo_count == 0u) {
    return -1;
  }
  stack = ctx->undo_stack;
  /* The next redoable entry lives immediately past the live top: it was
   * moved there by a previous undo. */
  if (out) {
    *out = stack->entries[stack->count];
  }
  ++stack->count;
  --stack->redo_count;
  return 0;
}

int capy_undo_can_undo(const struct capy_widget_context *ctx) {
  if (!ctx || !ctx->undo_stack) {
    return 0;
  }
  return ctx->undo_stack->count > 0u ? 1 : 0;
}

int capy_undo_can_redo(const struct capy_widget_context *ctx) {
  if (!ctx || !ctx->undo_stack) {
    return 0;
  }
  return ctx->undo_stack->redo_count > 0u ? 1 : 0;
}

/* ── v1.5: multi-display & DPI scaling ─────────────────────────────────── */

void capy_widget_set_dpi_scale(struct capy_widget_context *ctx,
                               uint16_t scale_x256) {
  if (!ctx) {
    return;
  }
  ctx->dpi_scale_x256 = (scale_x256 == 0u) ? 1u : scale_x256;
}

uint16_t capy_widget_get_dpi_scale(const struct capy_widget_context *ctx) {
  if (!ctx) {
    return 256u;
  }
  /* Pre-1.5 contexts (zero-initialised by callers before the tail field
   * existed) report `0` here. Treat that as the 1.0× default so legacy
   * call sites that bypass `capy_widget_context_init` still get sensible
   * behaviour from this getter. */
  return (ctx->dpi_scale_x256 == 0u) ? 256u : ctx->dpi_scale_x256;
}

int32_t capy_widget_dpi_scale_dim(uint16_t scale_x256, int32_t value) {
  int64_t scaled;
  if (scale_x256 == 256u) {
    return value;
  }
  if (scale_x256 == 0u) {
    /* Degenerate scale clamped to 1/256 so the helper never divides
     * by zero or returns a meaningless 0 for positive values. */
    scale_x256 = 1u;
  }
  scaled = ((int64_t)value * (int64_t)scale_x256) / 256;
  if (scaled > (int64_t)INT32_MAX) {
    return INT32_MAX;
  }
  if (scaled < (int64_t)INT32_MIN) {
    return INT32_MIN;
  }
  return (int32_t)scaled;
}

struct capy_widget *capy_widget_create(struct capy_widget_context *ctx,
                                       enum capy_widget_type type) {
  struct capy_widget *widget;
  if (!ctx || !ctx->allocator.alloc) {
    return 0;
  }
  widget = (struct capy_widget *)ctx->allocator.alloc(sizeof(*widget),
                                                       ctx->allocator.user_data);
  if (!widget) {
    return 0;
  }
  capy_widget_zero(widget, sizeof(*widget));
  widget->id = ctx->next_id++;
  widget->type = type;
  widget->visible = 1u;
  widget->enabled = 1u;
  widget->allocator = ctx->allocator;
  /* Since 0.15: every fresh widget must compute its layout once. */
  widget->layout_dirty = 1u;
  switch (type) {
    case CAPY_WIDGET_BUTTON:
    case CAPY_WIDGET_TEXTBOX:
    case CAPY_WIDGET_CHECKBOX:
    case CAPY_WIDGET_LIST:
    case CAPY_WIDGET_MENU_ITEM:
    case CAPY_WIDGET_TABS:
      widget->focusable = 1u;
      break;
    default:
      widget->focusable = 0u;
      break;
  }
  if (type == CAPY_WIDGET_BUTTON) {
    widget->style = capy_widget_button_style();
  } else if (type == CAPY_WIDGET_TEXTBOX) {
    widget->style = capy_widget_textbox_style();
  } else {
    widget->style = capy_widget_default_style();
  }
  return widget;
}

void capy_widget_destroy(struct capy_widget *widget) {
  struct capy_widget_allocator allocator;
  if (!widget) {
    return;
  }
  allocator = widget->allocator;
  for (uint32_t i = 0; i < widget->child_count; ++i) {
    capy_widget_destroy(widget->children[i]);
  }
  if (allocator.free) {
    allocator.free(widget, allocator.user_data);
  }
}

void capy_widget_set_bounds(struct capy_widget *widget, int32_t x, int32_t y,
                            uint32_t width, uint32_t height) {
  if (!widget) {
    return;
  }
  widget->bounds.x = x;
  widget->bounds.y = y;
  widget->bounds.width = width;
  widget->bounds.height = height;
}

void capy_widget_set_text(struct capy_widget *widget, const char *text) {
  if (!widget) {
    return;
  }
  capy_widget_copy_text(widget->text, sizeof(widget->text), text);
}

void capy_widget_set_visible(struct capy_widget *widget, int visible) {
  if (widget) {
    widget->visible = visible ? 1u : 0u;
  }
}

void capy_widget_set_enabled(struct capy_widget *widget, int enabled) {
  if (widget) {
    widget->enabled = enabled ? 1u : 0u;
  }
}

void capy_widget_set_style(struct capy_widget *widget,
                           const struct capy_widget_style *style) {
  if (widget && style) {
    widget->style = *style;
  }
}

int capy_widget_add_child(struct capy_widget *parent,
                          struct capy_widget *child) {
  if (!parent || !child || parent->child_count >= CAPY_WIDGET_MAX_CHILDREN) {
    return -1;
  }
  child->parent = parent;
  parent->children[parent->child_count++] = child;
  return 0;
}

int capy_widget_remove_child(struct capy_widget *parent,
                             struct capy_widget *child) {
  if (!parent || !child) {
    return -1;
  }
  for (uint32_t i = 0; i < parent->child_count; ++i) {
    if (parent->children[i] == child) {
      child->parent = 0;
      for (uint32_t j = i; j + 1u < parent->child_count; ++j) {
        parent->children[j] = parent->children[j + 1u];
      }
      --parent->child_count;
      return 0;
    }
  }
  return -1;
}

/* ── v0.10: input plumbing helpers ────────────────────────────────────── */

static struct capy_widget *capy_widget_find_list_ancestor(
    struct capy_widget *w) {
  while (w && w->type != CAPY_WIDGET_LIST) {
    w = w->parent;
  }
  return w;
}

static int32_t capy_clamp_i32(int32_t v, int32_t lo, int32_t hi) {
  if (lo == hi) {
    return v;
  }
  if (v < lo) {
    v = lo;
  }
  if (v > hi) {
    v = hi;
  }
  return v;
}

static int capy_widget_handle_wheel(struct capy_widget *root,
                                    const struct capy_widget_event *event) {
  const struct capy_wheel_payload *payload;
  struct capy_widget *under;
  struct capy_widget *list;
  int32_t new_value;
  if (!event->payload) {
    return 0;
  }
  payload = (const struct capy_wheel_payload *)event->payload;
  under = capy_widget_find_at(root, event->x, event->y);
  list = capy_widget_find_list_ancestor(under);
  if (!list || !list->enabled || !list->visible) {
    return 0;
  }
  new_value = list->value + (int32_t)payload->delta_y;
  new_value = capy_clamp_i32(new_value, list->min_value, list->max_value);
  if (new_value != list->value) {
    list->value = new_value;
    if (list->on_change) {
      list->on_change(list, list->user_data);
    }
  }
  return 1;
}

static int capy_widget_handle_touch(struct capy_widget *root,
                                    const struct capy_widget_event *event) {
  struct capy_widget_event pe = *event;
  switch (event->type) {
    case CAPY_WIDGET_EVENT_TOUCH_BEGIN:
      pe.type = CAPY_WIDGET_EVENT_POINTER_DOWN;
      pe.buttons = 1u;
      break;
    case CAPY_WIDGET_EVENT_TOUCH_MOVE:
      pe.type = CAPY_WIDGET_EVENT_POINTER_MOVE;
      break;
    case CAPY_WIDGET_EVENT_TOUCH_END:
      pe.type = CAPY_WIDGET_EVENT_POINTER_UP;
      pe.buttons = 0u;
      break;
    default:
      return 0;
  }
  if (event->payload) {
    const struct capy_touch_payload *t =
        (const struct capy_touch_payload *)event->payload;
    pe.x = t->pos.x;
    pe.y = t->pos.y;
  }
  pe.payload = 0;
  return capy_widget_handle_event(root, &pe);
}

/* ── v1.6: drag and drop ──────────────────────────────────────────────── */

void capy_widget_set_draggable(struct capy_widget *w,
                               const struct capy_dnd_payload *payload) {
  if (w) {
    w->drag_payload = payload;
  }
}

void capy_widget_set_drop_target(struct capy_widget *w,
                                 const char *const *accepted_types,
                                 uint32_t count) {
  if (!w) {
    return;
  }
  w->drop_accepted_types = accepted_types;
  w->drop_types_count = (accepted_types == 0) ? 0u : count;
}

void capy_widget_set_on_drop(
    struct capy_widget *w,
    int (*fn)(struct capy_widget *target,
              const struct capy_dnd_payload *payload, void *user_data),
    void *user_data) {
  if (!w) {
    return;
  }
  w->on_drop = fn;
  w->drop_user_data = user_data;
}

static int capy_dnd_ascii_eq_nocase(char a, char b) {
  if (a >= 'A' && a <= 'Z') {
    a = (char)(a + 32);
  }
  if (b >= 'A' && b <= 'Z') {
    b = (char)(b + 32);
  }
  return a == b;
}

int capy_dnd_type_matches(const char *accepted, const char *type) {
  uint32_t i;
  if (!accepted || !type) {
    return 0;
  }
  /* Pure wildcard accepts everything (matches the "*" sole pattern). */
  if (accepted[0] == '*' && accepted[1] == '\0') {
    return 1;
  }
  for (i = 0u; accepted[i] != '\0' && type[i] != '\0'; ++i) {
    /* A '*' inside an `accepted` pattern is treated as "match remainder"
     * (so a text-prefix wildcard accepts "text/plain"). We do not support intermediate
     * stars or character classes — keep the pattern language tiny. */
    if (accepted[i] == '*') {
      return 1;
    }
    if (!capy_dnd_ascii_eq_nocase(accepted[i], type[i])) {
      return 0;
    }
  }
  /* Trailing star at end of accepted ("foo*") with type ending exactly:
   * walk past the star if present. */
  if (accepted[i] == '*' && accepted[i + 1u] == '\0') {
    return 1;
  }
  return accepted[i] == '\0' && type[i] == '\0';
}

int capy_widget_dnd_accepts(const struct capy_widget *w,
                            const struct capy_dnd_payload *payload) {
  uint32_t i;
  if (!w || !payload || !w->drop_accepted_types ||
      w->drop_types_count == 0u) {
    return 0;
  }
  for (i = 0u; i < w->drop_types_count; ++i) {
    const char *pattern = w->drop_accepted_types[i];
    if (pattern && capy_dnd_type_matches(pattern, payload->type)) {
      return 1;
    }
  }
  return 0;
}

/* ── v1.7: slab allocator ─────────────────────────────────────────────── */

void capy_slab_init(struct capy_slab *s, void *buf, uint32_t size,
                    uint32_t element_size) {
  if (!s) {
    return;
  }
  s->buffer = buf;
  s->size = size;
  s->element_size = element_size;
  s->high_water = 0u;
  s->free_list = 0;
  /* Degenerate states (`element_size < sizeof(void*)`, NULL buffer or
   * zero size) are tolerated: subsequent `_alloc` calls just return NULL
   * because the bump cannot advance and the free-list stays empty. The
   * fields above are still set so callers can introspect the slab. */
  if (!buf || size == 0u || element_size < sizeof(void *)) {
    s->size = 0u;
  }
}

void *capy_slab_alloc(struct capy_slab *s) {
  uint32_t capacity;
  void *slot;
  if (!s || !s->buffer || s->element_size == 0u) {
    return 0;
  }
  /* Pop LIFO from the free-list first so freed slots are reused promptly. */
  if (s->free_list) {
    void *head = s->free_list;
    /* The freed slot holds a `void *` to the next free slot (or NULL).
     * `memcpy`-free read because all slots are at least `sizeof(void *)`
     * bytes and aligned to that boundary by construction. */
    s->free_list = *(void **)head;
    return head;
  }
  capacity = s->size / s->element_size;
  if (s->high_water >= capacity) {
    return 0;
  }
  slot = (unsigned char *)s->buffer + (size_t)s->high_water * s->element_size;
  ++s->high_water;
  return slot;
}

void capy_slab_free(struct capy_slab *s, void *ptr) {
  if (!s || !ptr || s->element_size < sizeof(void *)) {
    return;
  }
  /* Push onto the LIFO free-list. The caller is responsible for passing
   * a pointer previously returned by `capy_slab_alloc` from this slab;
   * CapyUI does not validate the range to keep `free` O(1). */
  *(void **)ptr = s->free_list;
  s->free_list = ptr;
}

/* ── v1.8: rich IME (composition state on TEXTBOX widgets) ─────────────── */

void capy_ime_set_preedit(struct capy_widget *w, const char *text,
                          uint16_t len, uint16_t cursor) {
  if (!w || w->type != CAPY_WIDGET_TEXTBOX) {
    return;
  }
  w->ime_preedit = text;
  w->ime_preedit_len = (text == 0) ? 0u : len;
  w->ime_cursor_pos = (text == 0) ? 0u : cursor;
  w->ime_composition_phase = 1u; /* composing */
}

void capy_ime_set_candidates(struct capy_widget *w, const char *const *list,
                             uint16_t count, uint16_t selected) {
  if (!w || w->type != CAPY_WIDGET_TEXTBOX) {
    return;
  }
  w->ime_candidates = list;
  w->ime_candidate_count = (list == 0) ? 0u : count;
  w->ime_selected_index = (list == 0 || count == 0u) ? 0u : selected;
  if (list != 0 && count > 0u) {
    w->ime_composition_phase = 2u; /* selecting */
  }
}

int capy_ime_commit(struct capy_widget *w, const char *final_text,
                    uint16_t len) {
  int rc;
  if (!w || w->type != CAPY_WIDGET_TEXTBOX) {
    return -1;
  }
  rc = (final_text == 0 || len == 0u)
           ? 0
           : capy_textbox_insert(w, final_text, len);
  /* Always clear the composition state, even when the insert was rejected
   * (e.g. textbox at capacity) so a subsequent edit starts from a clean
   * slate. The host engine is expected to surface the rejection via its
   * own status channel. */
  w->ime_preedit = 0;
  w->ime_preedit_len = 0u;
  w->ime_cursor_pos = 0u;
  w->ime_candidates = 0;
  w->ime_candidate_count = 0u;
  w->ime_selected_index = 0u;
  w->ime_composition_phase = 0u;
  return rc;
}

void capy_ime_cancel(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_TEXTBOX) {
    return;
  }
  w->ime_preedit = 0;
  w->ime_preedit_len = 0u;
  w->ime_cursor_pos = 0u;
  w->ime_candidates = 0;
  w->ime_candidate_count = 0u;
  w->ime_selected_index = 0u;
  w->ime_composition_phase = 0u;
}

/* ── v1.9: 2D affine transforms (factories + multiply + setter) ──────── */

struct capy_dl_transform capy_transform_identity(void) {
  struct capy_dl_transform t;
  t.m00 = 0x10000;
  t.m01 = 0;
  t.m02 = 0;
  t.m10 = 0;
  t.m11 = 0x10000;
  t.m12 = 0;
  return t;
}

struct capy_dl_transform capy_transform_scale(int32_t sx_q16, int32_t sy_q16) {
  struct capy_dl_transform t;
  t.m00 = sx_q16;
  t.m01 = 0;
  t.m02 = 0;
  t.m10 = 0;
  t.m11 = sy_q16;
  t.m12 = 0;
  return t;
}

struct capy_dl_transform capy_transform_translate(int32_t tx, int32_t ty) {
  struct capy_dl_transform t;
  t.m00 = 0x10000;
  t.m01 = 0;
  t.m02 = tx;
  t.m10 = 0;
  t.m11 = 0x10000;
  t.m12 = ty;
  return t;
}

struct capy_dl_transform capy_transform_rotate_quadrants(uint32_t n) {
  /* Exact 90° rotations (no LUT, no float). Reduce modulo 4 so callers
   * can pass any unsigned count. */
  struct capy_dl_transform t;
  t.m02 = 0;
  t.m12 = 0;
  switch (n & 3u) {
    case 0u: /* 0° = identity. */
      t.m00 = 0x10000;
      t.m01 = 0;
      t.m10 = 0;
      t.m11 = 0x10000;
      break;
    case 1u: /* 90° CCW: (x, y) → (-y, x). */
      t.m00 = 0;
      t.m01 = -0x10000;
      t.m10 = 0x10000;
      t.m11 = 0;
      break;
    case 2u: /* 180°: (x, y) → (-x, -y). */
      t.m00 = -0x10000;
      t.m01 = 0;
      t.m10 = 0;
      t.m11 = -0x10000;
      break;
    default: /* 270° = -90°: (x, y) → (y, -x). */
      t.m00 = 0;
      t.m01 = 0x10000;
      t.m10 = -0x10000;
      t.m11 = 0;
      break;
  }
  return t;
}

static int32_t capy_transform_mul_q16(int32_t a, int32_t b) {
  /* Q16.16 * Q16.16 = Q32.32, shift right by 16 → Q16.16. Strict-C11
   * division on int64_t (truncate toward zero) for negative values. */
  int64_t product = (int64_t)a * (int64_t)b;
  int64_t shifted = product / 0x10000;
  if (shifted > (int64_t)INT32_MAX) {
    return INT32_MAX;
  }
  if (shifted < (int64_t)INT32_MIN) {
    return INT32_MIN;
  }
  return (int32_t)shifted;
}

static int32_t capy_transform_add_clamped(int32_t a, int32_t b) {
  int64_t s = (int64_t)a + (int64_t)b;
  if (s > (int64_t)INT32_MAX) {
    return INT32_MAX;
  }
  if (s < (int64_t)INT32_MIN) {
    return INT32_MIN;
  }
  return (int32_t)s;
}

struct capy_dl_transform capy_transform_multiply(
    const struct capy_dl_transform *a, const struct capy_dl_transform *b) {
  struct capy_dl_transform r;
  if (!a || !b) {
    return capy_transform_identity();
  }
  /* [a00 a01 a02]   [b00 b01 b02]
   * [a10 a11 a12] × [b10 b11 b12]
   * [ 0   0   1 ]   [ 0   0   1 ]
   *
   * a00/a01/a10/a11 are Q16.16; a02/a12 are integer pixels. When
   * multiplying the linear part of `a` against `b02`/`b12` we need to
   * scale: m02 = a00*b02_q16 + a01*b12_q16 + a02 (treating `b02`/`b12`
   * as integer pixels lifted to Q16.16 by shifting left 16 and shifting
   * the result right 16). Implemented via mul_q16 helper. */
  r.m00 = capy_transform_add_clamped(capy_transform_mul_q16(a->m00, b->m00),
                                     capy_transform_mul_q16(a->m01, b->m10));
  r.m01 = capy_transform_add_clamped(capy_transform_mul_q16(a->m00, b->m01),
                                     capy_transform_mul_q16(a->m01, b->m11));
  /* m02 = (a00*b02 + a01*b12) / Q16 + a02. b02/b12 are integer pixels,
   * a00/a01 are Q16.16 → product is Q16.16 pixel-units; divide by Q16
   * to get integer pixels. */
  {
    int64_t lin = ((int64_t)a->m00 * (int64_t)b->m02 +
                   (int64_t)a->m01 * (int64_t)b->m12) /
                  0x10000;
    int64_t s = lin + (int64_t)a->m02;
    if (s > (int64_t)INT32_MAX) s = INT32_MAX;
    if (s < (int64_t)INT32_MIN) s = INT32_MIN;
    r.m02 = (int32_t)s;
  }
  r.m10 = capy_transform_add_clamped(capy_transform_mul_q16(a->m10, b->m00),
                                     capy_transform_mul_q16(a->m11, b->m10));
  r.m11 = capy_transform_add_clamped(capy_transform_mul_q16(a->m10, b->m01),
                                     capy_transform_mul_q16(a->m11, b->m11));
  {
    int64_t lin = ((int64_t)a->m10 * (int64_t)b->m02 +
                   (int64_t)a->m11 * (int64_t)b->m12) /
                  0x10000;
    int64_t s = lin + (int64_t)a->m12;
    if (s > (int64_t)INT32_MAX) s = INT32_MAX;
    if (s < (int64_t)INT32_MIN) s = INT32_MIN;
    r.m12 = (int32_t)s;
  }
  return r;
}

void capy_widget_set_transform(struct capy_widget *w,
                               const struct capy_dl_transform *t) {
  if (!w) {
    return;
  }
  w->transform = t;
}

void capy_ime_get_state(const struct capy_widget *w,
                        struct capy_ime_state *out) {
  if (!out) {
    return;
  }
  out->preedit = 0;
  out->preedit_len = 0u;
  out->cursor_pos = 0u;
  out->candidates = 0;
  out->candidate_count = 0u;
  out->selected_index = 0u;
  out->composition_phase = 0u;
  out->reserved[0] = 0u;
  out->reserved[1] = 0u;
  out->reserved[2] = 0u;
  if (!w || w->type != CAPY_WIDGET_TEXTBOX) {
    return;
  }
  out->preedit = w->ime_preedit;
  out->preedit_len = w->ime_preedit_len;
  out->cursor_pos = w->ime_cursor_pos;
  out->candidates = w->ime_candidates;
  out->candidate_count = w->ime_candidate_count;
  out->selected_index = w->ime_selected_index;
  out->composition_phase = w->ime_composition_phase;
}

int capy_widget_handle_event(struct capy_widget *widget,
                             const struct capy_widget_event *event) {
  if (!widget || !event || !widget->visible || !widget->enabled) {
    return 0;
  }
  /* v1.6: DROP routed deepest-first to the first widget that accepts the
   * payload AND has a registered on_drop. DRAG_BEGIN/MOVE/END are no-ops
   * here — the host dispatcher owns the visual preview and the begin/end
   * lifecycle; widget core only acts on the terminal DROP. */
  if (event->type == CAPY_WIDGET_EVENT_DROP) {
    const struct capy_dnd_payload *payload =
        (const struct capy_dnd_payload *)event->payload;
    uint32_t i;
    for (i = 0u; i < widget->child_count; ++i) {
      if (capy_widget_handle_event(widget->children[i], event)) {
        return 1;
      }
    }
    if (payload && widget->on_drop &&
        capy_widget_point_in_rect(event->x, event->y, &widget->bounds) &&
        capy_widget_dnd_accepts(widget, payload)) {
      return widget->on_drop(widget, payload, widget->drop_user_data);
    }
    return 0;
  }
  if (event->type == CAPY_WIDGET_EVENT_DRAG_BEGIN ||
      event->type == CAPY_WIDGET_EVENT_DRAG_MOVE ||
      event->type == CAPY_WIDGET_EVENT_DRAG_END) {
    return 0;
  }
  /* v1.8: IME phase-transition events are advisory; the host engine drives
   * the actual state through the `capy_ime_*` APIs. Core just acknowledges
   * the events without mutating anything. */
  if (event->type == CAPY_WIDGET_EVENT_IME_COMPOSE_START ||
      event->type == CAPY_WIDGET_EVENT_IME_PREEDIT_UPDATE ||
      event->type == CAPY_WIDGET_EVENT_IME_COMMIT ||
      event->type == CAPY_WIDGET_EVENT_IME_CANCEL) {
    return 0;
  }
  /* v2.2: virtualization prefetch hint. The host renderer emits this when
   * a virtualized LIST/TREE/TABLE is about to scroll into the
   * `[event->x, event->y)` index range; the host data source listens.
   * The widget core has nothing to do here. */
  if (event->type == CAPY_WIDGET_EVENT_VIRTUAL_REQUEST_RANGE) {
    return 0;
  }
  /* v0.10: WHEEL routed to nearest LIST ancestor under the cursor. */
  if (event->type == CAPY_WIDGET_EVENT_WHEEL) {
    return capy_widget_handle_wheel(widget, event);
  }
  /* v0.10: single-touch translates to POINTER_*; multi-touch gestures land
   * in v1.4. CapyUI only synthesizes the equivalent pointer event here. */
  if (event->type == CAPY_WIDGET_EVENT_TOUCH_BEGIN ||
      event->type == CAPY_WIDGET_EVENT_TOUCH_MOVE ||
      event->type == CAPY_WIDGET_EVENT_TOUCH_END) {
    return capy_widget_handle_touch(widget, event);
  }
  /* v0.10: GAMEPAD reserved for future configurable mapping. Accept the
   * event silently so hosts can deliver it without crashing the widget
   * tree even before bindings exist. */
  if (event->type == CAPY_WIDGET_EVENT_GAMEPAD) {
    return 0;
  }
  if (event->type == CAPY_WIDGET_EVENT_POINTER_MOVE) {
    widget->hovered = capy_widget_point_in_rect(event->x, event->y,
                                                &widget->bounds)
                          ? 1u
                          : 0u;
    for (uint32_t i = 0; i < widget->child_count; ++i) {
      capy_widget_handle_event(widget->children[i], event);
    }
    return 0;
  }
  for (uint32_t i = 0; i < widget->child_count; ++i) {
    if (capy_widget_handle_event(widget->children[i], event)) {
      return 1;
    }
  }
  if (event->type == CAPY_WIDGET_EVENT_POINTER_DOWN &&
      (event->buttons & 1u) != 0u &&
      capy_widget_point_in_rect(event->x, event->y, &widget->bounds)) {
    if ((widget->type == CAPY_WIDGET_BUTTON ||
         widget->type == CAPY_WIDGET_MENUBAR ||
         widget->type == CAPY_WIDGET_MENU_ITEM) &&
        widget->on_click) {
      widget->on_click(widget, widget->user_data);
    }
    if (widget->type == CAPY_WIDGET_CHECKBOX) {
      widget->checked = widget->checked ? 0u : 1u;
      if (widget->on_change) {
        widget->on_change(widget, widget->user_data);
      }
    }
    return 1;
  }
  return 0;
}

struct capy_widget *capy_widget_find_at(struct capy_widget *root, int32_t x,
                                        int32_t y) {
  if (!root || !root->visible) {
    return 0;
  }
  for (int32_t i = (int32_t)root->child_count - 1; i >= 0; --i) {
    struct capy_widget *found = capy_widget_find_at(root->children[i], x, y);
    if (found) {
      return found;
    }
  }
  return capy_widget_point_in_rect(x, y, &root->bounds) ? root : 0;
}

struct capy_widget *capy_widget_find_by_id(struct capy_widget *root,
                                           uint32_t id) {
  if (!root) {
    return 0;
  }
  if (root->id == id) {
    return root;
  }
  for (uint32_t i = 0u; i < root->child_count; ++i) {
    struct capy_widget *found = capy_widget_find_by_id(root->children[i], id);
    if (found) {
      return found;
    }
  }
  return 0;
}

void capy_widget_focus(struct capy_widget *widget) {
  if (widget) {
    widget->focused = 1u;
  }
}

void capy_widget_set_on_click(struct capy_widget *widget,
                              capy_widget_callback callback, void *user_data) {
  if (widget) {
    widget->on_click = callback;
    widget->user_data = user_data;
  }
}

void capy_widget_set_on_change(struct capy_widget *widget,
                               capy_widget_callback callback, void *user_data) {
  if (widget) {
    widget->on_change = callback;
    widget->user_data = user_data;
  }
}

void capy_widget_set_focusable(struct capy_widget *widget, int focusable,
                               int16_t tab_index) {
  if (!widget) {
    return;
  }
  widget->focusable = focusable ? 1u : 0u;
  widget->tab_index = tab_index;
}

void capy_widget_clear_focus(struct capy_widget *root) {
  uint32_t i;
  if (!root) {
    return;
  }
  root->focused = 0u;
  for (i = 0u; i < root->child_count; ++i) {
    capy_widget_clear_focus(root->children[i]);
  }
}

struct capy_widget *capy_widget_get_focused(struct capy_widget *root) {
  uint32_t i;
  struct capy_widget *found;
  if (!root || !root->visible) {
    return 0;
  }
  if (root->focused) {
    return root;
  }
  for (i = 0u; i < root->child_count; ++i) {
    found = capy_widget_get_focused(root->children[i]);
    if (found) {
      return found;
    }
  }
  return 0;
}

struct capy_focus_pass {
  int has_current;
  int16_t current_tab;
  uint32_t current_dfs;
  uint32_t dfs_counter;
  struct capy_widget *next_w;
  int16_t next_tab;
  uint32_t next_dfs;
  struct capy_widget *prev_w;
  int16_t prev_tab;
  uint32_t prev_dfs;
  struct capy_widget *min_w;
  int16_t min_tab;
  uint32_t min_dfs;
  struct capy_widget *max_w;
  int16_t max_tab;
  uint32_t max_dfs;
};

static int capy_focus_cmp(int16_t a_tab, uint32_t a_dfs, int16_t b_tab,
                          uint32_t b_dfs) {
  if (a_tab < b_tab) {
    return -1;
  }
  if (a_tab > b_tab) {
    return 1;
  }
  if (a_dfs < b_dfs) {
    return -1;
  }
  if (a_dfs > b_dfs) {
    return 1;
  }
  return 0;
}

static void capy_focus_find_current(struct capy_widget *w,
                                    struct capy_widget *target,
                                    uint32_t *dfs_counter, uint32_t *out_dfs,
                                    int *found) {
  uint32_t i;
  if (!w || !w->visible || *found) {
    return;
  }
  if (w == target) {
    *out_dfs = *dfs_counter;
    *found = 1;
  }
  ++(*dfs_counter);
  for (i = 0u; i < w->child_count; ++i) {
    capy_focus_find_current(w->children[i], target, dfs_counter, out_dfs,
                            found);
  }
}

static void capy_focus_visit(struct capy_widget *w,
                             struct capy_focus_pass *p) {
  uint32_t w_dfs;
  int16_t w_tab;
  uint32_t i;
  if (!w || !w->visible) {
    return;
  }
  w_dfs = p->dfs_counter++;
  if (w->focusable && w->enabled) {
    w_tab = w->tab_index;
    if (!p->min_w ||
        capy_focus_cmp(w_tab, w_dfs, p->min_tab, p->min_dfs) < 0) {
      p->min_w = w;
      p->min_tab = w_tab;
      p->min_dfs = w_dfs;
    }
    if (!p->max_w ||
        capy_focus_cmp(w_tab, w_dfs, p->max_tab, p->max_dfs) > 0) {
      p->max_w = w;
      p->max_tab = w_tab;
      p->max_dfs = w_dfs;
    }
    if (p->has_current &&
        capy_focus_cmp(w_tab, w_dfs, p->current_tab, p->current_dfs) > 0) {
      if (!p->next_w ||
          capy_focus_cmp(w_tab, w_dfs, p->next_tab, p->next_dfs) < 0) {
        p->next_w = w;
        p->next_tab = w_tab;
        p->next_dfs = w_dfs;
      }
    }
    if (p->has_current &&
        capy_focus_cmp(w_tab, w_dfs, p->current_tab, p->current_dfs) < 0) {
      if (!p->prev_w ||
          capy_focus_cmp(w_tab, w_dfs, p->prev_tab, p->prev_dfs) > 0) {
        p->prev_w = w;
        p->prev_tab = w_tab;
        p->prev_dfs = w_dfs;
      }
    }
  }
  for (i = 0u; i < w->child_count; ++i) {
    capy_focus_visit(w->children[i], p);
  }
}

struct capy_widget *capy_widget_focus_next(struct capy_widget *root,
                                           struct capy_widget *current) {
  struct capy_focus_pass p;
  uint32_t counter = 0u;
  int found = 0;
  uint32_t current_dfs = 0u;
  capy_widget_zero(&p, sizeof(p));
  if (current) {
    capy_focus_find_current(root, current, &counter, &current_dfs, &found);
    if (found && current->focusable && current->enabled && current->visible) {
      p.has_current = 1;
      p.current_tab = current->tab_index;
      p.current_dfs = current_dfs;
    }
  }
  p.dfs_counter = 0u;
  capy_focus_visit(root, &p);
  return p.next_w ? p.next_w : p.min_w;
}

struct capy_widget *capy_widget_focus_prev(struct capy_widget *root,
                                           struct capy_widget *current) {
  struct capy_focus_pass p;
  uint32_t counter = 0u;
  int found = 0;
  uint32_t current_dfs = 0u;
  capy_widget_zero(&p, sizeof(p));
  if (current) {
    capy_focus_find_current(root, current, &counter, &current_dfs, &found);
    if (found && current->focusable && current->enabled && current->visible) {
      p.has_current = 1;
      p.current_tab = current->tab_index;
      p.current_dfs = current_dfs;
    }
  }
  p.dfs_counter = 0u;
  capy_focus_visit(root, &p);
  return p.prev_w ? p.prev_w : p.max_w;
}

static uint16_t capy_text_len(const char *s, uint16_t max) {
  uint16_t i = 0u;
  if (!s) {
    return 0u;
  }
  while (i < max && s[i] != '\0') {
    ++i;
  }
  return i;
}

static int capy_utf8_validate(const char *s, uint16_t len) {
  uint16_t i = 0u;
  uint8_t b;
  uint16_t need;
  uint16_t j;
  if (!s) {
    return -1;
  }
  while (i < len) {
    b = (uint8_t)s[i];
    if (b < 0x80u) {
      need = 0u;
    } else if ((b & 0xE0u) == 0xC0u) {
      need = 1u;
    } else if ((b & 0xF0u) == 0xE0u) {
      need = 2u;
    } else if ((b & 0xF8u) == 0xF0u) {
      need = 3u;
    } else {
      return -1;
    }
    if ((uint32_t)i + 1u + need > (uint32_t)len) {
      return -1;
    }
    for (j = 1u; j <= need; ++j) {
      if (((uint8_t)s[i + j] & 0xC0u) != 0x80u) {
        return -1;
      }
    }
    i = (uint16_t)(i + 1u + need);
  }
  return 0;
}

static uint16_t capy_utf8_prev_char_size(const char *s, uint16_t caret) {
  uint16_t i;
  if (caret == 0u) {
    return 0u;
  }
  i = (uint16_t)(caret - 1u);
  while (i > 0u && ((uint8_t)s[i] & 0xC0u) == 0x80u) {
    --i;
  }
  return (uint16_t)(caret - i);
}

static uint16_t capy_utf8_char_size_at(const char *s, uint16_t pos,
                                       uint16_t len) {
  uint8_t lead;
  uint16_t need;
  if (pos >= len) {
    return 0u;
  }
  lead = (uint8_t)s[pos];
  if (lead < 0x80u) {
    need = 0u;
  } else if ((lead & 0xE0u) == 0xC0u) {
    need = 1u;
  } else if ((lead & 0xF0u) == 0xE0u) {
    need = 2u;
  } else if ((lead & 0xF8u) == 0xF0u) {
    need = 3u;
  } else {
    return 1u;
  }
  if ((uint32_t)pos + 1u + need > (uint32_t)len) {
    return 1u;
  }
  return (uint16_t)(1u + need);
}

static void capy_text_shift_right(char *s, uint16_t from, uint16_t count,
                                  uint16_t shift) {
  uint16_t i;
  if (shift == 0u) {
    return;
  }
  for (i = count; i > 0u; --i) {
    s[from + i - 1u + shift] = s[from + i - 1u];
  }
}

static void capy_text_shift_left(char *s, uint16_t from, uint16_t count,
                                 uint16_t shift) {
  uint16_t i;
  if (shift == 0u) {
    return;
  }
  for (i = 0u; i < count; ++i) {
    s[from - shift + i] = s[from + i];
  }
}

static void capy_textbox_clamp_caret(struct capy_widget *w) {
  uint16_t len = capy_text_len(w->text, CAPY_WIDGET_MAX_TEXT);
  if (w->text_edit.caret > len) {
    w->text_edit.caret = len;
  }
  if (w->text_edit.sel_start > len) {
    w->text_edit.sel_start = len;
  }
  if (w->text_edit.sel_end > len) {
    w->text_edit.sel_end = len;
  }
}

int capy_textbox_insert(struct capy_widget *widget, const char *utf8,
                        uint16_t len) {
  uint16_t cur_len;
  uint16_t caret;
  uint16_t i;
  if (!widget || widget->type != CAPY_WIDGET_TEXTBOX || !utf8) {
    return -1;
  }
  if (widget->text_edit.readonly) {
    return -1;
  }
  if (len == 0u) {
    return 0;
  }
  if (capy_utf8_validate(utf8, len) != 0) {
    return -1;
  }
  capy_textbox_clamp_caret(widget);
  cur_len = capy_text_len(widget->text, CAPY_WIDGET_MAX_TEXT);
  if (widget->text_edit.sel_start != widget->text_edit.sel_end) {
    uint16_t sel_size =
        (uint16_t)(widget->text_edit.sel_end - widget->text_edit.sel_start);
    capy_text_shift_left(widget->text, widget->text_edit.sel_end,
                         (uint16_t)(cur_len - widget->text_edit.sel_end + 1u),
                         sel_size);
    cur_len = (uint16_t)(cur_len - sel_size);
    widget->text_edit.caret = widget->text_edit.sel_start;
    widget->text_edit.sel_end = widget->text_edit.sel_start;
  }
  if ((uint32_t)cur_len + (uint32_t)len + 1u > (uint32_t)CAPY_WIDGET_MAX_TEXT) {
    return -1;
  }
  caret = widget->text_edit.caret;
  capy_text_shift_right(widget->text, caret,
                        (uint16_t)(cur_len - caret + 1u), len);
  for (i = 0u; i < len; ++i) {
    widget->text[caret + i] = utf8[i];
  }
  widget->text_edit.caret = (uint16_t)(caret + len);
  widget->text_edit.sel_start = widget->text_edit.caret;
  widget->text_edit.sel_end = widget->text_edit.caret;
  return 0;
}

int capy_textbox_delete(struct capy_widget *widget, int16_t count) {
  uint16_t cur_len;
  uint16_t to_delete;
  uint16_t pos;
  uint16_t sz;
  int16_t n;
  if (!widget || widget->type != CAPY_WIDGET_TEXTBOX) {
    return -1;
  }
  if (widget->text_edit.readonly) {
    return -1;
  }
  capy_textbox_clamp_caret(widget);
  cur_len = capy_text_len(widget->text, CAPY_WIDGET_MAX_TEXT);
  if (widget->text_edit.sel_start != widget->text_edit.sel_end) {
    uint16_t sel_size =
        (uint16_t)(widget->text_edit.sel_end - widget->text_edit.sel_start);
    capy_text_shift_left(widget->text, widget->text_edit.sel_end,
                         (uint16_t)(cur_len - widget->text_edit.sel_end + 1u),
                         sel_size);
    widget->text_edit.caret = widget->text_edit.sel_start;
    widget->text_edit.sel_end = widget->text_edit.sel_start;
    return 0;
  }
  if (count == 0) {
    return 0;
  }
  if (count < 0) {
    to_delete = 0u;
    n = count;
    while (n < 0 && widget->text_edit.caret > 0u) {
      uint16_t prev = capy_utf8_prev_char_size(widget->text,
                                                widget->text_edit.caret);
      if (prev == 0u) {
        break;
      }
      capy_text_shift_left(widget->text, widget->text_edit.caret,
                           (uint16_t)(cur_len - widget->text_edit.caret + 1u),
                           prev);
      widget->text_edit.caret = (uint16_t)(widget->text_edit.caret - prev);
      cur_len = (uint16_t)(cur_len - prev);
      to_delete = (uint16_t)(to_delete + prev);
      ++n;
    }
    widget->text_edit.sel_start = widget->text_edit.caret;
    widget->text_edit.sel_end = widget->text_edit.caret;
    return 0;
  }
  to_delete = 0u;
  pos = widget->text_edit.caret;
  n = count;
  while (n > 0 && pos < cur_len) {
    sz = capy_utf8_char_size_at(widget->text, pos, cur_len);
    if (sz == 0u) {
      break;
    }
    pos = (uint16_t)(pos + sz);
    to_delete = (uint16_t)(to_delete + sz);
    --n;
  }
  if (to_delete > 0u) {
    capy_text_shift_left(widget->text,
                         (uint16_t)(widget->text_edit.caret + to_delete),
                         (uint16_t)(cur_len - widget->text_edit.caret -
                                    to_delete + 1u),
                         to_delete);
  }
  widget->text_edit.sel_start = widget->text_edit.caret;
  widget->text_edit.sel_end = widget->text_edit.caret;
  return 0;
}

void capy_textbox_set_selection(struct capy_widget *widget, uint16_t start,
                                uint16_t end) {
  uint16_t len;
  uint16_t t;
  if (!widget || widget->type != CAPY_WIDGET_TEXTBOX) {
    return;
  }
  len = capy_text_len(widget->text, CAPY_WIDGET_MAX_TEXT);
  if (start > len) {
    start = len;
  }
  if (end > len) {
    end = len;
  }
  if (start > end) {
    t = start;
    start = end;
    end = t;
  }
  widget->text_edit.sel_start = start;
  widget->text_edit.sel_end = end;
}

int capy_textbox_copy(struct capy_widget *widget, char *out, uint16_t out_cap) {
  uint16_t start;
  uint16_t end;
  uint16_t size;
  uint16_t i;
  if (!widget || widget->type != CAPY_WIDGET_TEXTBOX || !out ||
      out_cap == 0u) {
    return -1;
  }
  if (widget->text_edit.sel_start != widget->text_edit.sel_end) {
    start = widget->text_edit.sel_start;
    end = widget->text_edit.sel_end;
  } else {
    start = 0u;
    end = capy_text_len(widget->text, CAPY_WIDGET_MAX_TEXT);
  }
  size = (uint16_t)(end - start);
  if ((uint32_t)size + 1u > (uint32_t)out_cap) {
    return -1;
  }
  for (i = 0u; i < size; ++i) {
    out[i] = widget->text[start + i];
  }
  out[size] = '\0';
  return (int)size;
}

int capy_widget_ime_compose(struct capy_widget *widget, const char *preedit,
                            uint16_t len) {
  if (!widget || widget->type != CAPY_WIDGET_TEXTBOX) {
    return -1;
  }
  if (len > 0u && capy_utf8_validate(preedit, len) != 0) {
    return -1;
  }
  /* v0.4 stub: validates and accepts; real composition state lands in v1.8. */
  (void)preedit;
  (void)len;
  return 0;
}

int capy_widget_dispatch_key(struct capy_widget *root,
                             const struct capy_widget_event *event) {
  struct capy_widget *focused;
  struct capy_widget *target;
  if (!root || !event) {
    return 0;
  }
  if (event->type != CAPY_WIDGET_EVENT_KEY_DOWN) {
    return 0;
  }
  focused = capy_widget_get_focused(root);
  if (event->keycode == CAPY_KEY_TAB) {
    if ((event->modifiers & CAPY_MOD_SHIFT) != 0u) {
      target = capy_widget_focus_prev(root, focused);
    } else {
      target = capy_widget_focus_next(root, focused);
    }
    if (target) {
      capy_widget_clear_focus(root);
      target->focused = 1u;
    }
    return 1;
  }
  if (!focused || !focused->enabled) {
    return 0;
  }
  if (event->keycode == CAPY_KEY_ENTER) {
    if (focused->type == CAPY_WIDGET_BUTTON ||
        focused->type == CAPY_WIDGET_MENU_ITEM ||
        focused->type == CAPY_WIDGET_DIALOG) {
      if (focused->on_click) {
        focused->on_click(focused, focused->user_data);
      }
      return 1;
    }
    return 0;
  }
  if (event->keycode == CAPY_KEY_SPACE) {
    if (focused->type == CAPY_WIDGET_CHECKBOX) {
      focused->checked = focused->checked ? 0u : 1u;
      if (focused->on_change) {
        focused->on_change(focused, focused->user_data);
      }
      return 1;
    }
    if (focused->type == CAPY_WIDGET_BUTTON) {
      if (focused->on_click) {
        focused->on_click(focused, focused->user_data);
      }
      return 1;
    }
    return 0;
  }
  if (event->keycode == CAPY_KEY_ESCAPE) {
    if (focused->type == CAPY_WIDGET_DIALOG) {
      if (focused->on_click) {
        focused->on_click(focused, focused->user_data);
      }
      return 1;
    }
    return 0;
  }
  return 0;
}

void capy_anim_start(struct capy_anim *a, uint32_t now, uint32_t duration,
                     int32_t from, int32_t to, uint8_t easing) {
  if (!a) {
    return;
  }
  a->start_tick = now;
  a->duration_ticks = duration;
  a->from = from;
  a->to = to;
  a->easing = easing;
  a->active = 1u;
  a->reserved = 0u;
}

static uint32_t capy_anim_apply_easing(uint8_t easing, uint32_t t_fp) {
  const uint64_t one = 65536ull;
  uint64_t t = (uint64_t)t_fp;
  uint64_t inv;
  switch (easing) {
    case CAPY_ANIM_EASE_IN:
      return (uint32_t)((t * t) / one);
    case CAPY_ANIM_EASE_OUT:
      inv = one - t;
      return (uint32_t)(one - (inv * inv) / one);
    case CAPY_ANIM_EASE_IN_OUT:
      if (t < (one / 2ull)) {
        return (uint32_t)((2ull * t * t) / one);
      }
      inv = one - t;
      return (uint32_t)(one - (2ull * inv * inv) / one);
    case CAPY_ANIM_LINEAR:
    default:
      return t_fp;
  }
}

static int32_t capy_anim_interp(int32_t from, int32_t to, uint32_t eased) {
  int64_t delta = (int64_t)to - (int64_t)from;
  int64_t scaled = (delta * (int64_t)eased) / 65536;
  return (int32_t)((int64_t)from + scaled);
}

int capy_anim_sample(const struct capy_anim *a, uint32_t now, int32_t *out) {
  uint32_t elapsed;
  uint32_t t_fp;
  uint32_t eased;
  if (!a || !out) {
    return -1;
  }
  if (!a->active || a->duration_ticks == 0u) {
    *out = a->from;
    return 0;
  }
  if (now <= a->start_tick) {
    *out = a->from;
    return 0;
  }
  elapsed = now - a->start_tick;
  if (elapsed >= a->duration_ticks) {
    *out = a->to;
    return 0;
  }
  t_fp = (uint32_t)(((uint64_t)elapsed * 65536ull) /
                    (uint64_t)a->duration_ticks);
  eased = capy_anim_apply_easing(a->easing, t_fp);
  *out = capy_anim_interp(a->from, a->to, eased);
  return 0;
}

void capy_widget_tick(struct capy_widget *root, uint32_t now) {
  uint32_t i;
  if (!root) {
    return;
  }
  (void)now;
  for (i = 0u; i < root->child_count; ++i) {
    capy_widget_tick(root->children[i], now);
  }
}

struct capy_theme capy_theme_default_light(void) {
  struct capy_theme t;
  capy_widget_zero(&t, sizeof(t));
  t.tokens[CAPY_TOKEN_BG_BASE] = 0xFFF5F6F8u;
  t.tokens[CAPY_TOKEN_BG_RAISED] = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_BG_SUNKEN] = 0xFFEEEFF2u;
  t.tokens[CAPY_TOKEN_FG_PRIMARY] = 0xFF1A1B1Fu;
  t.tokens[CAPY_TOKEN_FG_MUTED] = 0xFF6B7280u;
  t.tokens[CAPY_TOKEN_FG_INVERSE] = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_ACCENT] = 0xFF2563EBu;
  t.tokens[CAPY_TOKEN_ACCENT_HOVER] = 0xFF3B82F6u;
  t.tokens[CAPY_TOKEN_BORDER] = 0xFFE5E7EBu;
  t.tokens[CAPY_TOKEN_BORDER_FOCUS] = 0xFF2563EBu;
  t.tokens[CAPY_TOKEN_FOCUS_RING] = 0xFF60A5FAu;
  t.tokens[CAPY_TOKEN_DANGER] = 0xFFEF4444u;
  t.tokens[CAPY_TOKEN_WARNING] = 0xFFF59E0Bu;
  t.tokens[CAPY_TOKEN_SUCCESS] = 0xFF22C55Eu;
  t.tokens[CAPY_TOKEN_INFO] = 0xFF60A5FAu;
  t.tokens[CAPY_TOKEN_DISABLED] = 0xFF9CA3AFu;
  t.variant = (uint8_t)CAPY_THEME_VARIANT_LIGHT;
  t.high_contrast = 0u;
  t.version = (uint16_t)CAPY_THEME_FORMAT_VERSION;
  return t;
}

struct capy_theme capy_theme_default_dark(void) {
  struct capy_theme t;
  capy_widget_zero(&t, sizeof(t));
  t.tokens[CAPY_TOKEN_BG_BASE] = 0xFF1A1B1Fu;
  t.tokens[CAPY_TOKEN_BG_RAISED] = 0xFF24262Cu;
  t.tokens[CAPY_TOKEN_BG_SUNKEN] = 0xFF14161Au;
  t.tokens[CAPY_TOKEN_FG_PRIMARY] = 0xFFE8ECEFu;
  t.tokens[CAPY_TOKEN_FG_MUTED] = 0xFF8B95A1u;
  t.tokens[CAPY_TOKEN_FG_INVERSE] = 0xFF1A1B1Fu;
  t.tokens[CAPY_TOKEN_ACCENT] = 0xFF3B82F6u;
  t.tokens[CAPY_TOKEN_ACCENT_HOVER] = 0xFF60A5FAu;
  t.tokens[CAPY_TOKEN_BORDER] = 0xFF4B5563u;
  t.tokens[CAPY_TOKEN_BORDER_FOCUS] = 0xFF3B82F6u;
  t.tokens[CAPY_TOKEN_FOCUS_RING] = 0xFF60A5FAu;
  t.tokens[CAPY_TOKEN_DANGER] = 0xFFEF4444u;
  t.tokens[CAPY_TOKEN_WARNING] = 0xFFF59E0Bu;
  t.tokens[CAPY_TOKEN_SUCCESS] = 0xFF22C55Eu;
  t.tokens[CAPY_TOKEN_INFO] = 0xFF60A5FAu;
  t.tokens[CAPY_TOKEN_DISABLED] = 0xFF6B7280u;
  t.variant = (uint8_t)CAPY_THEME_VARIANT_DARK;
  t.high_contrast = 0u;
  t.version = (uint16_t)CAPY_THEME_FORMAT_VERSION;
  return t;
}

struct capy_theme capy_theme_high_contrast(void) {
  struct capy_theme t;
  capy_widget_zero(&t, sizeof(t));
  t.tokens[CAPY_TOKEN_BG_BASE] = 0xFF000000u;
  t.tokens[CAPY_TOKEN_BG_RAISED] = 0xFF000000u;
  t.tokens[CAPY_TOKEN_BG_SUNKEN] = 0xFF000000u;
  t.tokens[CAPY_TOKEN_FG_PRIMARY] = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_FG_MUTED] = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_FG_INVERSE] = 0xFF000000u;
  t.tokens[CAPY_TOKEN_ACCENT] = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_ACCENT_HOVER] = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_BORDER] = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_BORDER_FOCUS] = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_FOCUS_RING] = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_DANGER] = 0xFFFF0000u;
  t.tokens[CAPY_TOKEN_WARNING] = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_SUCCESS] = 0xFF00FF00u;
  t.tokens[CAPY_TOKEN_INFO] = 0xFF00FFFFu;
  t.tokens[CAPY_TOKEN_DISABLED] = 0xFF808080u;
  t.variant = (uint8_t)CAPY_THEME_VARIANT_HIGH_CONTRAST;
  t.high_contrast = 1u;
  t.version = (uint16_t)CAPY_THEME_FORMAT_VERSION;
  return t;
}

void capy_theme_apply(struct capy_widget_context *ctx,
                      const struct capy_theme *theme) {
  if (!ctx || !theme) {
    return;
  }
  ctx->theme = *theme;
}

uint32_t capy_theme_resolve(const struct capy_theme *theme, uint8_t token,
                            uint32_t fallback) {
  if (!theme || token == 0u ||
      (uint32_t)token >= (uint32_t)CAPY_TOKEN_COUNT) {
    return fallback;
  }
  return theme->tokens[token];
}

/* ── v0.9: text metrics hook + deterministic fallback ─────────────────── */

void capy_widget_set_text_metrics_hook(struct capy_widget_context *ctx,
                                       capy_text_metrics_fn fn,
                                       void *user_data) {
  if (!ctx) {
    return;
  }
  ctx->text_metrics_fn = fn;
  ctx->text_metrics_user = user_data;
}

int capy_widget_measure_text(const struct capy_widget_context *ctx,
                             uint16_t font_id, uint8_t font_size,
                             const char *text, uint16_t len,
                             uint32_t *out_width, uint32_t *out_height) {
  uint32_t effective_size;
  if (!out_width || !out_height) {
    return -1;
  }
  effective_size = (font_size > 0u) ? (uint32_t)font_size : 16u;
  /* Empty/NULL text always reports zero width regardless of the hook so the
   * fallback stays trivial and deterministic. */
  if (!text || len == 0u) {
    *out_width = 0u;
    *out_height = effective_size;
    return 0;
  }
  if (ctx && ctx->text_metrics_fn) {
    uint32_t w = 0u;
    uint32_t h = 0u;
    int rc = ctx->text_metrics_fn(font_id, font_size, text, len, &w, &h,
                                  ctx->text_metrics_user);
    if (rc == 0) {
      *out_width = w;
      *out_height = h;
      return 0;
    }
    /* fall through to deterministic fallback on hook failure */
  }
  *out_width = (effective_size / 2u) * (uint32_t)len;
  *out_height = effective_size;
  return 0;
}

/* ── v0.11: accessibility tree snapshot ───────────────────────────────── */

#define CAPY_A11Y_FNV_OFFSET 0x811C9DC5u
#define CAPY_A11Y_FNV_PRIME  0x01000193u

static uint32_t capy_a11y_hash_step(uint32_t state, uint32_t value) {
  uint32_t i;
  for (i = 0u; i < 4u; ++i) {
    uint8_t byte = (uint8_t)((value >> (i * 8u)) & 0xFFu);
    state ^= (uint32_t)byte;
    state *= CAPY_A11Y_FNV_PRIME;
  }
  return state;
}

static const char *capy_a11y_role_for(enum capy_widget_type type) {
  switch (type) {
    case CAPY_WIDGET_LABEL:     return "label";
    case CAPY_WIDGET_BUTTON:    return "button";
    case CAPY_WIDGET_TEXTBOX:   return "textbox";
    case CAPY_WIDGET_CHECKBOX:  return "checkbox";
    case CAPY_WIDGET_LIST:      return "list";
    case CAPY_WIDGET_PANEL:     return "panel";
    case CAPY_WIDGET_SCROLLBAR: return "scrollbar";
    case CAPY_WIDGET_MENUBAR:   return "menu";
    case CAPY_WIDGET_MENU_ITEM: return "menuitem";
    case CAPY_WIDGET_DIALOG:    return "dialog";
    case CAPY_WIDGET_PROGRESS:  return "progressbar";
    case CAPY_WIDGET_TABS:      return "tab";
    case CAPY_WIDGET_NONE:
    default:
      return "none";
  }
}

static uint32_t capy_a11y_flags_for(const struct capy_widget *w) {
  uint32_t flags = 0u;
  if (w->focused) {
    flags |= CAPY_A11Y_FOCUSED;
  }
  if (w->checked) {
    flags |= CAPY_A11Y_CHECKED;
  }
  if (!w->enabled) {
    flags |= CAPY_A11Y_DISABLED;
  }
  if (!w->visible) {
    flags |= CAPY_A11Y_HIDDEN;
  }
  if (w->type == CAPY_WIDGET_TEXTBOX && w->text_edit.readonly) {
    flags |= CAPY_A11Y_READONLY;
  }
  return flags;
}

static int capy_a11y_walk(struct capy_widget *w, uint32_t parent_id,
                          uint32_t my_id, struct capy_a11y_node *out,
                          uint32_t cap, uint32_t *written) {
  uint32_t i;
  if (!w) {
    return 0;
  }
  if (*written >= cap) {
    return -1;
  }
  out[*written].widget_id = my_id;
  out[*written].parent_id = parent_id;
  out[*written].role = capy_a11y_role_for(w->type);
  out[*written].label = (w->text[0] != '\0') ? w->text : "Untitled";
  out[*written].description = 0;
  out[*written].state_flags = capy_a11y_flags_for(w);
  ++(*written);
  for (i = 0u; i < w->child_count; ++i) {
    uint32_t child_id = capy_a11y_hash_step(my_id, i);
    if (capy_a11y_walk(w->children[i], my_id, child_id, out, cap, written) !=
        0) {
      return -1;
    }
  }
  return 0;
}

int capy_widget_a11y_snapshot(struct capy_widget *root,
                              struct capy_a11y_node *out, uint32_t cap) {
  uint32_t written = 0u;
  if (!root) {
    return -1;
  }
  if (cap > 0u && !out) {
    return -1;
  }
  if (cap == 0u) {
    return -1;
  }
  if (capy_a11y_walk(root, 0u, CAPY_A11Y_FNV_OFFSET, out, cap, &written) !=
      0) {
    return -1;
  }
  return (int)written;
}

/* ── v0.13: locale + RTL + plural + format ────────────────────────────── */

struct capy_locale capy_locale_default(void) {
  struct capy_locale l;
  uint32_t i;
  /* "en-US" + NUL into a 16-byte buffer, rest zeroed. */
  const char *tag = "en-US";
  for (i = 0u; i < sizeof(l.tag); ++i) {
    l.tag[i] = (i < 5u) ? tag[i] : '\0';
  }
  l.rtl = 0u;
  l.plural_rule = (uint8_t)CAPY_PLURAL_EN;
  l.reserved = 0u;
  return l;
}

void capy_context_set_locale(struct capy_widget_context *ctx,
                             const struct capy_locale *l) {
  if (!ctx) {
    return;
  }
  if (!l) {
    ctx->locale = capy_locale_default();
    return;
  }
  ctx->locale = *l;
}

int capy_locale_plural(const struct capy_locale *l, uint32_t n) {
  uint8_t rule = l ? l->plural_rule : (uint8_t)CAPY_PLURAL_EN;
  switch ((enum capy_plural_rule)rule) {
    case CAPY_PLURAL_EN:
    case CAPY_PLURAL_PT:
      return (n == 1u) ? 0 : 1;
    case CAPY_PLURAL_AR: {
      uint32_t mod100 = n % 100u;
      if (n == 0u) return 0;
      if (n == 1u) return 1;
      if (n == 2u) return 2;
      if (mod100 >= 3u && mod100 <= 10u) return 3;
      if (mod100 >= 11u && mod100 <= 99u) return 4;
      return 5;
    }
    case CAPY_PLURAL_RU: {
      uint32_t mod10 = n % 10u;
      uint32_t mod100 = n % 100u;
      if (mod10 == 1u && mod100 != 11u) return 0;
      if (mod10 >= 2u && mod10 <= 4u && (mod100 < 12u || mod100 > 14u)) {
        return 1;
      }
      return 2;
    }
    case CAPY_PLURAL_OTHER:
    default:
      return 0;
  }
}

static int capy_fmt_append_char(char *out, uint16_t cap, uint16_t *written,
                                char ch) {
  /* Reserve one byte for the terminating NUL: written must stay < cap-1. */
  if ((uint32_t)*written + 1u >= (uint32_t)cap) {
    return -1;
  }
  out[*written] = ch;
  ++(*written);
  return 0;
}

static int capy_fmt_u32(uint32_t value, uint32_t base, char *out,
                        uint16_t cap, uint16_t *written) {
  char buf[11]; /* enough for 32-bit value in base 10 or 16 */
  uint32_t n = 0u;
  if (base != 10u && base != 16u) {
    return -1;
  }
  if (value == 0u) {
    buf[n++] = '0';
  } else {
    while (value > 0u && n < sizeof(buf)) {
      uint32_t digit = value % base;
      buf[n++] = (digit < 10u) ? (char)('0' + (int)digit)
                               : (char)('a' + (int)(digit - 10u));
      value /= base;
    }
  }
  while (n > 0u) {
    --n;
    if (capy_fmt_append_char(out, cap, written, buf[n]) != 0) {
      return -1;
    }
  }
  return 0;
}

static int capy_fmt_i32(int32_t value, char *out, uint16_t cap,
                        uint16_t *written) {
  uint32_t u;
  if (value < 0) {
    if (capy_fmt_append_char(out, cap, written, '-') != 0) {
      return -1;
    }
    /* Negation through int64_t avoids UB on INT32_MIN. */
    u = (uint32_t)(-(int64_t)value);
  } else {
    u = (uint32_t)value;
  }
  return capy_fmt_u32(u, 10u, out, cap, written);
}

int capy_locale_format(const struct capy_locale *l, char *out,
                       uint16_t out_cap, const char *fmt, ...) {
  va_list ap;
  uint16_t written = 0u;
  (void)l; /* reserved for future locale-aware digit shaping */
  if (!out || out_cap == 0u || !fmt) {
    return -1;
  }
  va_start(ap, fmt);
  while (*fmt != '\0') {
    if (*fmt != '%') {
      if (capy_fmt_append_char(out, out_cap, &written, *fmt) != 0) {
        va_end(ap);
        return -1;
      }
      ++fmt;
      continue;
    }
    /* Consume '%' and dispatch on the next character. */
    ++fmt;
    if (*fmt == '\0') {
      /* Trailing '%' is an error rather than silent drop. */
      va_end(ap);
      return -1;
    }
    switch (*fmt) {
      case 'd': {
        int v = va_arg(ap, int);
        if (capy_fmt_i32((int32_t)v, out, out_cap, &written) != 0) {
          va_end(ap);
          return -1;
        }
        break;
      }
      case 'u': {
        unsigned int v = va_arg(ap, unsigned int);
        if (capy_fmt_u32((uint32_t)v, 10u, out, out_cap, &written) != 0) {
          va_end(ap);
          return -1;
        }
        break;
      }
      case 'x': {
        unsigned int v = va_arg(ap, unsigned int);
        if (capy_fmt_u32((uint32_t)v, 16u, out, out_cap, &written) != 0) {
          va_end(ap);
          return -1;
        }
        break;
      }
      case 's': {
        const char *s = va_arg(ap, const char *);
        while (s && *s != '\0') {
          if (capy_fmt_append_char(out, out_cap, &written, *s) != 0) {
            va_end(ap);
            return -1;
          }
          ++s;
        }
        break;
      }
      case '%': {
        if (capy_fmt_append_char(out, out_cap, &written, '%') != 0) {
          va_end(ap);
          return -1;
        }
        break;
      }
      default:
        /* Unknown specifier: fail closed rather than emit silently. */
        va_end(ap);
        return -1;
    }
    ++fmt;
  }
  va_end(ap);
  /* Always terminate within capacity (we reserved one slot). */
  out[written] = '\0';
  return (int)written;
}

/* ── v0.14: theme serialization (canonical line-oriented key=value) ────── */

static const char *capy_theme_token_name(uint8_t tok) {
  switch ((enum capy_token)tok) {
    case CAPY_TOKEN_BG_BASE:      return "bg_base";
    case CAPY_TOKEN_BG_RAISED:    return "bg_raised";
    case CAPY_TOKEN_BG_SUNKEN:    return "bg_sunken";
    case CAPY_TOKEN_FG_PRIMARY:   return "fg_primary";
    case CAPY_TOKEN_FG_MUTED:     return "fg_muted";
    case CAPY_TOKEN_FG_INVERSE:   return "fg_inverse";
    case CAPY_TOKEN_ACCENT:       return "accent";
    case CAPY_TOKEN_ACCENT_HOVER: return "accent_hover";
    case CAPY_TOKEN_BORDER:       return "border";
    case CAPY_TOKEN_BORDER_FOCUS: return "border_focus";
    case CAPY_TOKEN_FOCUS_RING:   return "focus_ring";
    case CAPY_TOKEN_DANGER:       return "danger";
    case CAPY_TOKEN_WARNING:      return "warning";
    case CAPY_TOKEN_SUCCESS:      return "success";
    case CAPY_TOKEN_INFO:         return "info";
    case CAPY_TOKEN_DISABLED:     return "disabled";
    case CAPY_TOKEN_NONE:
    case CAPY_TOKEN_COUNT:
    default:
      return 0;
  }
}

static int capy_theme_append_str(char *out, uint32_t cap, uint32_t *written,
                                 const char *s) {
  while (*s != '\0') {
    if (*written + 1u >= cap) {
      return -1;
    }
    out[*written] = *s;
    ++(*written);
    ++s;
  }
  return 0;
}

static int capy_theme_append_hex32(char *out, uint32_t cap, uint32_t *written,
                                   uint32_t value) {
  static const char hex[] = "0123456789ABCDEF";
  char buf[11];
  int i;
  buf[0] = '0';
  buf[1] = 'x';
  for (i = 0; i < 8; ++i) {
    buf[2 + i] = hex[(value >> (uint32_t)((7 - i) * 4)) & 0xFu];
  }
  buf[10] = '\0';
  return capy_theme_append_str(out, cap, written, buf);
}

int capy_theme_serialize(const struct capy_theme *t, char *out, uint32_t cap) {
  uint32_t written = 0u;
  uint32_t i;
  const char *variant_str;
  if (!t || !out || cap == 0u) {
    return -1;
  }
  switch ((uint32_t)t->variant) {
    case CAPY_THEME_VARIANT_LIGHT:         variant_str = "light"; break;
    case CAPY_THEME_VARIANT_DARK:          variant_str = "dark"; break;
    case CAPY_THEME_VARIANT_HIGH_CONTRAST: variant_str = "high_contrast"; break;
    default: return -1;
  }
  /* `version` is emitted as decimal per the canonical example in
   * `docs/roadmap/contracts/theme-serialization.md` (`version=2`). */
  if (capy_theme_append_str(out, cap, &written, "version=2\n") != 0) {
    return -1;
  }
  if (capy_theme_append_str(out, cap, &written, "variant=") != 0) {
    return -1;
  }
  if (capy_theme_append_str(out, cap, &written, variant_str) != 0) {
    return -1;
  }
  if (capy_theme_append_str(out, cap, &written, "\n") != 0) {
    return -1;
  }
  if (capy_theme_append_str(out, cap, &written, "high_contrast=") != 0) {
    return -1;
  }
  if (capy_theme_append_str(out, cap, &written,
                            t->high_contrast ? "1" : "0") != 0) {
    return -1;
  }
  if (capy_theme_append_str(out, cap, &written, "\n") != 0) {
    return -1;
  }
  for (i = 1u; i < (uint32_t)CAPY_TOKEN_COUNT; ++i) {
    const char *name = capy_theme_token_name((uint8_t)i);
    if (!name) {
      continue;
    }
    if (capy_theme_append_str(out, cap, &written, name) != 0) {
      return -1;
    }
    if (capy_theme_append_str(out, cap, &written, "=") != 0) {
      return -1;
    }
    if (capy_theme_append_hex32(out, cap, &written, t->tokens[i]) != 0) {
      return -1;
    }
    if (capy_theme_append_str(out, cap, &written, "\n") != 0) {
      return -1;
    }
  }
  if (written >= cap) {
    return -1;
  }
  out[written] = '\0';
  return (int)written;
}

static int capy_theme_range_eq_literal(const char *s, uint32_t start,
                                       uint32_t end, const char *literal) {
  uint32_t i = 0u;
  while (start + i < end && literal[i] != '\0') {
    if (s[start + i] != literal[i]) {
      return 0;
    }
    ++i;
  }
  return (start + i == end) && (literal[i] == '\0');
}

static int capy_theme_token_from_range(const char *s, uint32_t start,
                                       uint32_t end, uint8_t *out_tok) {
  uint32_t i;
  for (i = 1u; i < (uint32_t)CAPY_TOKEN_COUNT; ++i) {
    const char *name = capy_theme_token_name((uint8_t)i);
    if (name && capy_theme_range_eq_literal(s, start, end, name)) {
      *out_tok = (uint8_t)i;
      return 0;
    }
  }
  return -1;
}

static int capy_theme_parse_hex32(const char *s, uint32_t start, uint32_t end,
                                  uint32_t *out_value) {
  uint32_t v = 0u;
  uint32_t i = start;
  uint32_t digits = 0u;
  if (start >= end) {
    return -1;
  }
  if (i + 1u < end && s[i] == '0' && (s[i + 1u] == 'x' || s[i + 1u] == 'X')) {
    i += 2u;
  }
  if (i >= end) {
    return -1;
  }
  for (; i < end; ++i) {
    char c = s[i];
    uint32_t d;
    if (c >= '0' && c <= '9') {
      d = (uint32_t)(c - '0');
    } else if (c >= 'a' && c <= 'f') {
      d = (uint32_t)(c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      d = (uint32_t)(c - 'A' + 10);
    } else {
      return -1;
    }
    if (digits >= 8u) {
      return -1; /* more than 32 bits */
    }
    v = (v << 4) | d;
    ++digits;
  }
  if (digits == 0u) {
    return -1;
  }
  *out_value = v;
  return 0;
}

static int capy_theme_parse_decimal_u32(const char *s, uint32_t start,
                                        uint32_t end, uint32_t *out_value) {
  uint32_t i;
  uint32_t v = 0u;
  if (start >= end) {
    return -1;
  }
  for (i = start; i < end; ++i) {
    char c = s[i];
    if (c < '0' || c > '9') {
      return -1;
    }
    if (v > 429496729u) {
      return -1; /* would overflow uint32_t */
    }
    v = v * 10u + (uint32_t)(c - '0');
  }
  *out_value = v;
  return 0;
}

static int capy_theme_parse_bool(const char *s, uint32_t start, uint32_t end,
                                 uint8_t *out_value) {
  if (end - start != 1u) {
    return -1;
  }
  if (s[start] == '0') {
    *out_value = 0u;
    return 0;
  }
  if (s[start] == '1') {
    *out_value = 1u;
    return 0;
  }
  return -1;
}

/* ── v0.15: pool allocator + invalidate walker ────────────────────────── */

#define CAPY_WIDGET_POOL_ALIGN 16u

static uint32_t capy_widget_pool_align_up(uint32_t v, uint32_t align) {
  uint32_t mask = align - 1u;
  return (v + mask) & ~mask;
}

void capy_widget_pool_init(struct capy_widget_pool *p, void *buf,
                           uint32_t size) {
  if (!p) {
    return;
  }
  p->buffer = buf;
  p->size = buf ? size : 0u;
  p->used = 0u;
  p->high_water = 0u;
}

void capy_widget_pool_reset(struct capy_widget_pool *p) {
  if (!p) {
    return;
  }
  /* `high_water` deliberately retained for diagnostics. */
  p->used = 0u;
}

static void *capy_widget_pool_alloc(size_t size, void *user_data) {
  struct capy_widget_pool *p = (struct capy_widget_pool *)user_data;
  uint32_t aligned;
  uint32_t need;
  unsigned char *base;
  if (!p || !p->buffer || size == 0u) {
    return 0;
  }
  if ((uint64_t)size > 0xFFFFFFFFu) {
    return 0;
  }
  aligned = capy_widget_pool_align_up(p->used, CAPY_WIDGET_POOL_ALIGN);
  if (aligned > p->size) {
    return 0;
  }
  if ((uint32_t)size > p->size - aligned) {
    return 0;
  }
  need = aligned + (uint32_t)size;
  base = (unsigned char *)p->buffer + aligned;
  p->used = need;
  if (p->used > p->high_water) {
    p->high_water = p->used;
  }
  return base;
}

static void capy_widget_pool_free(void *ptr, void *user_data) {
  /* The pool is bulk-managed; per-pointer free is intentionally a no-op. */
  (void)ptr;
  (void)user_data;
}

struct capy_widget_allocator capy_widget_allocator_from_pool(
    struct capy_widget_pool *p) {
  struct capy_widget_allocator a;
  a.alloc = capy_widget_pool_alloc;
  a.free = capy_widget_pool_free;
  a.user_data = p;
  return a;
}

/* ── v1.3: rich animation (tracks + springs + Bezier) ─────────────────── */

int capy_anim_track_sample(const struct capy_anim_track *track, uint32_t now,
                           int32_t *out) {
  uint32_t base;
  uint32_t total;
  uint32_t eff;
  uint32_t i;
  if (!track || !out || track->count == 0u || !track->keyframes) {
    return -1;
  }
  base = track->keyframes[0].tick;
  /* Single keyframe or degenerate (zero-duration) track: clamp to first. */
  if (track->count == 1u ||
      track->keyframes[track->count - 1u].tick == base) {
    *out = track->keyframes[0].value;
    return 0;
  }
  total = track->keyframes[track->count - 1u].tick - base;
  if (now <= base) {
    *out = track->keyframes[0].value;
    return 0;
  }
  /* Loop policy. */
  if (track->loop == 0) {
    if (now - base >= total) {
      *out = track->keyframes[track->count - 1u].value;
      return 0;
    }
    eff = now - base;
  } else if (track->loop < 0) {
    eff = (now - base) % total;
  } else {
    uint64_t n_loops = (uint64_t)(uint32_t)track->loop;
    uint64_t span = (uint64_t)total * n_loops;
    if ((uint64_t)(now - base) >= span) {
      *out = track->keyframes[track->count - 1u].value;
      return 0;
    }
    eff = (uint32_t)(((uint64_t)(now - base)) % (uint64_t)total);
  }
  /* Linear segment search: typical tracks are short (≤ tens of keyframes). */
  for (i = 1u; i < track->count; ++i) {
    uint32_t kf_eff = track->keyframes[i].tick - base;
    if (kf_eff > eff) {
      const struct capy_anim_keyframe *a = &track->keyframes[i - 1u];
      const struct capy_anim_keyframe *b = &track->keyframes[i];
      uint32_t seg_len = b->tick - a->tick;
      uint32_t seg_pos = eff - (a->tick - base);
      uint32_t t_fp;
      uint32_t eased;
      if (seg_len == 0u) {
        *out = a->value;
        return 0;
      }
      t_fp = (uint32_t)(((uint64_t)seg_pos * 65536ull) / (uint64_t)seg_len);
      eased = capy_anim_apply_easing(a->easing, t_fp);
      *out = capy_anim_interp(a->value, b->value, eased);
      return 0;
    }
  }
  *out = track->keyframes[track->count - 1u].value;
  return 0;
}

void capy_spring_step(struct capy_spring *s, uint32_t dt_ms) {
  uint32_t step;
  if (!s || dt_ms == 0u) {
    return;
  }
  /* 1 ms symplectic-Euler substeps. Each substep:
   *   disp        = target - position                                (units)
   *   spring_acc  = (stiffness_q8_8 * disp) >> 8                     (units)
   *   damp_acc    = (damping_q8_8 * velocity_q24_8) >> 16            (units)
   *   accel       = spring_acc - damp_acc                            (units)
   *   velocity   += accel << 8        (Q24.8 += units<<8)
   *   position   += velocity >> 8     (units += Q24.8>>8)
   * Both updates clamp to int32. With damping > 0 the system bleeds energy
   * and converges; with damping == 0 it conserves energy and oscillates. */
  for (step = 0u; step < dt_ms; ++step) {
    /* All shifts replaced by `/` and `*` because right-shift on negative
     * signed values is implementation-defined and left-shift of negative
     * signed values is undefined behaviour under C11. Truncate-toward-zero
     * division keeps the integrator deterministic across compilers. */
    int64_t disp = (int64_t)s->target - (int64_t)s->position;
    int64_t spring_acc = ((int64_t)s->stiffness * disp) / 256;
    int64_t damp_acc =
        ((int64_t)s->damping * (int64_t)s->velocity) / 65536;
    int64_t accel = spring_acc - damp_acc;
    int64_t new_v = (int64_t)s->velocity + (accel * 256);
    int64_t new_p;
    if (new_v > (int64_t)INT32_MAX) {
      new_v = (int64_t)INT32_MAX;
    } else if (new_v < (int64_t)INT32_MIN) {
      new_v = (int64_t)INT32_MIN;
    }
    s->velocity = (int32_t)new_v;
    new_p = (int64_t)s->position + ((int64_t)s->velocity / 256);
    if (new_p > (int64_t)INT32_MAX) {
      new_p = (int64_t)INT32_MAX;
    } else if (new_p < (int64_t)INT32_MIN) {
      new_p = (int64_t)INT32_MIN;
    }
    s->position = (int32_t)new_p;
  }
}

int32_t capy_anim_bezier_eval(int32_t p0, int32_t p1, int32_t p2, int32_t p3,
                              int32_t t_q16) {
  /* De Casteljau in Q16.16 t-space. Values stay in the input units; only
   * `t` carries the fractional fixed-point. Intermediate lerps use int64_t
   * to avoid overflow when p1-p0 multiplied by t_q16. */
  int64_t t;
  int64_t a, b, c, ab, bc, abc;
  if (t_q16 < 0) {
    t_q16 = 0;
  }
  if (t_q16 > 0x10000) {
    t_q16 = 0x10000;
  }
  t = (int64_t)t_q16;
  /* `/ 65536` instead of `>> 16` to stay strictly-defined for negative
   * intermediates (`p_i+1 - p_i` may be negative). */
  a = (int64_t)p0 + ((((int64_t)p1 - (int64_t)p0) * t) / 65536);
  b = (int64_t)p1 + ((((int64_t)p2 - (int64_t)p1) * t) / 65536);
  c = (int64_t)p2 + ((((int64_t)p3 - (int64_t)p2) * t) / 65536);
  ab = a + (((b - a) * t) / 65536);
  bc = b + (((c - b) * t) / 65536);
  abc = ab + (((bc - ab) * t) / 65536);
  if (abc > (int64_t)INT32_MAX) {
    abc = (int64_t)INT32_MAX;
  } else if (abc < (int64_t)INT32_MIN) {
    abc = (int64_t)INT32_MIN;
  }
  return (int32_t)abc;
}

/* ── v1.4: gesture engine (single-touch recognizer) ───────────────────── */

static int32_t capy_gesture_abs32(int32_t v) { return v < 0 ? -v : v; }

static uint32_t capy_gesture_chebyshev(const struct capy_ui_point *a,
                                       const struct capy_ui_point *b) {
  int32_t dx = capy_gesture_abs32(a->x - b->x);
  int32_t dy = capy_gesture_abs32(a->y - b->y);
  return (uint32_t)(dx > dy ? dx : dy);
}

static void capy_gesture_queue(struct capy_gesture_recognizer *r,
                               uint8_t kind, const struct capy_ui_point *start,
                               const struct capy_ui_point *end,
                               int32_t magnitude, uint32_t duration_ms) {
  /* Depth-1 queue: a new emit overwrites any pending gesture that the
   * host failed to poll. Determinism is preserved because the overwrite
   * is purely a function of the input sequence. */
  r->pending.kind = kind;
  r->pending.reserved[0] = 0u;
  r->pending.reserved[1] = 0u;
  r->pending.reserved[2] = 0u;
  r->pending.start = *start;
  r->pending.end = *end;
  r->pending.magnitude = magnitude;
  r->pending.duration_ms = duration_ms;
  r->has_pending = 1u;
}

/* Multi-touch (since 2.22): evaluate pinch/rotate from the two tracked finger
 * positions and queue at most one gesture (one-shot per session). Integer-only:
 * pinch uses the signed Chebyshev-separation delta vs the baseline; rotate uses
 * the sign of the integer cross product of the baseline vs current finger
 * vector, gated by a scale-independent significance test
 * (|cross| / dot > tan(~15 deg) == 27/100; dot <= 0 means >= 90 deg). */
static void capy_gesture_eval_multi(struct capy_gesture_recognizer *r) {
  int32_t cur_dist =
      (int32_t)capy_gesture_chebyshev(&r->last_pos, &r->touch2_pos);
  if (!r->pinch_emitted) {
    int32_t delta = cur_dist - r->multi_start_dist;
    int32_t adelta = delta < 0 ? -delta : delta;
    if ((uint32_t)adelta >= r->pinch_min_distance_px) {
      uint8_t kind = (delta > 0) ? (uint8_t)CAPY_GESTURE_PINCH_OUT
                                 : (uint8_t)CAPY_GESTURE_PINCH_IN;
      capy_gesture_queue(r, kind, &r->last_pos, &r->touch2_pos, delta, 0u);
      r->pinch_emitted = 1u;
      return; /* one gesture per move; rotate gets a later move */
    }
  }
  if (!r->rotate_emitted) {
    int32_t vx = r->last_pos.x - r->touch2_pos.x;
    int32_t vy = r->last_pos.y - r->touch2_pos.y;
    int64_t cross = (int64_t)r->multi_v0.x * vy - (int64_t)r->multi_v0.y * vx;
    int64_t dot = (int64_t)r->multi_v0.x * vx + (int64_t)r->multi_v0.y * vy;
    int64_t acrossv = (cross < 0) ? -cross : cross;
    if (cross != 0 && (dot <= 0 || acrossv * 100 > dot * 27)) {
      uint8_t kind = (cross > 0) ? (uint8_t)CAPY_GESTURE_ROTATE_CW
                                 : (uint8_t)CAPY_GESTURE_ROTATE_CCW;
      capy_gesture_queue(r, kind, &r->last_pos, &r->touch2_pos, 0, 0u);
      r->rotate_emitted = 1u;
    }
  }
}

void capy_gesture_recognizer_init(struct capy_gesture_recognizer *r) {
  if (!r) {
    return;
  }
  r->tap_max_duration_ms = 200u;
  r->tap_max_distance_px = 10u;
  r->double_tap_max_gap_ms = 300u;
  r->long_press_min_ms = 500u;
  r->swipe_min_distance_px = 50u;
  r->active = 0u;
  r->long_press_emitted = 0u;
  r->drag_emitted = 0u;
  r->has_pending = 0u;
  r->has_last_tap = 0u;
  r->reserved[0] = 0u;
  r->reserved[1] = 0u;
  r->reserved[2] = 0u;
  r->touch_id = 0u;
  r->start_pos.x = 0;
  r->start_pos.y = 0;
  r->last_pos.x = 0;
  r->last_pos.y = 0;
  r->start_tick = 0u;
  r->last_tap_end_tick = 0u;
  r->last_tap_pos.x = 0;
  r->last_tap_pos.y = 0;
  r->pending.kind = (uint8_t)CAPY_GESTURE_NONE;
  r->pending.reserved[0] = 0u;
  r->pending.reserved[1] = 0u;
  r->pending.reserved[2] = 0u;
  r->pending.start.x = 0;
  r->pending.start.y = 0;
  r->pending.end.x = 0;
  r->pending.end.y = 0;
  r->pending.magnitude = 0;
  r->pending.duration_ms = 0u;
  /* Multi-touch state (since 2.22). */
  r->pinch_min_distance_px = 20u;
  r->touch2_active = 0u;
  r->pinch_emitted = 0u;
  r->rotate_emitted = 0u;
  r->reserved2 = 0u;
  r->touch2_id = 0u;
  r->touch2_pos.x = 0;
  r->touch2_pos.y = 0;
  r->multi_v0.x = 0;
  r->multi_v0.y = 0;
  r->multi_start_dist = 0;
}

int capy_gesture_feed(struct capy_gesture_recognizer *r,
                      const struct capy_widget_event *ev, uint32_t now) {
  const struct capy_touch_payload *t;
  uint32_t emitted_before;
  if (!r || !ev) {
    return -1;
  }
  emitted_before = r->has_pending;
  switch (ev->type) {
    case CAPY_WIDGET_EVENT_TOUCH_BEGIN:
      t = (const struct capy_touch_payload *)ev->payload;
      if (!t) {
        return 0;
      }
      if (r->active) {
        /* A second distinct finger opens a two-finger session (since 2.22):
         * capture the baseline separation (Chebyshev) and vector (touch1 -
         * touch2) for pinch/rotate detection. A third concurrent finger, or a
         * repeat of an already-tracked id, is ignored. */
        if (!r->touch2_active && t->touch_id != r->touch_id) {
          r->touch2_active = 1u;
          r->touch2_id = t->touch_id;
          r->touch2_pos = t->pos;
          r->multi_v0.x = r->last_pos.x - t->pos.x;
          r->multi_v0.y = r->last_pos.y - t->pos.y;
          r->multi_start_dist =
              (int32_t)capy_gesture_chebyshev(&r->last_pos, &t->pos);
          r->pinch_emitted = 0u;
          r->rotate_emitted = 0u;
        }
        return 0;
      }
      r->active = 1u;
      r->long_press_emitted = 0u;
      r->drag_emitted = 0u;
      r->touch_id = t->touch_id;
      r->start_pos = t->pos;
      r->last_pos = t->pos;
      r->start_tick = now;
      return 0;
    case CAPY_WIDGET_EVENT_TOUCH_MOVE:
      t = (const struct capy_touch_payload *)ev->payload;
      if (!t) {
        return 0;
      }
      if (r->touch2_active) {
        /* Two-finger session (since 2.22): update whichever finger moved, then
         * re-evaluate pinch/rotate. Unknown ids are ignored. The single-touch
         * tap/swipe/drag path below is suppressed while a session is active. */
        if (t->touch_id == r->touch_id) {
          r->last_pos = t->pos;
        } else if (t->touch_id == r->touch2_id) {
          r->touch2_pos = t->pos;
        } else {
          return 0;
        }
        capy_gesture_eval_multi(r);
        return (r->has_pending && !emitted_before) ? 1 : 0;
      }
      if (!r->active || t->touch_id != r->touch_id) {
        return 0;
      }
      r->last_pos = t->pos;
      if (!r->drag_emitted && !r->long_press_emitted) {
        uint32_t dist = capy_gesture_chebyshev(&t->pos, &r->start_pos);
        if (dist >= r->swipe_min_distance_px) {
          int32_t dx = t->pos.x - r->start_pos.x;
          int32_t dy = t->pos.y - r->start_pos.y;
          int32_t mag = (capy_gesture_abs32(dx) > capy_gesture_abs32(dy))
                            ? capy_gesture_abs32(dx)
                            : capy_gesture_abs32(dy);
          capy_gesture_queue(r, (uint8_t)CAPY_GESTURE_DRAG, &r->start_pos,
                             &t->pos, mag, now - r->start_tick);
          r->drag_emitted = 1u;
        }
      }
      return (r->has_pending && !emitted_before) ? 1 : 0;
    case CAPY_WIDGET_EVENT_TOUCH_END:
      t = (const struct capy_touch_payload *)ev->payload;
      if (r->touch2_active) {
        /* Either finger lifting ends the two-finger session (since 2.22):
         * full reset, no emit. The still-down finger is ignored until it also
         * lifts (its later events find an inactive recognizer). An unrelated
         * id is ignored. */
        if (t && t->touch_id != r->touch_id && t->touch_id != r->touch2_id) {
          return 0;
        }
        r->active = 0u;
        r->touch2_active = 0u;
        r->long_press_emitted = 0u;
        r->drag_emitted = 0u;
        r->pinch_emitted = 0u;
        r->rotate_emitted = 0u;
        r->touch_id = 0u;
        r->touch2_id = 0u;
        r->has_last_tap = 0u; /* a two-finger session never seeds a tap chain */
        return 0;
      }
      if (!r->active) {
        return 0;
      }
      if (t && t->touch_id != r->touch_id) {
        return 0;
      }
      if (t) {
        r->last_pos = t->pos;
      }
      {
        uint32_t duration = now - r->start_tick;
        uint32_t dist = capy_gesture_chebyshev(&r->last_pos, &r->start_pos);
        int32_t dx = r->last_pos.x - r->start_pos.x;
        int32_t dy = r->last_pos.y - r->start_pos.y;
        if (r->drag_emitted) {
          /* Drag already emitted; release just clears state. */
        } else if (r->long_press_emitted) {
          /* Long-press already emitted; release clears state. */
        } else if (duration <= r->tap_max_duration_ms &&
                   dist <= r->tap_max_distance_px) {
          /* TAP candidate. Promote to DOUBLE_TAP if within gap. */
          uint8_t is_double = 0u;
          if (r->has_last_tap) {
            uint32_t gap = r->start_tick - r->last_tap_end_tick;
            uint32_t gap_dist =
                capy_gesture_chebyshev(&r->start_pos, &r->last_tap_pos);
            if (gap <= r->double_tap_max_gap_ms &&
                gap_dist <= r->tap_max_distance_px) {
              is_double = 1u;
            }
          }
          if (is_double) {
            capy_gesture_queue(r, (uint8_t)CAPY_GESTURE_DOUBLE_TAP,
                               &r->start_pos, &r->last_pos, 0, duration);
            r->has_last_tap = 0u;
          } else {
            capy_gesture_queue(r, (uint8_t)CAPY_GESTURE_TAP, &r->start_pos,
                               &r->last_pos, 0, duration);
            r->has_last_tap = 1u;
            r->last_tap_end_tick = now;
            r->last_tap_pos = r->last_pos;
          }
        } else if (dist >= r->swipe_min_distance_px) {
          /* SWIPE — dominant axis decides direction. */
          int32_t adx = capy_gesture_abs32(dx);
          int32_t ady = capy_gesture_abs32(dy);
          uint8_t kind;
          int32_t mag;
          if (adx >= ady) {
            kind = (dx >= 0) ? (uint8_t)CAPY_GESTURE_SWIPE_RIGHT
                             : (uint8_t)CAPY_GESTURE_SWIPE_LEFT;
            mag = adx;
          } else {
            kind = (dy >= 0) ? (uint8_t)CAPY_GESTURE_SWIPE_DOWN
                             : (uint8_t)CAPY_GESTURE_SWIPE_UP;
            mag = ady;
          }
          capy_gesture_queue(r, kind, &r->start_pos, &r->last_pos, mag,
                             duration);
          /* Swipe invalidates any pending double-tap chain. */
          r->has_last_tap = 0u;
        } else {
          /* Held too long for tap but moved too little for swipe: no emit.
           * Drop the double-tap chain to avoid stale state. */
          r->has_last_tap = 0u;
        }
      }
      r->active = 0u;
      r->long_press_emitted = 0u;
      r->drag_emitted = 0u;
      r->touch_id = 0u;
      return (r->has_pending && !emitted_before) ? 1 : 0;
    default:
      /* POINTER/KEY/WHEEL/GAMEPAD and any future event types are ignored. */
      return 0;
  }
}

int capy_gesture_tick(struct capy_gesture_recognizer *r, uint32_t now) {
  uint32_t elapsed;
  uint32_t dist;
  if (!r) {
    return -1;
  }
  if (!r->active || r->long_press_emitted || r->drag_emitted) {
    return 0;
  }
  elapsed = now - r->start_tick;
  if (elapsed < r->long_press_min_ms) {
    return 0;
  }
  dist = capy_gesture_chebyshev(&r->last_pos, &r->start_pos);
  if (dist > r->tap_max_distance_px) {
    return 0;
  }
  capy_gesture_queue(r, (uint8_t)CAPY_GESTURE_LONG_PRESS, &r->start_pos,
                     &r->last_pos, 0, elapsed);
  r->long_press_emitted = 1u;
  /* A long-press resets the double-tap chain. */
  r->has_last_tap = 0u;
  return 1;
}

int capy_gesture_poll(struct capy_gesture_recognizer *r,
                      struct capy_gesture *out) {
  if (!r || !out) {
    return -1;
  }
  if (!r->has_pending) {
    return 0;
  }
  *out = r->pending;
  r->has_pending = 0u;
  return 1;
}

void capy_widget_invalidate(struct capy_widget *widget) {
  while (widget) {
    widget->layout_dirty = 1u;
    /* Since 1.1: also bump the monotonic dirty counter so external observers
     * can detect that an ancestor was touched even when measure decides not
     * to recompute bounds. Overflow wraps silently — hosts observe the
     * *change* relative to a previous sample, not the absolute value. */
    ++widget->dirty_version;
    widget = widget->parent;
  }
}

/* Since 1.1: subtree-walking sibling of capy_widget_invalidate. */
static void capy_widget_invalidate_subtree_recurse(struct capy_widget *w) {
  uint32_t i;
  if (!w) {
    return;
  }
  w->layout_dirty = 1u;
  ++w->dirty_version;
  for (i = 0u; i < w->child_count; ++i) {
    capy_widget_invalidate_subtree_recurse(w->children[i]);
  }
}

void capy_widget_invalidate_subtree(struct capy_widget *widget) {
  capy_widget_invalidate_subtree_recurse(widget);
}

uint32_t capy_widget_dirty_version(const struct capy_widget *widget) {
  return widget ? widget->dirty_version : 0u;
}

void capy_widget_frame_budget(struct capy_widget_context *ctx,
                              uint32_t microseconds) {
  if (!ctx) {
    return;
  }
  ctx->frame_budget_microseconds = microseconds;
}

int capy_theme_deserialize(struct capy_theme *out, const char *in,
                           uint32_t len) {
  struct capy_theme tmp;
  uint32_t pos = 0u;
  int saw_version = 0;
  uint32_t format_version = 0u;
  if (!out || !in) {
    return -1;
  }
  capy_widget_zero(&tmp, sizeof(tmp));
  while (pos < len) {
    uint32_t line_start = pos;
    uint32_t line_end;
    uint32_t eq_pos = 0u;
    int has_eq = 0;
    uint32_t key_start;
    uint32_t key_end;
    uint32_t val_start;
    uint32_t val_end;
    uint32_t j;
    while (pos < len && in[pos] != '\n') {
      ++pos;
    }
    line_end = pos;
    if (pos < len) {
      ++pos; /* skip the '\n' */
    }
    /* Trim leading whitespace from the line view. */
    while (line_start < line_end &&
           (in[line_start] == ' ' || in[line_start] == '\t')) {
      ++line_start;
    }
    /* Trim trailing whitespace + optional CR. */
    while (line_end > line_start &&
           (in[line_end - 1u] == ' ' || in[line_end - 1u] == '\t' ||
            in[line_end - 1u] == '\r')) {
      --line_end;
    }
    if (line_start == line_end) {
      continue;
    }
    if (in[line_start] == '#') {
      continue;
    }
    for (j = line_start; j < line_end; ++j) {
      if (in[j] == '=') {
        eq_pos = j;
        has_eq = 1;
        break;
      }
    }
    if (!has_eq) {
      return -1;
    }
    key_start = line_start;
    key_end = eq_pos;
    while (key_end > key_start &&
           (in[key_end - 1u] == ' ' || in[key_end - 1u] == '\t')) {
      --key_end;
    }
    val_start = eq_pos + 1u;
    val_end = line_end;
    while (val_start < val_end &&
           (in[val_start] == ' ' || in[val_start] == '\t')) {
      ++val_start;
    }
    if (key_end == key_start) {
      return -1;
    }
    if (capy_theme_range_eq_literal(in, key_start, key_end, "version")) {
      if (capy_theme_parse_decimal_u32(in, val_start, val_end,
                                       &format_version) != 0) {
        return -1;
      }
      if (format_version == 0u ||
          format_version > (uint32_t)CAPY_THEME_FORMAT_VERSION) {
        return -1;
      }
      tmp.version = (uint16_t)format_version;
      saw_version = 1;
    } else if (capy_theme_range_eq_literal(in, key_start, key_end,
                                           "variant")) {
      if (capy_theme_range_eq_literal(in, val_start, val_end, "light")) {
        tmp.variant = (uint8_t)CAPY_THEME_VARIANT_LIGHT;
      } else if (capy_theme_range_eq_literal(in, val_start, val_end,
                                             "dark")) {
        tmp.variant = (uint8_t)CAPY_THEME_VARIANT_DARK;
      } else if (capy_theme_range_eq_literal(in, val_start, val_end,
                                             "high_contrast")) {
        tmp.variant = (uint8_t)CAPY_THEME_VARIANT_HIGH_CONTRAST;
      } else {
        return -1;
      }
    } else if (capy_theme_range_eq_literal(in, key_start, key_end,
                                           "high_contrast")) {
      uint8_t b = 0u;
      if (capy_theme_parse_bool(in, val_start, val_end, &b) != 0) {
        return -1;
      }
      tmp.high_contrast = b;
    } else {
      uint8_t tok = 0u;
      uint32_t value = 0u;
      if (capy_theme_token_from_range(in, key_start, key_end, &tok) == 0) {
        if (capy_theme_parse_hex32(in, val_start, val_end, &value) != 0) {
          return -1;
        }
        tmp.tokens[tok] = value;
      }
      /* Unknown key: silently ignored per contract. */
    }
  }
  if (!saw_version) {
    return -1;
  }
  *out = tmp;
  return 0;
}

/* ------------------------------------------------------------------------
 * Theme packs (since 2.4)
 * ------------------------------------------------------------------------ */

static uint32_t capy_theme_pack_fnv1a(const uint8_t *bytes, uint32_t len) {
  uint32_t h = 2166136261u;
  uint32_t i;
  for (i = 0u; i < len; ++i) {
    h ^= (uint32_t)bytes[i];
    h *= 16777619u;
  }
  return h;
}

static uint16_t capy_theme_pack_read_u16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t capy_theme_pack_read_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int capy_theme_pack_validate(const void *buf, uint32_t len) {
  const uint8_t *b = (const uint8_t *)buf;
  uint16_t format_version;
  uint16_t entry_count;
  uint16_t reserved;
  uint32_t declared_checksum;
  uint32_t expected_total;
  uint32_t actual_checksum;
  if (!b || len < CAPY_THEME_PACK_HEADER_SIZE) {
    return -1;
  }
  if (b[0] != CAPY_THEME_PACK_MAGIC0 || b[1] != CAPY_THEME_PACK_MAGIC1 ||
      b[2] != CAPY_THEME_PACK_MAGIC2 || b[3] != CAPY_THEME_PACK_MAGIC3) {
    return -1;
  }
  format_version = capy_theme_pack_read_u16(b + 4);
  if (format_version == 0u ||
      format_version > (uint16_t)CAPY_THEME_PACK_FORMAT_VERSION) {
    return -1;
  }
  declared_checksum = capy_theme_pack_read_u32(b + 8);
  entry_count = capy_theme_pack_read_u16(b + 12);
  reserved = capy_theme_pack_read_u16(b + 14);
  if (reserved != 0u) {
    return -1;
  }
  expected_total = (uint32_t)CAPY_THEME_PACK_HEADER_SIZE +
                   (uint32_t)entry_count * (uint32_t)CAPY_THEME_PACK_ENTRY_SIZE;
  if (expected_total != len) {
    return -1;
  }
  actual_checksum = capy_theme_pack_fnv1a(b + CAPY_THEME_PACK_HEADER_SIZE,
                                          len - CAPY_THEME_PACK_HEADER_SIZE);
  if (actual_checksum != declared_checksum) {
    return -1;
  }
  return 0;
}

int capy_theme_pack_load(const void *buf, uint32_t len,
                         struct capy_theme *out) {
  const uint8_t *b = (const uint8_t *)buf;
  uint16_t entry_count;
  uint16_t i;
  uint8_t variant;
  uint8_t high_contrast;
  if (!out) {
    return -1;
  }
  if (capy_theme_pack_validate(buf, len) != 0) {
    return -1;
  }
  variant = b[6];
  high_contrast = b[7];
  if (variant >= 16u || high_contrast > 1u) {
    return -1;
  }
  entry_count = capy_theme_pack_read_u16(b + 12);
  for (i = 0u; i < entry_count; ++i) {
    const uint8_t *e = b + CAPY_THEME_PACK_HEADER_SIZE +
                       (uint32_t)i * (uint32_t)CAPY_THEME_PACK_ENTRY_SIZE;
    uint8_t token_id = e[0];
    uint8_t entry_reserved = e[1];
    uint32_t colour;
    if (entry_reserved != 0u) {
      return -1;
    }
    colour = capy_theme_pack_read_u32(e + 2);
    if (token_id == 0u ||
        (uint32_t)token_id >= (uint32_t)CAPY_TOKEN_COUNT) {
      /* Forward-compat: unknown tokens silently skipped. */
      continue;
    }
    out->tokens[token_id] = colour;
  }
  out->variant = variant;
  out->high_contrast = high_contrast;
  return 0;
}

/* ------------------------------------------------------------------------
 * Devtools / inspector (since 2.5)
 * ------------------------------------------------------------------------ */

static uint32_t capy_inspector_flags_for(const struct capy_widget *w) {
  uint32_t f = 0u;
  if (w->visible) {
    f |= CAPY_INSPECTOR_FLAG_VISIBLE;
  }
  if (w->enabled) {
    f |= CAPY_INSPECTOR_FLAG_ENABLED;
  }
  if (w->focused) {
    f |= CAPY_INSPECTOR_FLAG_FOCUSED;
  }
  if (w->focusable) {
    f |= CAPY_INSPECTOR_FLAG_FOCUSABLE;
  }
  if (w->checked) {
    f |= CAPY_INSPECTOR_FLAG_CHECKED;
  }
  if (w->hovered) {
    f |= CAPY_INSPECTOR_FLAG_HOVERED;
  }
  if (w->layout_dirty) {
    f |= CAPY_INSPECTOR_FLAG_LAYOUT_DIRTY;
  }
  if (w->transform) {
    f |= CAPY_INSPECTOR_FLAG_HAS_TRANSFORM;
  }
  if (w->virtual_source) {
    f |= CAPY_INSPECTOR_FLAG_HAS_VIRTUAL_SRC;
  }
  if (w->drag_payload) {
    f |= CAPY_INSPECTOR_FLAG_HAS_DRAG_PAYLOAD;
  }
  if (w->on_drop) {
    f |= CAPY_INSPECTOR_FLAG_HAS_DROP_TARGET;
  }
  return f;
}

static int capy_inspector_visit(const struct capy_widget *w, uint32_t parent_idx,
                                uint16_t depth,
                                struct capy_inspector_node *out_nodes,
                                uint32_t cap, char *out_text_pool,
                                uint32_t text_cap, uint32_t *node_count,
                                uint32_t *text_used) {
  uint32_t my_idx;
  struct capy_inspector_node *n;
  uint16_t tlen;
  uint32_t i;
  if (*node_count >= cap) {
    return -1;
  }
  my_idx = *node_count;
  n = &out_nodes[my_idx];
  n->id = w->id;
  n->type = (uint16_t)w->type;
  n->depth = depth;
  n->parent_index = parent_idx;
  n->child_count = w->child_count;
  n->x = w->bounds.x;
  n->y = w->bounds.y;
  n->w = w->bounds.width;
  n->h = w->bounds.height;
  n->flags = capy_inspector_flags_for(w);
  n->tab_index = w->tab_index;
  n->font_id = w->font_id;
  n->layout_version = w->layout_version;
  n->dirty_version = w->dirty_version;
  tlen = capy_text_len(w->text, CAPY_WIDGET_MAX_TEXT);
  if (tlen > 0u) {
    if ((uint32_t)*text_used + (uint32_t)tlen > text_cap) {
      return -1;
    }
    n->text_offset = (uint16_t)*text_used;
    n->text_len = tlen;
    for (i = 0u; i < (uint32_t)tlen; ++i) {
      out_text_pool[*text_used + i] = w->text[i];
    }
    *text_used += (uint32_t)tlen;
  } else {
    n->text_offset = 0u;
    n->text_len = 0u;
  }
  *node_count += 1u;
  for (i = 0u; i < w->child_count; ++i) {
    if (!w->children[i]) {
      continue;
    }
    if (capy_inspector_visit(w->children[i], my_idx, (uint16_t)(depth + 1u),
                             out_nodes, cap, out_text_pool, text_cap,
                             node_count, text_used) != 0) {
      return -1;
    }
  }
  return 0;
}

int capy_widget_inspect(const struct capy_widget *root,
                        struct capy_inspector_node *out_nodes,
                        uint32_t cap, char *out_text_pool,
                        uint32_t text_cap, uint32_t *out_node_count,
                        uint32_t *out_text_used) {
  uint32_t node_count;
  uint32_t text_used;
  if (!root || !out_nodes || !out_node_count || !out_text_used) {
    return -1;
  }
  if (text_cap > 0u && !out_text_pool) {
    return -1;
  }
  node_count = 0u;
  text_used = 0u;
  if (capy_inspector_visit(root, CAPY_INSPECTOR_NO_PARENT, 0u, out_nodes,
                           cap, out_text_pool, text_cap, &node_count,
                           &text_used) != 0) {
    /* Roll back to pre-call state (which was 0 here). */
    *out_node_count = 0u;
    *out_text_used = 0u;
    return -1;
  }
  *out_node_count = node_count;
  *out_text_used = text_used;
  return 0;
}

static uint32_t capy_inspector_count_widgets(const struct capy_widget *w) {
  uint32_t total = 1u;
  uint32_t i;
  for (i = 0u; i < w->child_count; ++i) {
    if (w->children[i]) {
      total += capy_inspector_count_widgets(w->children[i]);
    }
  }
  return total;
}

void capy_widget_set_display_controller(
    struct capy_widget_context *ctx,
    struct capy_display_controller *controller) {
  if (!ctx) {
    return;
  }
  ctx->display_controller = controller;
}

int capy_display_enum_modes(struct capy_widget_context *ctx,
                            struct capy_display_mode *out, uint32_t cap) {
  struct capy_display_controller *c;
  if (!ctx) {
    return -1;
  }
  c = ctx->display_controller;
  if (!c || !c->enum_modes) {
    return -1;
  }
  if (cap > 0u && !out) {
    return -1;
  }
  return c->enum_modes(c->user_data, out, cap);
}

int capy_display_set_mode(struct capy_widget_context *ctx,
                          struct capy_widget *root,
                          const struct capy_display_mode *mode) {
  struct capy_display_controller *c;
  int rc;
  if (!ctx || !mode) {
    return -1;
  }
  if (mode->width == 0u || mode->height == 0u) {
    return -1;
  }
  c = ctx->display_controller;
  if (!c || !c->set_mode) {
    return -1;
  }
  rc = c->set_mode(c->user_data, mode);
  if (rc != 0) {
    return -1;
  }
  c->current_mode = *mode;
  c->has_current = 1u;
  if (root) {
    struct capy_widget_event ev;
    capy_widget_zero(&ev, sizeof(ev));
    ev.type = CAPY_WIDGET_EVENT_DISPLAY_MODE_CHANGED;
    ev.x = (int32_t)(((uint32_t)mode->width << 16) | (uint32_t)mode->height);
    ev.y = (int32_t)(((uint32_t)mode->bpp << 16) |
                     (uint32_t)mode->refresh_hz_q8);
    (void)capy_widget_handle_event(root, &ev);
  }
  return 0;
}

int capy_display_current_mode(const struct capy_widget_context *ctx,
                              struct capy_display_mode *out) {
  const struct capy_display_controller *c;
  if (!ctx || !out) {
    return -1;
  }
  c = ctx->display_controller;
  if (!c || !c->has_current) {
    return -1;
  }
  *out = c->current_mode;
  return 0;
}

void capy_widget_set_user_directory(struct capy_widget_context *ctx,
                                    struct capy_user_directory *directory) {
  if (!ctx) {
    return;
  }
  ctx->user_directory = directory;
}

int capy_user_list(struct capy_widget_context *ctx,
                   struct capy_user_account *out, uint32_t cap) {
  struct capy_user_directory *d;
  if (!ctx) {
    return -1;
  }
  d = ctx->user_directory;
  if (!d || !d->list) {
    return -1;
  }
  if (cap > 0u && !out) {
    return -1;
  }
  return d->list(d->user_data, out, cap);
}

int capy_user_create(struct capy_widget_context *ctx,
                     const struct capy_user_account *account,
                     const char *password) {
  struct capy_user_directory *d;
  if (!ctx || !account) {
    return -1;
  }
  if (account->username[0] == '\0') {
    return -1;
  }
  d = ctx->user_directory;
  if (!d || !d->create) {
    return -1;
  }
  return d->create(d->user_data, account, password);
}

int capy_user_update(struct capy_widget_context *ctx,
                     const struct capy_user_account *account) {
  struct capy_user_directory *d;
  if (!ctx || !account) {
    return -1;
  }
  d = ctx->user_directory;
  if (!d || !d->update) {
    return -1;
  }
  return d->update(d->user_data, account);
}

int capy_user_delete(struct capy_widget_context *ctx, uint32_t uid) {
  struct capy_user_directory *d;
  if (!ctx) {
    return -1;
  }
  if (uid == 0u) {
    return -1;
  }
  d = ctx->user_directory;
  if (!d || !d->del) {
    return -1;
  }
  return d->del(d->user_data, uid);
}

int capy_user_set_avatar(struct capy_widget_context *ctx, uint32_t uid,
                         const void *png, uint32_t len) {
  struct capy_user_directory *d;
  if (!ctx) {
    return -1;
  }
  if (len > 0u && !png) {
    return -1;
  }
  d = ctx->user_directory;
  if (!d || !d->set_avatar) {
    return -1;
  }
  return d->set_avatar(d->user_data, uid, png, len);
}

/* ------------------------------------------------------------------------
 * Contrast & accessibility (since 2.8)
 * ------------------------------------------------------------------------ */

/* sRGB byte → Q16.16 linear luminance with gamma ≈ 2.0. Zero-float.
 * Range: c=0 → 0, c=255 → 65536. Monotonic. */
static uint32_t capy_contrast_srgb_linear_q16(uint8_t c) {
  uint32_t cu = (uint32_t)c;
  return (cu * cu * 65536u) / 65025u; /* 65025 = 255 * 255 */
}

/* Q16.16 relative luminance. WCAG-2.1-style weights × 10000. */
static uint32_t capy_contrast_luminance_q16(uint32_t colour) {
  uint32_t r = (colour >> 16) & 0xFFu;
  uint32_t g = (colour >> 8) & 0xFFu;
  uint32_t b = colour & 0xFFu;
  uint32_t rl = capy_contrast_srgb_linear_q16((uint8_t)r);
  uint32_t gl = capy_contrast_srgb_linear_q16((uint8_t)g);
  uint32_t bl = capy_contrast_srgb_linear_q16((uint8_t)b);
  uint64_t weighted = (uint64_t)2126u * rl + (uint64_t)7152u * gl +
                      (uint64_t)722u * bl;
  return (uint32_t)(weighted / 10000u);
}

uint32_t capy_theme_contrast_ratio_x1000(uint32_t fg, uint32_t bg) {
  uint32_t lf = capy_contrast_luminance_q16(fg);
  uint32_t lb = capy_contrast_luminance_q16(bg);
  uint32_t lmax = lf >= lb ? lf : lb;
  uint32_t lmin = lf >= lb ? lb : lf;
  /* WCAG offset 0.05 in Q16.16. Use the floor so white/black clamps to
   * the canonical 21.000 ratio despite integer arithmetic. */
  uint64_t numerator = ((uint64_t)lmax + 3276u) * 1000u;
  uint64_t denominator = (uint64_t)lmin + 3276u;
  uint64_t ratio = numerator / denominator;
  if (ratio < 1000u) {
    ratio = 1000u;
  }
  if (ratio > 21000u) {
    ratio = 21000u;
  }
  return (uint32_t)ratio;
}

struct capy_contrast_pair {
  uint8_t fg;
  uint8_t bg;
};

static const struct capy_contrast_pair capy_contrast_audit_pairs[] = {
    {CAPY_TOKEN_FG_PRIMARY, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_FG_PRIMARY, CAPY_TOKEN_BG_RAISED},
    {CAPY_TOKEN_FG_MUTED, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_FG_INVERSE, CAPY_TOKEN_ACCENT},
    {CAPY_TOKEN_BORDER_FOCUS, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_FOCUS_RING, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_DANGER, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_WARNING, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_SUCCESS, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_INFO, CAPY_TOKEN_BG_BASE},
    {CAPY_TOKEN_DISABLED, CAPY_TOKEN_BG_BASE}
};

#define CAPY_CONTRAST_PAIR_COUNT \
  ((uint32_t)(sizeof(capy_contrast_audit_pairs) / \
              sizeof(capy_contrast_audit_pairs[0])))

int capy_theme_audit_wcag(const struct capy_theme *theme,
                          struct capy_contrast_finding *out, uint32_t cap) {
  uint32_t i;
  uint32_t written;
  if (!theme) {
    return -1;
  }
  if (cap == 0u) {
    return (int)CAPY_CONTRAST_PAIR_COUNT;
  }
  if (!out) {
    return -1;
  }
  written = 0u;
  for (i = 0u; i < CAPY_CONTRAST_PAIR_COUNT; ++i) {
    uint8_t fg_tok = capy_contrast_audit_pairs[i].fg;
    uint8_t bg_tok = capy_contrast_audit_pairs[i].bg;
    uint32_t fg = theme->tokens[fg_tok];
    uint32_t bg = theme->tokens[bg_tok];
    uint32_t ratio;
    struct capy_contrast_finding *f;
    if (written >= cap) {
      break;
    }
    ratio = capy_theme_contrast_ratio_x1000(fg, bg);
    f = &out[written];
    f->fg_token = fg_tok;
    f->bg_token = bg_tok;
    f->ratio_x1000 = (uint16_t)ratio;
    f->passes_aa = (ratio >= CAPY_CONTRAST_AA_X1000) ? 1u : 0u;
    f->passes_aaa = (ratio >= CAPY_CONTRAST_AAA_X1000) ? 1u : 0u;
    f->reserved[0] = 0u;
    f->reserved[1] = 0u;
    ++written;
  }
  return (int)written;
}

struct capy_theme capy_theme_default_dark_high_contrast(void) {
  struct capy_theme t;
  capy_widget_zero(&t, sizeof(t));
  t.tokens[CAPY_TOKEN_BG_BASE]     = 0xFF000000u;
  t.tokens[CAPY_TOKEN_BG_RAISED]   = 0xFF000000u;
  t.tokens[CAPY_TOKEN_BG_SUNKEN]   = 0xFF000000u;
  t.tokens[CAPY_TOKEN_FG_PRIMARY]  = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_FG_MUTED]    = 0xFFC0C0C0u;
  t.tokens[CAPY_TOKEN_FG_INVERSE]  = 0xFF000000u;
  t.tokens[CAPY_TOKEN_ACCENT]      = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_ACCENT_HOVER]= 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_BORDER]      = 0xFFFFFFFFu;
  t.tokens[CAPY_TOKEN_BORDER_FOCUS]= 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_FOCUS_RING]  = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_DANGER]      = 0xFFFF8080u;
  t.tokens[CAPY_TOKEN_WARNING]     = 0xFFFFFF00u;
  t.tokens[CAPY_TOKEN_SUCCESS]     = 0xFF80FF80u;
  t.tokens[CAPY_TOKEN_INFO]        = 0xFF80FFFFu;
  t.tokens[CAPY_TOKEN_DISABLED]    = 0xFFA0A0A0u;
  t.variant = (uint8_t)CAPY_THEME_VARIANT_DARK_HIGH_CONTRAST;
  t.high_contrast = 1u;
  t.version = (uint16_t)CAPY_THEME_FORMAT_VERSION;
  return t;
}

/* ------------------------------------------------------------------------
 * Desktop icon arrangement (since 2.9)
 * ------------------------------------------------------------------------ */

static uint32_t capy_desktop_layout_fnv1a(const uint8_t *bytes, uint32_t len) {
  uint32_t h = 2166136261u;
  uint32_t i;
  for (i = 0u; i < len; ++i) {
    h ^= (uint32_t)bytes[i];
    h *= 16777619u;
  }
  return h;
}

static uint16_t capy_desktop_layout_read_u16(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int16_t capy_desktop_layout_read_i16(const uint8_t *p) {
  return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t capy_desktop_layout_read_u32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void capy_desktop_layout_write_u16(uint8_t *p, uint16_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void capy_desktop_layout_write_i16(uint8_t *p, int16_t v) {
  uint16_t u = (uint16_t)v;
  p[0] = (uint8_t)(u & 0xFFu);
  p[1] = (uint8_t)((u >> 8) & 0xFFu);
}

static void capy_desktop_layout_write_u32(uint8_t *p, uint32_t v) {
  p[0] = (uint8_t)(v & 0xFFu);
  p[1] = (uint8_t)((v >> 8) & 0xFFu);
  p[2] = (uint8_t)((v >> 16) & 0xFFu);
  p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

int capy_desktop_layout_validate(const void *buf, uint32_t len) {
  const uint8_t *b = (const uint8_t *)buf;
  uint16_t format_version;
  uint16_t cell_w;
  uint16_t cell_h;
  uint16_t entry_count;
  uint8_t snap;
  uint8_t auto_arrange;
  uint8_t sort_by;
  uint32_t declared_checksum;
  uint32_t actual_checksum;
  uint32_t reserved;
  uint32_t expected_total;
  if (!b || len < CAPY_DESKTOP_LAYOUT_HEADER_SIZE) {
    return -1;
  }
  if (b[0] != CAPY_DESKTOP_LAYOUT_MAGIC0 ||
      b[1] != CAPY_DESKTOP_LAYOUT_MAGIC1 ||
      b[2] != CAPY_DESKTOP_LAYOUT_MAGIC2 ||
      b[3] != CAPY_DESKTOP_LAYOUT_MAGIC3) {
    return -1;
  }
  format_version = capy_desktop_layout_read_u16(b + 4);
  if (format_version == 0u ||
      format_version > (uint16_t)CAPY_DESKTOP_LAYOUT_FORMAT_VERSION) {
    return -1;
  }
  snap = b[6];
  auto_arrange = b[7];
  if (snap > 1u || auto_arrange > 1u) {
    return -1;
  }
  declared_checksum = capy_desktop_layout_read_u32(b + 8);
  cell_w = capy_desktop_layout_read_u16(b + 12);
  cell_h = capy_desktop_layout_read_u16(b + 14);
  if (cell_w == 0u || cell_h == 0u) {
    return -1;
  }
  sort_by = b[16];
  if ((uint32_t)sort_by >= (uint32_t)CAPY_DESKTOP_SORT_COUNT) {
    return -1;
  }
  entry_count = capy_desktop_layout_read_u16(b + 18);
  reserved = capy_desktop_layout_read_u32(b + 20);
  if (reserved != 0u) {
    return -1;
  }
  expected_total = (uint32_t)CAPY_DESKTOP_LAYOUT_HEADER_SIZE +
                   (uint32_t)entry_count *
                       (uint32_t)CAPY_DESKTOP_LAYOUT_ENTRY_SIZE;
  if (expected_total != len) {
    return -1;
  }
  actual_checksum =
      capy_desktop_layout_fnv1a(b + CAPY_DESKTOP_LAYOUT_HEADER_SIZE,
                                len - CAPY_DESKTOP_LAYOUT_HEADER_SIZE);
  if (actual_checksum != declared_checksum) {
    return -1;
  }
  return 0;
}

int capy_desktop_layout_load(const void *buf, uint32_t len,
                             struct capy_desktop_layout *out_layout,
                             struct capy_desktop_icon_position *out_positions,
                             uint32_t cap, uint32_t *out_count) {
  const uint8_t *b = (const uint8_t *)buf;
  uint16_t entry_count;
  uint16_t i;
  uint16_t to_copy;
  if (!out_layout || !out_count) {
    return -1;
  }
  if (cap > 0u && !out_positions) {
    return -1;
  }
  if (capy_desktop_layout_validate(buf, len) != 0) {
    return -1;
  }
  out_layout->cell_w = capy_desktop_layout_read_u16(b + 12);
  out_layout->cell_h = capy_desktop_layout_read_u16(b + 14);
  out_layout->snap = b[6];
  out_layout->auto_arrange = b[7];
  out_layout->sort_by = b[16];
  out_layout->flags = b[17];
  entry_count = capy_desktop_layout_read_u16(b + 18);
  *out_count = (uint32_t)entry_count;
  to_copy = (uint16_t)((uint32_t)entry_count < cap ? (uint32_t)entry_count
                                                  : cap);
  for (i = 0u; i < to_copy; ++i) {
    const uint8_t *e = b + CAPY_DESKTOP_LAYOUT_HEADER_SIZE +
                       (uint32_t)i * (uint32_t)CAPY_DESKTOP_LAYOUT_ENTRY_SIZE;
    struct capy_desktop_icon_position *p = &out_positions[i];
    uint8_t entry_reserved;
    p->icon_id = capy_desktop_layout_read_u32(e);
    p->x = capy_desktop_layout_read_i16(e + 4);
    p->y = capy_desktop_layout_read_i16(e + 6);
    p->grid_x = capy_desktop_layout_read_i16(e + 8);
    p->grid_y = capy_desktop_layout_read_i16(e + 10);
    p->pinned = e[12];
    entry_reserved = (uint8_t)(e[13] | e[14] | e[15]);
    if (entry_reserved != 0u || p->pinned > 1u) {
      return -1;
    }
    p->reserved[0] = 0u;
    p->reserved[1] = 0u;
    p->reserved[2] = 0u;
  }
  return 0;
}

int capy_desktop_layout_serialize(
    const struct capy_desktop_layout *layout,
    const struct capy_desktop_icon_position *positions, uint32_t count,
    void *out, uint32_t cap) {
  uint8_t *b = (uint8_t *)out;
  uint32_t total;
  uint32_t i;
  uint32_t checksum;
  if (!layout || !out) {
    return -1;
  }
  if (count > 0u && !positions) {
    return -1;
  }
  if (count > 0xFFFFu) {
    return -1;
  }
  if (layout->cell_w == 0u || layout->cell_h == 0u) {
    return -1;
  }
  if (layout->snap > 1u || layout->auto_arrange > 1u) {
    return -1;
  }
  if ((uint32_t)layout->sort_by >= (uint32_t)CAPY_DESKTOP_SORT_COUNT) {
    return -1;
  }
  total = (uint32_t)CAPY_DESKTOP_LAYOUT_HEADER_SIZE +
          count * (uint32_t)CAPY_DESKTOP_LAYOUT_ENTRY_SIZE;
  if (total > cap) {
    return -1;
  }
  b[0] = CAPY_DESKTOP_LAYOUT_MAGIC0;
  b[1] = CAPY_DESKTOP_LAYOUT_MAGIC1;
  b[2] = CAPY_DESKTOP_LAYOUT_MAGIC2;
  b[3] = CAPY_DESKTOP_LAYOUT_MAGIC3;
  capy_desktop_layout_write_u16(b + 4,
                                (uint16_t)CAPY_DESKTOP_LAYOUT_FORMAT_VERSION);
  b[6] = layout->snap;
  b[7] = layout->auto_arrange;
  /* checksum placeholder */
  capy_desktop_layout_write_u32(b + 8, 0u);
  capy_desktop_layout_write_u16(b + 12, layout->cell_w);
  capy_desktop_layout_write_u16(b + 14, layout->cell_h);
  b[16] = layout->sort_by;
  b[17] = layout->flags;
  capy_desktop_layout_write_u16(b + 18, (uint16_t)count);
  capy_desktop_layout_write_u32(b + 20, 0u);
  for (i = 0u; i < count; ++i) {
    uint8_t *e = b + CAPY_DESKTOP_LAYOUT_HEADER_SIZE +
                 i * (uint32_t)CAPY_DESKTOP_LAYOUT_ENTRY_SIZE;
    const struct capy_desktop_icon_position *p = &positions[i];
    if (p->pinned > 1u) {
      return -1;
    }
    capy_desktop_layout_write_u32(e, p->icon_id);
    capy_desktop_layout_write_i16(e + 4, p->x);
    capy_desktop_layout_write_i16(e + 6, p->y);
    capy_desktop_layout_write_i16(e + 8, p->grid_x);
    capy_desktop_layout_write_i16(e + 10, p->grid_y);
    e[12] = p->pinned;
    e[13] = 0u;
    e[14] = 0u;
    e[15] = 0u;
  }
  checksum = capy_desktop_layout_fnv1a(b + CAPY_DESKTOP_LAYOUT_HEADER_SIZE,
                                       total - CAPY_DESKTOP_LAYOUT_HEADER_SIZE);
  capy_desktop_layout_write_u32(b + 8, checksum);
  return (int)total;
}

/* ------------------------------------------------------------------------
 * File manager UX plumbing (since 2.10)
 * ------------------------------------------------------------------------ */

int capy_breadcrumb_truncate(const struct capy_breadcrumb_segment *in,
                             uint32_t in_count, uint32_t available_width_px,
                             uint32_t segment_avg_px,
                             struct capy_breadcrumb_segment *out,
                             uint32_t cap, uint32_t *out_count,
                             uint8_t *out_dropdown) {
  uint32_t max_inline;
  uint32_t inline_count;
  uint32_t i;
  if (!out_count || !out_dropdown) {
    return -1;
  }
  if (in_count == 0u) {
    *out_count = 0u;
    *out_dropdown = 0u;
    return 0;
  }
  if (!in || !out) {
    return -1;
  }
  if (segment_avg_px == 0u) {
    return -1;
  }
  if (cap == 0u) {
    return -1;
  }
  /* Full fit: all segments inline, no dropdown. */
  if ((uint64_t)in_count * (uint64_t)segment_avg_px <=
      (uint64_t)available_width_px) {
    if (in_count > cap) {
      return -1;
    }
    for (i = 0u; i < in_count; ++i) {
      out[i] = in[i];
    }
    *out_count = in_count;
    *out_dropdown = 0u;
    return 0;
  }
  /* Truncated: root + tail with `…` dropdown between. */
  max_inline = available_width_px / segment_avg_px;
  if (max_inline < 2u) {
    max_inline = 2u;
  }
  if (max_inline > in_count) {
    max_inline = in_count;
  }
  if (max_inline > cap) {
    max_inline = cap;
  }
  if (max_inline < 2u) {
    return -1;
  }
  inline_count = max_inline;
  out[0] = in[0];
  /* Fill the remaining slots from the tail of `in[]`. */
  for (i = 1u; i < inline_count; ++i) {
    uint32_t src_index = in_count - (inline_count - i);
    out[i] = in[src_index];
  }
  *out_count = inline_count;
  *out_dropdown = 1u;
  return 0;
}

/* ------------------------------------------------------------------------
 * Icon & thumbnail system (since 2.11)
 * ------------------------------------------------------------------------ */

void capy_widget_set_icon_provider(struct capy_widget_context *ctx,
                                   struct capy_icon_provider *provider) {
  if (!ctx) {
    return;
  }
  ctx->icon_provider = provider;
}

int capy_icon_resolve(struct capy_widget_context *ctx, const char *mime_type,
                      const char *extension, uint32_t *out_icon_id) {
  struct capy_icon_provider *p;
  if (!ctx || !out_icon_id) {
    return -1;
  }
  *out_icon_id = 0u;
  p = ctx->icon_provider;
  if (!p || !p->resolve) {
    return 0;
  }
  if (p->resolve(p->user_data, mime_type, extension, out_icon_id) != 0) {
    *out_icon_id = 0u;
  }
  return 0;
}

int capy_icon_thumbnail_request(struct capy_widget_context *ctx,
                                const char *path, uint32_t *out_request_id) {
  struct capy_icon_provider *p;
  int rc;
  if (!ctx || !path || !out_request_id) {
    return -1;
  }
  *out_request_id = 0u;
  p = ctx->icon_provider;
  if (!p || !p->thumbnail_request) {
    return -1;
  }
  rc = p->thumbnail_request(p->user_data, path, out_request_id);
  if (rc != 0 || *out_request_id == 0u) {
    *out_request_id = 0u;
    return -1;
  }
  return 0;
}

int capy_icon_thumbnail_ready(struct capy_widget_context *ctx,
                              struct capy_widget *root, uint32_t request_id,
                              uint32_t image_id) {
  if (!ctx) {
    return -1;
  }
  if (request_id == 0u) {
    return -1;
  }
  if (root) {
    struct capy_widget_event ev;
    capy_widget_zero(&ev, sizeof(ev));
    ev.type = CAPY_WIDGET_EVENT_THUMBNAIL_READY;
    ev.x = (int32_t)request_id;
    ev.y = (int32_t)image_id;
    (void)capy_widget_handle_event(root, &ev);
  }
  return 0;
}

/* ------------------------------------------------------------------------
 * Wallpaper management (since 2.12)
 *
 * Reuses the LE byte helpers and FNV-1a from the desktop-layout block to
 * keep the canonical blob format consistent across `CUIS`, `CTHM`, `CDLA`
 * and `CWLP`.
 * ------------------------------------------------------------------------ */

int capy_wallpaper_validate(const void *buf, uint32_t len) {
  const uint8_t *b = (const uint8_t *)buf;
  uint16_t format_version;
  uint8_t fit_mode;
  uint16_t slide_count;
  uint32_t declared_checksum;
  uint32_t actual_checksum;
  uint16_t reserved;
  uint32_t expected_total;
  uint32_t i;
  if (!b || len < CAPY_WALLPAPER_HEADER_SIZE) {
    return -1;
  }
  if (b[0] != CAPY_WALLPAPER_MAGIC0 ||
      b[1] != CAPY_WALLPAPER_MAGIC1 ||
      b[2] != CAPY_WALLPAPER_MAGIC2 ||
      b[3] != CAPY_WALLPAPER_MAGIC3) {
    return -1;
  }
  format_version = capy_desktop_layout_read_u16(b + 4);
  if (format_version == 0u ||
      format_version > (uint16_t)CAPY_WALLPAPER_FORMAT_VERSION) {
    return -1;
  }
  fit_mode = b[6];
  if ((uint32_t)fit_mode >= (uint32_t)CAPY_WALLPAPER_FIT_COUNT) {
    return -1;
  }
  declared_checksum = capy_desktop_layout_read_u32(b + 8);
  slide_count = capy_desktop_layout_read_u16(b + 18);
  reserved = capy_desktop_layout_read_u16(b + 22);
  if (reserved != 0u) {
    return -1;
  }
  expected_total = (uint32_t)CAPY_WALLPAPER_HEADER_SIZE +
                   (uint32_t)slide_count *
                       (uint32_t)CAPY_WALLPAPER_ENTRY_SIZE;
  if (expected_total != len) {
    return -1;
  }
  actual_checksum = capy_desktop_layout_fnv1a(
      b + CAPY_WALLPAPER_HEADER_SIZE,
      len - CAPY_WALLPAPER_HEADER_SIZE);
  if (actual_checksum != declared_checksum) {
    return -1;
  }
  /* Per-entry validation: image_id > 0, duration > 0, reserved zero. */
  for (i = 0u; i < (uint32_t)slide_count; ++i) {
    const uint8_t *e = b + CAPY_WALLPAPER_HEADER_SIZE +
                       i * (uint32_t)CAPY_WALLPAPER_ENTRY_SIZE;
    uint32_t image_id = capy_desktop_layout_read_u32(e);
    uint16_t dur = capy_desktop_layout_read_u16(e + 4);
    uint16_t ereserved = capy_desktop_layout_read_u16(e + 6);
    if (image_id == 0u || dur == 0u || ereserved != 0u) {
      return -1;
    }
  }
  return 0;
}

int capy_wallpaper_load(const void *buf, uint32_t len,
                        struct capy_wallpaper_config *out_config,
                        struct capy_wallpaper_slide *out_slides, uint32_t cap,
                        uint32_t *out_count) {
  const uint8_t *b = (const uint8_t *)buf;
  uint16_t slide_count;
  uint32_t to_copy;
  uint32_t i;
  if (!out_config || !out_count) {
    return -1;
  }
  if (cap > 0u && !out_slides) {
    return -1;
  }
  if (capy_wallpaper_validate(buf, len) != 0) {
    return -1;
  }
  out_config->fit_mode = b[6];
  out_config->flags = b[7];
  out_config->default_image_id = capy_desktop_layout_read_u32(b + 12);
  out_config->slideshow_interval_sec = capy_desktop_layout_read_u16(b + 16);
  slide_count = capy_desktop_layout_read_u16(b + 18);
  out_config->monitor_index = capy_desktop_layout_read_u16(b + 20);
  out_config->reserved = 0u;
  *out_count = (uint32_t)slide_count;
  to_copy = (uint32_t)slide_count < cap ? (uint32_t)slide_count : cap;
  for (i = 0u; i < to_copy; ++i) {
    const uint8_t *e = b + CAPY_WALLPAPER_HEADER_SIZE +
                       i * (uint32_t)CAPY_WALLPAPER_ENTRY_SIZE;
    out_slides[i].image_id = capy_desktop_layout_read_u32(e);
    out_slides[i].duration_sec = capy_desktop_layout_read_u16(e + 4);
    out_slides[i].reserved = 0u;
  }
  return 0;
}

int capy_wallpaper_serialize(const struct capy_wallpaper_config *config,
                             const struct capy_wallpaper_slide *slides,
                             uint32_t count, void *out, uint32_t cap) {
  uint8_t *b = (uint8_t *)out;
  uint32_t total;
  uint32_t i;
  uint32_t checksum;
  if (!config || !out) {
    return -1;
  }
  if (count > 0u && !slides) {
    return -1;
  }
  if (count > 0xFFFFu) {
    return -1;
  }
  if ((uint32_t)config->fit_mode >= (uint32_t)CAPY_WALLPAPER_FIT_COUNT) {
    return -1;
  }
  /* Validate slides pre-flight to avoid partial writes. */
  for (i = 0u; i < count; ++i) {
    if (slides[i].image_id == 0u || slides[i].duration_sec == 0u) {
      return -1;
    }
  }
  total = (uint32_t)CAPY_WALLPAPER_HEADER_SIZE +
          count * (uint32_t)CAPY_WALLPAPER_ENTRY_SIZE;
  if (total > cap) {
    return -1;
  }
  b[0] = CAPY_WALLPAPER_MAGIC0;
  b[1] = CAPY_WALLPAPER_MAGIC1;
  b[2] = CAPY_WALLPAPER_MAGIC2;
  b[3] = CAPY_WALLPAPER_MAGIC3;
  capy_desktop_layout_write_u16(b + 4,
                                (uint16_t)CAPY_WALLPAPER_FORMAT_VERSION);
  b[6] = config->fit_mode;
  b[7] = config->flags;
  /* checksum placeholder */
  capy_desktop_layout_write_u32(b + 8, 0u);
  capy_desktop_layout_write_u32(b + 12, config->default_image_id);
  capy_desktop_layout_write_u16(b + 16, config->slideshow_interval_sec);
  capy_desktop_layout_write_u16(b + 18, (uint16_t)count);
  capy_desktop_layout_write_u16(b + 20, config->monitor_index);
  capy_desktop_layout_write_u16(b + 22, 0u);
  for (i = 0u; i < count; ++i) {
    uint8_t *e = b + CAPY_WALLPAPER_HEADER_SIZE +
                 i * (uint32_t)CAPY_WALLPAPER_ENTRY_SIZE;
    capy_desktop_layout_write_u32(e, slides[i].image_id);
    capy_desktop_layout_write_u16(e + 4, slides[i].duration_sec);
    capy_desktop_layout_write_u16(e + 6, 0u);
  }
  checksum = capy_desktop_layout_fnv1a(b + CAPY_WALLPAPER_HEADER_SIZE,
                                       total - CAPY_WALLPAPER_HEADER_SIZE);
  capy_desktop_layout_write_u32(b + 8, checksum);
  return (int)total;
}

/* ------------------------------------------------------------------------
 * Login screen (since 2.13)
 * ------------------------------------------------------------------------ */

uint8_t capy_login_choose_layout(uint32_t user_count) {
  if (user_count >= (uint32_t)CAPY_LOGIN_LIST_THRESHOLD) {
    return (uint8_t)CAPY_LOGIN_LAYOUT_LIST;
  }
  if (user_count >= (uint32_t)CAPY_LOGIN_GRID_THRESHOLD) {
    return (uint8_t)CAPY_LOGIN_LAYOUT_GRID;
  }
  return (uint8_t)CAPY_LOGIN_LAYOUT_SINGLE;
}

int capy_perf_counters_snapshot(const struct capy_widget_context *ctx,
                                const struct capy_widget *root,
                                struct capy_perf_counters *out) {
  if (!ctx || !out) {
    return -1;
  }
  out->widget_count = root ? capy_inspector_count_widgets(root) : 0u;
  out->plugin_count = ctx->plugin_count;
  if (ctx->undo_stack) {
    out->undo_count = ctx->undo_stack->count;
    out->undo_redo_count = ctx->undo_stack->redo_count;
    out->undo_capacity = ctx->undo_stack->capacity;
  } else {
    out->undo_count = 0u;
    out->undo_redo_count = 0u;
    out->undo_capacity = 0u;
  }
  out->dpi_scale_x256 = ctx->dpi_scale_x256;
  out->reserved_dpi = 0u;
  out->frame_budget_microseconds = ctx->frame_budget_microseconds;
  out->theme_variant = (uint16_t)ctx->theme.variant;
  out->theme_high_contrast = ctx->theme.high_contrast;
  out->reserved[0] = 0u;
  return 0;
}

/* ── v2.14: advanced widget state — date picker ────────────────────────── */

int capy_date_is_valid(uint16_t year, uint8_t month, uint8_t day) {
  static const uint8_t days_in_month[12] = {
      31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u};
  uint8_t max_day;
  if (year == 0u || month < 1u || month > 12u || day < 1u) {
    return 0;
  }
  max_day = days_in_month[month - 1u];
  if (month == 2u && ((year % 4u == 0u && year % 100u != 0u) ||
                      year % 400u == 0u)) {
    max_day = 29u; /* leap February */
  }
  return day <= max_day ? 1 : 0;
}

int capy_widget_set_date(struct capy_widget *w, uint16_t year, uint8_t month,
                         uint8_t day) {
  if (!w || w->type != CAPY_WIDGET_DATE_PICKER) {
    return -1;
  }
  if (!capy_date_is_valid(year, month, day)) {
    return -1; /* fail-closed: stored value left unchanged */
  }
  w->date_value.year = year;
  w->date_value.month = month;
  w->date_value.day = day;
  return 0;
}

int capy_widget_clear_date(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_DATE_PICKER) {
    return -1;
  }
  w->date_value.year = 0u;
  w->date_value.month = 0u;
  w->date_value.day = 0u;
  return 0;
}

int capy_widget_get_date(const struct capy_widget *w, struct capy_date *out) {
  if (!w || w->type != CAPY_WIDGET_DATE_PICKER || !out) {
    return -1;
  }
  *out = w->date_value;
  return capy_date_is_valid(w->date_value.year, w->date_value.month,
                            w->date_value.day)
             ? 1
             : 0;
}

/* ── v2.15: advanced widget state — color picker ───────────────────────── */

uint32_t capy_color_pack(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  /* 0xAARRGGBB. Cast each channel to uint32_t before shifting so the high
   * byte never triggers signed-shift UB (cf. the 1.3 zero-float/strict-C11
   * cleanup). */
  return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) |
         (uint32_t)b;
}

int capy_widget_set_color(struct capy_widget *w, uint32_t argb) {
  if (!w || w->type != CAPY_WIDGET_COLOR_PICKER) {
    return -1;
  }
  w->picker_color = argb;
  w->picker_color_set = 1u;
  return 0;
}

int capy_widget_clear_color(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_COLOR_PICKER) {
    return -1;
  }
  w->picker_color = 0u;
  w->picker_color_set = 0u;
  return 0;
}

int capy_widget_get_color(const struct capy_widget *w, uint32_t *out) {
  if (!w || w->type != CAPY_WIDGET_COLOR_PICKER || !out) {
    return -1;
  }
  *out = w->picker_color;
  return w->picker_color_set ? 1 : 0;
}

/* ── v2.16: advanced widget state — table columns ──────────────────────── */

int capy_widget_set_table_columns(struct capy_widget *w,
                                  const uint16_t *widths, uint16_t count) {
  if (!w || w->type != CAPY_WIDGET_TABLE) {
    return -1;
  }
  if (count == 0u) {
    w->table_column_widths = 0;
    w->table_column_count = 0u;
    return 0;
  }
  if (!widths) {
    return -1; /* fail-closed: count > 0 needs a real array; state unchanged */
  }
  w->table_column_widths = widths;
  w->table_column_count = count;
  return 0;
}

int capy_widget_clear_table_columns(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_TABLE) {
    return -1;
  }
  w->table_column_widths = 0;
  w->table_column_count = 0u;
  return 0;
}

int capy_widget_table_column_count(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_TABLE) {
    return -1;
  }
  return (int)w->table_column_count;
}

int capy_widget_table_column_width(const struct capy_widget *w, uint16_t index,
                                   uint16_t *out_width) {
  if (!w || w->type != CAPY_WIDGET_TABLE || !out_width) {
    return -1;
  }
  if (!w->table_column_widths || index >= w->table_column_count) {
    return -1; /* no model / out-of-range index — fail-closed */
  }
  *out_width = w->table_column_widths[index];
  return 0;
}

/* ── v2.17: advanced widget state — autocomplete suggestions ───────────── */

int capy_widget_set_autocomplete(struct capy_widget *w,
                                 const char *const *items, uint16_t count) {
  if (!w || w->type != CAPY_WIDGET_AUTOCOMPLETE) {
    return -1;
  }
  if (count == 0u) {
    w->autocomplete_items = 0;
    w->autocomplete_count = 0u;
    w->autocomplete_selected = -1;
    return 0;
  }
  if (!items) {
    return -1; /* fail-closed: count > 0 needs a real array; state unchanged */
  }
  w->autocomplete_items = items;
  w->autocomplete_count = count;
  w->autocomplete_selected = -1; /* new list -> nothing selected yet */
  return 0;
}

int capy_widget_clear_autocomplete(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_AUTOCOMPLETE) {
    return -1;
  }
  w->autocomplete_items = 0;
  w->autocomplete_count = 0u;
  w->autocomplete_selected = -1;
  return 0;
}

int capy_widget_autocomplete_count(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_AUTOCOMPLETE) {
    return -1;
  }
  return (int)w->autocomplete_count;
}

int capy_widget_autocomplete_item(const struct capy_widget *w, uint16_t index,
                                  const char **out_item) {
  if (!w || w->type != CAPY_WIDGET_AUTOCOMPLETE || !out_item) {
    return -1;
  }
  if (!w->autocomplete_items || index >= w->autocomplete_count) {
    return -1; /* no list / out-of-range index — fail-closed */
  }
  *out_item = w->autocomplete_items[index];
  return 0;
}

int capy_widget_set_autocomplete_selected(struct capy_widget *w,
                                          int32_t index) {
  if (!w || w->type != CAPY_WIDGET_AUTOCOMPLETE) {
    return -1;
  }
  if (index < -1 || index >= (int32_t)w->autocomplete_count) {
    return -1; /* only -1 (clear) or a live index is accepted — fail-closed */
  }
  w->autocomplete_selected = index;
  return 0;
}

int capy_widget_get_autocomplete_selected(const struct capy_widget *w,
                                          int32_t *out_index) {
  int32_t sel;
  if (!w || w->type != CAPY_WIDGET_AUTOCOMPLETE || !out_index) {
    return -1;
  }
  sel = w->autocomplete_selected;
  /* Clamp against the live count so a stale selection never points past the
   * list (also masks the zero-initialised default of a fresh widget). */
  if (w->autocomplete_count == 0u || sel < 0 ||
      sel >= (int32_t)w->autocomplete_count) {
    sel = -1;
  }
  *out_index = sel;
  return sel >= 0 ? 1 : 0;
}

/* ── v2.18: advanced widget state — tree hierarchy ─────────────────────── */

int capy_widget_set_tree_collapsed(struct capy_widget *w, int collapsed) {
  if (!w || w->type != CAPY_WIDGET_TREE) {
    return -1;
  }
  w->tree_collapsed = collapsed ? 1u : 0u;
  return 0;
}

int capy_widget_tree_is_collapsed(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_TREE) {
    return -1;
  }
  return w->tree_collapsed ? 1 : 0;
}

int capy_widget_set_tree_depth(struct capy_widget *w, uint16_t depth) {
  if (!w || w->type != CAPY_WIDGET_TREE) {
    return -1;
  }
  w->tree_depth = depth;
  return 0;
}

int capy_widget_tree_depth(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_TREE) {
    return -1;
  }
  return (int)w->tree_depth;
}

int capy_widget_tree_row_visible(const struct capy_widget *w) {
  const struct capy_widget *p;
  if (!w || w->type != CAPY_WIDGET_TREE) {
    return -1;
  }
  for (p = w->parent; p != 0; p = p->parent) {
    if (p->type == CAPY_WIDGET_TREE && p->tree_collapsed) {
      return 0; /* a collapsed TREE ancestor hides this row */
    }
  }
  return 1;
}

/* ── v2.19: advanced widget state — chart dataset ──────────────────────── */

int capy_widget_set_chart_data(struct capy_widget *w, const int32_t *values,
                               uint16_t count) {
  if (!w || w->type != CAPY_WIDGET_CHART) {
    return -1;
  }
  if (count == 0u) {
    w->chart_values = 0;
    w->chart_count = 0u;
    return 0;
  }
  if (!values) {
    return -1; /* fail-closed: count > 0 needs a real array; state unchanged */
  }
  w->chart_values = values;
  w->chart_count = count;
  return 0;
}

int capy_widget_clear_chart_data(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_CHART) {
    return -1;
  }
  w->chart_values = 0;
  w->chart_count = 0u;
  return 0;
}

int capy_widget_chart_count(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_CHART) {
    return -1;
  }
  return (int)w->chart_count;
}

int capy_widget_chart_value(const struct capy_widget *w, uint16_t index,
                            int32_t *out_value) {
  if (!w || w->type != CAPY_WIDGET_CHART || !out_value) {
    return -1;
  }
  if (!w->chart_values || index >= w->chart_count) {
    return -1; /* no data / out-of-range index — fail-closed */
  }
  *out_value = w->chart_values[index];
  return 0;
}

int capy_widget_chart_range(const struct capy_widget *w, int32_t *out_min,
                            int32_t *out_max) {
  int32_t min;
  int32_t max;
  int32_t v;
  uint16_t i;
  if (!w || w->type != CAPY_WIDGET_CHART || !out_min || !out_max) {
    return -1;
  }
  if (!w->chart_values || w->chart_count == 0u) {
    *out_min = 0;
    *out_max = 0;
    return 0; /* no data */
  }
  min = w->chart_values[0];
  max = w->chart_values[0];
  for (i = 1u; i < w->chart_count; ++i) {
    v = w->chart_values[i];
    if (v < min) {
      min = v;
    }
    if (v > max) {
      max = v;
    }
  }
  *out_min = min;
  *out_max = max;
  return 1;
}

/* ── v2.20: advanced widget state — rich-text ranges ───────────────────── */

int capy_widget_set_rich_text_ranges(struct capy_widget *w,
                                     const struct capy_text_range *ranges,
                                     uint16_t count) {
  if (!w || w->type != CAPY_WIDGET_RICH_TEXT) {
    return -1;
  }
  if (count == 0u) {
    w->rich_text_ranges = 0;
    w->rich_text_range_count = 0u;
    return 0;
  }
  if (!ranges) {
    return -1; /* fail-closed: count > 0 needs a real array; state unchanged */
  }
  w->rich_text_ranges = ranges;
  w->rich_text_range_count = count;
  return 0;
}

int capy_widget_clear_rich_text_ranges(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_RICH_TEXT) {
    return -1;
  }
  w->rich_text_ranges = 0;
  w->rich_text_range_count = 0u;
  return 0;
}

int capy_widget_rich_text_range_count(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_RICH_TEXT) {
    return -1;
  }
  return (int)w->rich_text_range_count;
}

int capy_widget_rich_text_range(const struct capy_widget *w, uint16_t index,
                                struct capy_text_range *out_range) {
  if (!w || w->type != CAPY_WIDGET_RICH_TEXT || !out_range) {
    return -1;
  }
  if (!w->rich_text_ranges || index >= w->rich_text_range_count) {
    return -1; /* no data / out-of-range index — fail-closed */
  }
  *out_range = w->rich_text_ranges[index];
  return 0;
}

int capy_widget_rich_text_range_at(const struct capy_widget *w, uint32_t offset,
                                   struct capy_text_range *out_range) {
  uint16_t i;
  int found = 0;
  if (!w || w->type != CAPY_WIDGET_RICH_TEXT || !out_range) {
    return -1;
  }
  out_range->start = 0u;
  out_range->length = 0u;
  out_range->flags = 0u;
  out_range->reserved = 0u;
  out_range->color = 0u;
  if (!w->rich_text_ranges || w->rich_text_range_count == 0u) {
    return 0; /* no ranges */
  }
  for (i = 0u; i < w->rich_text_range_count; ++i) {
    const struct capy_text_range *r = &w->rich_text_ranges[i];
    /* Covers `offset` on the half-open interval [start, start + length).
     * Guard with `offset >= start` first, then subtract to compare against
     * length — overflow-safe (no `start + length` that could wrap uint32).
     * Zero-length runs cover nothing. Last covering run wins. */
    if (r->length != 0u && offset >= r->start &&
        (offset - r->start) < r->length) {
      *out_range = *r;
      found = 1;
    }
  }
  return found ? 1 : 0;
}

/* ── v2.21: advanced widget state — canvas draw callback ───────────────── */

int capy_widget_set_canvas_draw(struct capy_widget *w, capy_canvas_draw_fn fn,
                                void *user_data) {
  if (!w || w->type != CAPY_WIDGET_CANVAS) {
    return -1;
  }
  if (!fn) {
    w->canvas_draw = 0;
    w->canvas_user_data = 0;
    return 0; /* NULL fn clears (user_data ignored) */
  }
  w->canvas_draw = fn;
  w->canvas_user_data = user_data;
  return 0;
}

int capy_widget_clear_canvas_draw(struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_CANVAS) {
    return -1;
  }
  w->canvas_draw = 0;
  w->canvas_user_data = 0;
  return 0;
}

int capy_widget_has_canvas_draw(const struct capy_widget *w) {
  if (!w || w->type != CAPY_WIDGET_CANVAS) {
    return -1;
  }
  return w->canvas_draw ? 1 : 0;
}

int capy_widget_canvas_user_data(const struct capy_widget *w,
                                 void **out_user_data) {
  if (!w || w->type != CAPY_WIDGET_CANVAS || !out_user_data) {
    return -1;
  }
  *out_user_data = w->canvas_user_data;
  return 0;
}

int capy_widget_canvas_draw(struct capy_widget *w,
                            struct capy_display_list *dl) {
  if (!w || w->type != CAPY_WIDGET_CANVAS || !dl || !w->canvas_draw) {
    return -1; /* fail-closed: nothing to draw, or bad args */
  }
  /* Forward to the caller-owned callback; normalise its result to 0/-1.
   * CapyUI never inspects the dl contents the callback produced. */
  return w->canvas_draw(w, dl, w->canvas_user_data) == 0 ? 0 : -1;
}
