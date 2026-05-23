#include "gui/taskbar.h"
#include "gui/font.h"
#include "gui/compositor.h"

#include "internal/taskbar_internal.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#include "gui/capyui_display_adapter.h"
#include "capy_display_list.h"
#include "util/kstring.h"

#define TASKBAR_DL_CMD_CAP 256u
#define TASKBAR_DL_TEXT_CAP 2048u
#define TASKBAR_DL_FRAME_COUNT 2u

static struct capy_dl_cmd g_taskbar_dl_cmds[TASKBAR_DL_FRAME_COUNT][TASKBAR_DL_CMD_CAP];
static char g_taskbar_dl_text[TASKBAR_DL_FRAME_COUNT][TASKBAR_DL_TEXT_CAP];
static struct capy_display_list g_taskbar_dl[TASKBAR_DL_FRAME_COUNT];
static int g_taskbar_dl_initialized = 0;
static int g_taskbar_dl_have_prev = 0;
static uint8_t g_taskbar_dl_prev = 0u;

static void taskbar_dl_prepare(uint8_t index) {
  struct capy_display_list *dl = &g_taskbar_dl[index];
  dl->cmds = g_taskbar_dl_cmds[index];
  dl->count = 0u;
  dl->capacity = TASKBAR_DL_CMD_CAP;
  dl->text_pool = g_taskbar_dl_text[index];
  dl->text_used = 0u;
  dl->text_capacity = TASKBAR_DL_TEXT_CAP;
  dl->version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl->theme = 0;
  dl->dpi_scale_x256 = 256u;
  dl->reserved_dpi = 0u;
}

static void taskbar_dl_init_once(void) {
  if (g_taskbar_dl_initialized) return;
  taskbar_dl_prepare(0u);
  taskbar_dl_prepare(1u);
  g_taskbar_dl_initialized = 1;
}

void taskbar_display_list_reset(void) {
  g_taskbar_dl_have_prev = 0;
}

static struct capy_dl_cmd *taskbar_dl_push(struct capy_display_list *dl) {
  struct capy_dl_cmd *cmd;
  if (!dl || !dl->cmds || dl->count >= dl->capacity) return 0;
  cmd = &dl->cmds[dl->count++];
  kmemzero(cmd, sizeof(*cmd));
  return cmd;
}

static int taskbar_dl_emit_rect(struct capy_display_list *dl,
                                int32_t x,
                                int32_t y,
                                uint32_t width,
                                uint32_t height,
                                uint32_t color) {
  struct capy_dl_cmd *cmd;
  if (width == 0u || height == 0u) return 0;
  cmd = taskbar_dl_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_RECT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  return 0;
}

