#include "internal/app_display_list_bridge.h"

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#include "gui/font.h"
#include "util/kstring.h"
#include <stddef.h>

static void app_display_list_prepare(struct app_display_list_bridge *bridge,
                                     uint8_t index) {
  struct capy_display_list *dl = &bridge->lists[index];
  dl->cmds = bridge->cmds[index];
  dl->count = 0u;
  dl->capacity = bridge->cmd_cap;
  dl->text_pool = bridge->text[index];
  dl->text_used = 0u;
  dl->text_capacity = bridge->text_cap;
  dl->version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl->theme = 0;
  dl->dpi_scale_x256 = 256u;
  dl->reserved_dpi = 0u;
}

void app_display_list_bridge_init(struct app_display_list_bridge *bridge,
                                  struct capy_dl_cmd *cmds0,
                                  struct capy_dl_cmd *cmds1,
                                  uint32_t cmd_cap,
                                  char *text0,
                                  char *text1,
                                  uint32_t text_cap) {
  if (!bridge) return;
  bridge->cmds[0] = cmds0;
  bridge->cmds[1] = cmds1;
  bridge->text[0] = text0;
  bridge->text[1] = text1;
  bridge->cmd_cap = cmd_cap;
  bridge->text_cap = text_cap;
  bridge->initialized = 1;
  bridge->have_prev = 0;
  bridge->prev_index = 0u;
  app_display_list_prepare(bridge, 0u);
  app_display_list_prepare(bridge, 1u);
}

void app_display_list_bridge_reset(struct app_display_list_bridge *bridge) {
  if (!bridge) return;
  bridge->have_prev = 0;
}

int app_display_list_bridge_render_window(struct app_display_list_bridge *bridge,
                                          struct gui_window *window,
                                          capyui_display_adapter_emit_fn emit,
                                          void *producer) {
  const struct capy_display_list *prev;
  uint8_t next;
  int rc;
  if (!bridge || !bridge->initialized || !window || !emit) return -1;
  next = bridge->have_prev ? (uint8_t)(bridge->prev_index ^ 1u) : 0u;
  app_display_list_prepare(bridge, next);
  prev = bridge->have_prev ? &bridge->lists[bridge->prev_index] : 0;
  rc = capyui_display_adapter_render_producer_window(
      window, prev, &bridge->lists[next], emit, producer, 0);
  if (rc != CAPYUI_DISPLAY_ADAPTER_OK) return rc;
  bridge->prev_index = next;
  bridge->have_prev = 1;
  return CAPYUI_DISPLAY_ADAPTER_OK;
}

struct capy_dl_cmd *app_display_list_push(struct capy_display_list *dl) {
  struct capy_dl_cmd *cmd;
  if (!dl || !dl->cmds || dl->count >= dl->capacity) return 0;
  cmd = &dl->cmds[dl->count++];
  kmemzero(cmd, sizeof(*cmd));
  return cmd;
}

int app_display_list_emit_rect(struct capy_display_list *dl,
                               int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height,
                               uint32_t color) {
  struct capy_dl_cmd *cmd = app_display_list_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_RECT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  return 0;
}

int app_display_list_emit_border_rect(struct capy_display_list *dl,
                                      int32_t x,
                                      int32_t y,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t color,
                                      uint8_t border_width) {
  struct capy_dl_cmd *cmd;
  if (border_width == 0u) return 0;
  cmd = app_display_list_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_BORDER;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  cmd->border_width = border_width;
  return 0;
}

static int app_display_list_copy_text(struct capy_display_list *dl,
                                      const char *text,
                                      uint16_t *offset,
                                      uint16_t *len) {
  uint32_t n = 0u;
  if (!offset || !len) return -1;
  *offset = 0u;
  *len = 0u;
  if (!text || !text[0]) return 0;
  while (text[n] && n < 0xFFFFu) ++n;
  if (!dl || !dl->text_pool || dl->text_used + n > dl->text_capacity) return -1;
  *offset = (uint16_t)dl->text_used;
  *len = (uint16_t)n;
  for (uint32_t i = 0u; i < n; ++i) dl->text_pool[dl->text_used + i] = text[i];
  dl->text_used += n;
  return 0;
}

int app_display_list_emit_text(struct capy_display_list *dl,
                               int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height,
                               const char *text,
                               uint32_t color,
                               uint8_t font_size) {
  struct capy_dl_cmd *cmd;
  uint16_t text_offset = 0u;
  uint16_t text_len = 0u;
  if (!text || !text[0]) return 0;
  if (app_display_list_copy_text(dl, text, &text_offset, &text_len) != 0) return -1;
  cmd = app_display_list_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_TEXT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  cmd->text_offset = text_offset;
  cmd->text_len = text_len;
  cmd->font_size = font_size;
  return 0;
}

int app_display_list_emit_widget(struct capy_display_list *dl,
                                 const struct widget *w) {
  const struct font *f;
  uint32_t bg;
  int32_t tx;
  int32_t ty;
  uint32_t tw;
  uint32_t text_width;
  if (!w || !w->visible) return 0;
  bg = w->hovered ? w->style.hover_color : w->style.bg_color;
  if (!w->enabled) bg = 0xC0C0C0u;
  if (app_display_list_emit_rect(dl, w->bounds.x, w->bounds.y,
                                 w->bounds.width, w->bounds.height, bg) != 0) {
    return -1;
  }
  if (app_display_list_emit_border_rect(dl, w->bounds.x, w->bounds.y,
                                        w->bounds.width, w->bounds.height,
                                        w->style.border_color,
                                        w->style.border_width) != 0) {
    return -1;
  }
  if (!w->text[0]) return 0;
  f = font_default();
  if (!f) return 0;
  tx = w->bounds.x + w->style.padding + w->style.border_width;
  ty = w->bounds.y + (int32_t)(w->bounds.height / 2u) -
       (int32_t)(f->glyph_height / 2u);
  if (w->type == WIDGET_BUTTON) {
    tw = font_string_width(f, w->text);
    tx = (tw < w->bounds.width)
             ? w->bounds.x + (int32_t)((w->bounds.width - tw) / 2u)
             : w->bounds.x + 2;
  }
  text_width = (tx > w->bounds.x)
                   ? w->bounds.width - (uint32_t)(tx - w->bounds.x)
                   : w->bounds.width;
  return app_display_list_emit_text(dl, tx, ty, text_width, f->glyph_height,
                                    w->text, w->style.text_color,
                                    w->style.font_size);
}
#endif
