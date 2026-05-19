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

int capy_widget_handle_event(struct capy_widget *widget,
                             const struct capy_widget_event *event) {
  if (!widget || !event || !widget->visible || !widget->enabled) {
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
  t.version = 1u;
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
  t.version = 1u;
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
  t.version = 1u;
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