static int taskbar_dl_copy_text(struct capy_display_list *dl,
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

static int taskbar_dl_emit_text(struct capy_display_list *dl,
                                int32_t x,
                                int32_t y,
                                uint32_t width,
                                uint32_t height,
                                const char *text,
                                uint32_t color) {
  struct capy_dl_cmd *cmd;
  uint16_t text_offset = 0u;
  uint16_t text_len = 0u;
  if (!text || !text[0]) return 0;
  if (taskbar_dl_copy_text(dl, text, &text_offset, &text_len) != 0) return -1;
  cmd = taskbar_dl_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_TEXT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  cmd->text_offset = text_offset;
  cmd->text_len = text_len;
  cmd->font_size = 16u;
  return 0;
}

static int taskbar_dl_emit_fit(struct capy_display_list *dl,
                               const struct font *f,
                               int32_t x,
                               int32_t y,
                               uint32_t max_width,
                               const char *text,
                               uint32_t color) {
  char fitted[TASKBAR_ITEM_NAME_MAX];
  if (!f) return 0;
  tb_fit_text(f, text, max_width, fitted, sizeof(fitted));
  if (!fitted[0]) return 0;
  return taskbar_dl_emit_text(dl, x, y, max_width, f->glyph_height, fitted, color);
}

static int taskbar_dl_has_prefix(const char *text, const char *prefix) {
  uint32_t i = 0u;
  if (!text || !prefix) return 0;
  while (prefix[i]) {
    if (text[i] != prefix[i]) return 0;
    i++;
  }
  return 1;
}

static int taskbar_dl_tray_net_state(struct taskbar *tb) {
  if (!tb) return 0;
  if (taskbar_dl_has_prefix(tb->tray_text, "net-on")) return 1;
  if (taskbar_dl_has_prefix(tb->tray_text, "net-wait")) return 2;
  if (taskbar_dl_has_prefix(tb->tray_text, "net-off")) return 3;
  return 0;
}

static int32_t taskbar_dl_clock_x(struct taskbar *tb,
                                  const struct font *f,
                                  uint32_t surface_w) {
  uint32_t cw = 0u;
  if (!tb || !tb->show_clock || !f) return (int32_t)surface_w;
  cw = font_string_width(f, tb->clock_text);
  if (surface_w <= cw + 12u) return 0;
  return (int32_t)(surface_w - cw - 12u);
}

static uint32_t taskbar_dl_tray_width(struct taskbar *tb,
                                      const struct font *f) {
  uint32_t tw = 0u;
  if (!tb || !tb->tray_text[0]) return 0u;
  if (taskbar_dl_tray_net_state(tb) != 0) return 34u;
  if (!f) return 0u;
  tw = font_string_width(f, tb->tray_text) + 12u;
  return (tw < 44u) ? 44u : tw;
}

static int32_t taskbar_dl_tray_x(struct taskbar *tb,
                                 const struct font *f,
                                 uint32_t surface_w) {
  uint32_t tw = taskbar_dl_tray_width(tb, f);
  int32_t cx = taskbar_dl_clock_x(tb, f, surface_w);
  if (tw == 0u) return cx;
  if (cx <= (int32_t)(tw + 8u)) return 0;
  return cx - (int32_t)tw - 8;
}

static int32_t taskbar_dl_items_right_edge(struct taskbar *tb,
                                           const struct font *f,
                                           uint32_t surface_w) {
  int32_t right = (int32_t)surface_w - 4;
  if (tb && tb->show_clock) right = taskbar_dl_clock_x(tb, f, surface_w) - 8;
  if (tb && tb->tray_text[0]) right = taskbar_dl_tray_x(tb, f, surface_w) - 8;
  if (right < 0) right = 0;
  return right;
}

static uint32_t taskbar_dl_mix_color(uint32_t color,
                                     uint32_t target,
                                     uint8_t amount) {
  uint32_t r = (color >> 16) & 0xFFu;
  uint32_t g = (color >> 8) & 0xFFu;
  uint32_t b = color & 0xFFu;
  uint32_t tr = (target >> 16) & 0xFFu;
  uint32_t tg = (target >> 8) & 0xFFu;
  uint32_t tb = target & 0xFFu;
  r = r + ((tr > r) ? ((tr - r) * amount) / 255u
                    : -((r - tr) * amount) / 255u);
  g = g + ((tg > g) ? ((tg - g) * amount) / 255u
                    : -((g - tg) * amount) / 255u);
  b = b + ((tb > b) ? ((tb - b) * amount) / 255u
                    : -((b - tb) * amount) / 255u);
  return (r << 16) | (g << 8) | b;
}

static int taskbar_dl_emit_launcher_mark(struct capy_display_list *dl,
                                         int32_t x,
                                         int32_t y,
                                         uint32_t bg,
                                         uint32_t fg) {
  uint32_t shadow = taskbar_dl_mix_color(bg, 0x00000000, 72u);
  if (taskbar_dl_emit_rect(dl, x + 1, y + 2, 15u, 15u, shadow) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x, y, 7u, 7u, fg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 9, y, 7u, 7u, fg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x, y + 9, 7u, 7u, fg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 9, y + 9, 7u, 7u, fg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 2, y + 2, 3u, 3u, bg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 11, y + 2, 3u, 3u, bg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 2, y + 11, 3u, 3u, bg) != 0) return -1;
  return taskbar_dl_emit_rect(dl, x + 11, y + 11, 3u, 3u, bg);
}

