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