static int taskbar_dl_emit_net_tray(struct capy_display_list *dl,
                                    int32_t x,
                                    int32_t y,
                                    uint32_t w,
                                    int state,
                                    const struct gui_theme_palette *theme) {
  uint32_t bars = state == 1 ? theme->accent : theme->text_muted;
  uint32_t alert = 0x00CC3333;
  if (w < 24u) return 0;
  if (taskbar_dl_emit_rect(dl, x, y, w, TASKBAR_HEIGHT - 8, theme->window_border) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 1, y + 1, w - 2u, TASKBAR_HEIGHT - 10, theme->taskbar_bg) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 8, y + 15, 3u, 5u, bars) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 13, y + 11, 3u, 9u,
                           state == 3 ? theme->text_muted : bars) != 0) return -1;
  if (taskbar_dl_emit_rect(dl, x + 18, y + 7, 3u, 13u,
                           state == 1 ? bars : theme->text_muted) != 0) return -1;
  if (state == 2) {
    if (taskbar_dl_emit_rect(dl, x + 24, y + 15, 2u, 2u, theme->text_muted) != 0) return -1;
    if (taskbar_dl_emit_rect(dl, x + 28, y + 15, 2u, 2u, theme->text_muted) != 0) return -1;
  }
  if (state == 3) {
    for (uint32_t i = 0u; i < 7u; i++) {
      if (taskbar_dl_emit_rect(dl, x + 24 + (int32_t)i, y + 8 + (int32_t)i, 1u, 1u, alert) != 0) return -1;
      if (taskbar_dl_emit_rect(dl, x + 30 - (int32_t)i, y + 8 + (int32_t)i, 1u, 1u, alert) != 0) return -1;
    }
  }
  return 0;
}

static int taskbar_emit_display_list(void *producer,
                                     struct capy_display_list *out) {
  struct taskbar *tb = (struct taskbar *)producer;
  const struct gui_theme_palette *theme = compositor_theme();
  const struct font *f = font_default();
  uint8_t scale = compositor_ui_scale();
  uint32_t menu_w = 82u + 20u * (uint32_t)(scale - 1u);
  uint32_t item_w = 120u + 28u * (uint32_t)(scale - 1u);
  struct gui_surface *s;
  int32_t x = 4;
  int32_t item_right;
  if (!tb || !tb->window || !out) return -1;
  s = &tb->window->surface;
  tb->bg_color = theme->taskbar_bg;
  tb->fg_color = theme->taskbar_fg;
  tb->highlight_color = theme->taskbar_highlight;
  out->count = 0u;
  out->text_used = 0u;
  if (taskbar_dl_emit_rect(out, 0, 0, s->width, s->height, tb->bg_color) != 0) return -1;
  if (taskbar_dl_emit_rect(out, 0, 0, s->width, 1u, theme->accent_alt) != 0) return -1;
  if (taskbar_dl_emit_rect(out, 0, 1, s->width, 1u, theme->window_border) != 0) return -1;
  if (!f) return 0;
  item_right = taskbar_dl_items_right_edge(tb, f, s->width);
  {
    uint32_t menu_btn_bg = tb->menu_open ? theme->accent_alt : theme->accent;
    if (taskbar_dl_emit_rect(out, x, 4, menu_w, TASKBAR_HEIGHT - 8, menu_btn_bg) != 0) return -1;
    if (taskbar_dl_emit_rect(out, x, 4, menu_w, 1u, theme->accent_text) != 0) return -1;
    if (taskbar_dl_emit_rect(out, x, TASKBAR_HEIGHT - 5, menu_w, 1u,
                             taskbar_dl_mix_color(menu_btn_bg, 0x00000000, 88u)) != 0) return -1;
    if (taskbar_dl_emit_rect(out, x, 4, 1u, TASKBAR_HEIGHT - 8,
                             taskbar_dl_mix_color(menu_btn_bg, 0x00000000, 72u)) != 0) return -1;
    if (taskbar_dl_emit_launcher_mark(out, x + 8, 8, menu_btn_bg, theme->accent_text) != 0) return -1;
    if (taskbar_dl_emit_fit(out, f, x + 30, 8, (menu_w > 36u) ? menu_w - 36u : 0u,
                            "Capy", theme->accent_text) != 0) return -1;
    x += (int32_t)menu_w + 8;
  }
  for (uint32_t i = 0u; i < tb->item_count; i++) {
    struct taskbar_item *item = &tb->items[i];
    uint32_t draw_w = item_w;
    uint32_t bg = item->focused ? tb->highlight_color : tb->bg_color;
    uint32_t edge = item->focused ? theme->accent : theme->window_border;
    if (x >= item_right) break;
    if (x + (int32_t)draw_w > item_right) draw_w = (uint32_t)(item_right - x);
    if (draw_w < 16u) break;
    if (taskbar_dl_emit_rect(out, x, 4, draw_w, TASKBAR_HEIGHT - 8, bg) != 0) return -1;
    if (taskbar_dl_emit_rect(out, x, 4, draw_w, 1u, edge) != 0) return -1;
    if (taskbar_dl_emit_rect(out, x, TASKBAR_HEIGHT - 5, draw_w, 1u, edge) != 0) return -1;
    if (taskbar_dl_emit_rect(out, x, 4, 1u, TASKBAR_HEIGHT - 8, edge) != 0) return -1;
    if (taskbar_dl_emit_fit(out, f, x + 6, 8, (draw_w > 12u) ? draw_w - 12u : 0u,
                            item->name, tb->fg_color) != 0) return -1;
    x += (int32_t)item_w + 4;
  }
  if (tb->tray_text[0]) {
    uint32_t tray_w = taskbar_dl_tray_width(tb, f);
    int32_t tx = taskbar_dl_tray_x(tb, f, s->width);
    int state = taskbar_dl_tray_net_state(tb);
    if (tray_w > 12u && tx < (int32_t)s->width) {
      if (tx < 0) tx = 0;
      if ((uint32_t)tx + tray_w > s->width) tray_w = s->width - (uint32_t)tx;
      if (tray_w > 12u) {
        if (state) {
          if (taskbar_dl_emit_net_tray(out, tx, 4, tray_w, state, theme) != 0) return -1;
        } else {
          if (taskbar_dl_emit_rect(out, tx, 4, tray_w, TASKBAR_HEIGHT - 8,
                                   theme->window_border) != 0) return -1;
          if (taskbar_dl_emit_rect(out, tx + 1, 5, tray_w - 2u,
                                   TASKBAR_HEIGHT - 10, tb->bg_color) != 0) return -1;
          if (taskbar_dl_emit_fit(out, f, tx + 6, 8, tray_w - 12u,
                                  tb->tray_text, tb->fg_color) != 0) return -1;
        }
      }
    }
  }
  if (tb->show_clock) {
    int32_t cx = taskbar_dl_clock_x(tb, f, s->width);
    uint32_t cw = font_string_width(f, tb->clock_text);
    int32_t pill_x = (cx > 6) ? cx - 6 : 0;
    uint32_t pill_w = cw + 12u;
    if (pill_x < (int32_t)s->width) {
      if ((uint32_t)pill_x + pill_w > s->width) pill_w = s->width - (uint32_t)pill_x;
      if (taskbar_dl_emit_rect(out, pill_x, 4, pill_w, TASKBAR_HEIGHT - 8,
                               theme->accent_alt) != 0) return -1;
    }
    if (taskbar_dl_emit_fit(out, f, cx, 8,
                            (s->width > (uint32_t)cx) ? s->width - (uint32_t)cx : 0u,
                            tb->clock_text, tb->fg_color) != 0) return -1;
  }
  return 0;
}

int taskbar_render_display_list(struct taskbar *tb) {
  const struct capy_display_list *prev;
  uint8_t next;
  int rc;
  if (!tb || !tb->window) return -1;
  taskbar_dl_init_once();
  next = g_taskbar_dl_have_prev ? (uint8_t)(g_taskbar_dl_prev ^ 1u) : 0u;
  taskbar_dl_prepare(next);
  prev = g_taskbar_dl_have_prev ? &g_taskbar_dl[g_taskbar_dl_prev] : 0;
  rc = capyui_display_adapter_render_producer_window(
      tb->window, prev, &g_taskbar_dl[next], taskbar_emit_display_list, tb, 0);
  if (rc != CAPYUI_DISPLAY_ADAPTER_OK) return rc;
  g_taskbar_dl_prev = next;
  g_taskbar_dl_have_prev = 1;
  return CAPYUI_DISPLAY_ADAPTER_OK;
}
#endif
