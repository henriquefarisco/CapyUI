#include "gui/taskbar.h"
#include "gui/font.h"
#include "gui/compositor.h"
#include "lang/app_language.h"

#include "internal/taskbar_internal.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#include "gui/capyui_display_adapter.h"
#include "capy_display_list.h"
#include "util/kstring.h"

#define TASKBAR_MENU_DL_CMD_CAP 512u
#define TASKBAR_MENU_DL_TEXT_CAP 8192u
#define TASKBAR_MENU_DL_FRAME_COUNT 2u

struct taskbar_menu_dl_bridge {
  struct capy_dl_cmd cmds[TASKBAR_MENU_DL_FRAME_COUNT][TASKBAR_MENU_DL_CMD_CAP];
  char text[TASKBAR_MENU_DL_FRAME_COUNT][TASKBAR_MENU_DL_TEXT_CAP];
  struct capy_display_list lists[TASKBAR_MENU_DL_FRAME_COUNT];
  int initialized;
  int have_prev;
  uint8_t prev_index;
  uint32_t last_w;
  uint32_t last_h;
};

static struct taskbar_menu_dl_bridge g_menu_dl;
static struct taskbar_menu_dl_bridge g_recent_dl;

static void taskbar_menu_dl_prepare(struct taskbar_menu_dl_bridge *bridge,
                                    uint8_t index) {
  struct capy_display_list *dl = &bridge->lists[index];
  dl->cmds = bridge->cmds[index];
  dl->count = 0u;
  dl->capacity = TASKBAR_MENU_DL_CMD_CAP;
  dl->text_pool = bridge->text[index];
  dl->text_used = 0u;
  dl->text_capacity = TASKBAR_MENU_DL_TEXT_CAP;
  dl->version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl->theme = 0;
  dl->dpi_scale_x256 = 256u;
  dl->reserved_dpi = 0u;
}

static void taskbar_menu_dl_init_once(struct taskbar_menu_dl_bridge *bridge) {
  if (!bridge || bridge->initialized) return;
  taskbar_menu_dl_prepare(bridge, 0u);
  taskbar_menu_dl_prepare(bridge, 1u);
  bridge->initialized = 1;
}

static struct capy_dl_cmd *taskbar_menu_dl_push(struct capy_display_list *dl) {
  struct capy_dl_cmd *cmd;
  if (!dl || !dl->cmds || dl->count >= dl->capacity) return 0;
  cmd = &dl->cmds[dl->count++];
  kmemzero(cmd, sizeof(*cmd));
  return cmd;
}

static int taskbar_menu_dl_emit_rect(struct capy_display_list *dl,
                                     int32_t x,
                                     int32_t y,
                                     uint32_t width,
                                     uint32_t height,
                                     uint32_t color) {
  struct capy_dl_cmd *cmd;
  if (width == 0u || height == 0u) return 0;
  cmd = taskbar_menu_dl_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_RECT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  return 0;
}

static int taskbar_menu_dl_copy_text(struct capy_display_list *dl,
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

static int taskbar_menu_dl_emit_text(struct capy_display_list *dl,
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
  if (taskbar_menu_dl_copy_text(dl, text, &text_offset, &text_len) != 0) return -1;
  cmd = taskbar_menu_dl_push(dl);
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

static int taskbar_menu_dl_emit_fit(struct capy_display_list *dl,
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
  return taskbar_menu_dl_emit_text(dl, x, y, max_width, f->glyph_height, fitted, color);
}

static uint32_t taskbar_menu_dl_lighten(uint32_t color, uint8_t amount) {
  uint32_t r = (color >> 16) & 0xFFu;
  uint32_t g = (color >> 8) & 0xFFu;
  uint32_t b = color & 0xFFu;
  uint32_t a = color & 0xFF000000u;
  r = r + ((255u - r) * amount) / 255u;
  g = g + ((255u - g) * amount) / 255u;
  b = b + ((255u - b) * amount) / 255u;
  if (r > 255u) r = 255u;
  if (g > 255u) g = 255u;
  if (b > 255u) b = 255u;
  return a | (r << 16) | (g << 8) | b;
}

static uint32_t taskbar_menu_dl_darken(uint32_t color, uint8_t amount) {
  uint32_t r = (color >> 16) & 0xFFu;
  uint32_t g = (color >> 8) & 0xFFu;
  uint32_t b = color & 0xFFu;
  uint32_t a = color & 0xFF000000u;
  r = r - (r * amount) / 255u;
  g = g - (g * amount) / 255u;
  b = b - (b * amount) / 255u;
  return a | (r << 16) | (g << 8) | b;
}

static int taskbar_menu_dl_emit_symbol(struct capy_display_list *dl,
                                       int32_t x,
                                       int32_t y,
                                       uint32_t bg,
                                       uint32_t fg,
                                       int symbol) {
  if (taskbar_menu_dl_emit_rect(dl, x, y, 16u, 16u, bg) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(dl, x, y, 16u, 1u,
                                taskbar_menu_dl_lighten(bg, 90u)) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(dl, x, y + 15, 16u, 1u,
                                taskbar_menu_dl_darken(bg, 96u)) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(dl, x, y, 1u, 16u,
                                taskbar_menu_dl_darken(bg, 72u)) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(dl, x + 15, y, 1u, 16u,
                                taskbar_menu_dl_darken(bg, 72u)) != 0) return -1;
  if (symbol == 4) {
    if (taskbar_menu_dl_emit_rect(dl, x + 4, y + 7, 8u, 2u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 9, y + 4, 2u, 2u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 11, y + 6, 2u, 2u, fg) != 0) return -1;
    return taskbar_menu_dl_emit_rect(dl, x + 9, y + 9, 2u, 2u, fg);
  }
  if (symbol == 10 || symbol == 11) {
    if (taskbar_menu_dl_emit_rect(dl, x + 3, y + 7, 9u, 2u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 10, y + 4, 2u, 2u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 12, y + 6, 2u, 2u, fg) != 0) return -1;
    return taskbar_menu_dl_emit_rect(dl, x + 10, y + 9, 2u, 2u, fg);
  }
  if (symbol == 9) {
    if (taskbar_menu_dl_emit_rect(dl, x + 7, y + 3, 2u, 6u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 4, y + 6, 2u, 5u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 10, y + 6, 2u, 5u, fg) != 0) return -1;
    return taskbar_menu_dl_emit_rect(dl, x + 5, y + 11, 6u, 2u, fg);
  }
  if (symbol == 1) {
    if (taskbar_menu_dl_emit_rect(dl, x + 4, y + 4, 3u, 3u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 9, y + 4, 3u, 3u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 4, y + 9, 3u, 3u, fg) != 0) return -1;
    return taskbar_menu_dl_emit_rect(dl, x + 9, y + 9, 3u, 3u, fg);
  }
  if (symbol == 2 || symbol == 6) {
    if (taskbar_menu_dl_emit_rect(dl, x + 5, y + 3, 7u, 10u, fg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 7, y + 5, 3u, 1u, bg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, x + 7, y + 8, 3u, 1u, bg) != 0) return -1;
    return taskbar_menu_dl_emit_rect(dl, x + 7, y + 11, 2u, 1u, bg);
  }
  if (taskbar_menu_dl_emit_rect(dl, x + 5, y + 3, 6u, 2u, fg) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(dl, x + 3, y + 6, 10u, 2u, fg) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(dl, x + 3, y + 9, 10u, 4u, fg) != 0) return -1;
  return taskbar_menu_dl_emit_rect(dl, x + 5, y + 10, 6u, 1u, bg);
}

static int taskbar_menu_dl_row_symbol(struct taskbar *tb, int row) {
  const char *label = NULL;
  if (row == TASKBAR_MENU_ROW_RECENT_TOGGLE) return 4;
  if (taskbar_row_is_recent(row)) return 2;
  if (!tb || row < 0 || row >= (int)tb->menu_entry_count) return 1;
  label = tb->menu_entries[row].label;
  if (tb_contains_ci(label, "Terminal")) return 5;
  if (tb_contains_ci(label, "Arquivo") || tb_contains_ci(label, "Files") ||
      tb_contains_ci(label, "Archivos")) return 0;
  if (tb_contains_ci(label, "Editor")) return 6;
  if (tb_contains_ci(label, "Calcul")) return 7;
  if (tb_contains_ci(label, "Config") || tb_contains_ci(label, "Settings")) return 3;
  if (tb_contains_ci(label, "Tarefas") || tb_contains_ci(label, "Tasks")) return 8;
  if (tb_contains_ci(label, "Logoff") || tb_contains_ci(label, "Logout") ||
      tb_contains_ci(label, "Sair")) return 10;
  if (tb_contains_ci(label, "Rein") || tb_contains_ci(label, "Restart") ||
      tb_contains_ci(label, "Reboot")) return 11;
  if (tb_contains_ci(label, "Deslig") || tb_contains_ci(label, "Shutdown") ||
      tb_contains_ci(label, "Power")) return 9;
  if (tb->menu_entries[row].pinned) return 1;
  if (taskbar_menu_entry_is_session(tb, (uint32_t)row)) return 3;
  return 0;
}

static int taskbar_menu_dl_emit_row(struct capy_display_list *dl,
                                    struct taskbar *tb,
                                    const struct font *f,
                                    const struct gui_theme_palette *theme,
                                    uint32_t width,
                                    int row,
                                    int32_t ey,
                                    const char *label) {
  int active;
  uint32_t symbol_bg;
  uint32_t symbol_fg;
  if (!tb || !f || !label) return 0;
  active = (row == tb->hover_entry) ||
           (tb->hover_entry < 0 && row == tb->selected_entry);
  if (active) {
    uint32_t hover_bg = taskbar_menu_dl_lighten(theme->window_bg, 40u);
    if (width > 4u) {
      if (taskbar_menu_dl_emit_rect(dl, 4, ey, width - 4u,
                                    TASKBAR_MENU_ENTRY_HEIGHT, hover_bg) != 0) return -1;
    }
    if (taskbar_menu_dl_emit_rect(dl, 4, ey, 4u,
                                  TASKBAR_MENU_ENTRY_HEIGHT, theme->accent) != 0) return -1;
  }
  symbol_bg = active ? theme->accent : theme->taskbar_bg;
  symbol_fg = active ? theme->accent_text : theme->text;
  if (taskbar_menu_dl_emit_symbol(dl, 14, ey + 5, symbol_bg, symbol_fg,
                                  taskbar_menu_dl_row_symbol(tb, row)) != 0) return -1;
  return taskbar_menu_dl_emit_fit(dl, f, 40, ey + 6,
                                  (width > 50u) ? width - 50u : 0u,
                                  label, active ? theme->accent : theme->text);
}

static uint32_t taskbar_menu_dl_session_button_color(uint32_t slot) {
  if (slot == 0u) return 0x002F80EDu;
  if (slot == 1u) return 0x00F2994Au;
  return 0x00EB5757u;
}

static int taskbar_menu_dl_emit_session_footer(struct capy_display_list *dl,
                                               struct taskbar *tb,
                                               const struct font *f,
                                               const struct gui_theme_palette *theme,
                                               uint32_t width,
                                               uint32_t height) {
  uint32_t count = taskbar_session_count(tb);
  uint32_t gap = 6u;
  uint32_t margin = 10u;
  uint32_t usable;
  uint32_t button_w;
  int32_t y;
  if (!tb || count == 0u || height < TASKBAR_MENU_FOOTER_HEIGHT) return 0;
  y = (int32_t)(height - TASKBAR_MENU_FOOTER_HEIGHT);
  if (taskbar_menu_dl_emit_rect(dl, 8, y, width > 16u ? width - 16u : 0u,
                                1u, theme->window_border) != 0) return -1;
  if (width <= margin * 2u) return 0;
  usable = width - margin * 2u;
  if (count > 1u && usable > gap * (count - 1u)) usable -= gap * (count - 1u);
  button_w = count ? usable / count : 0u;
  for (uint32_t slot = 0u; slot < count; slot++) {
    int index = taskbar_session_entry_by_slot(tb, slot);
    int32_t bx = (int32_t)(margin + slot * (button_w + gap));
    uint32_t bg = taskbar_menu_dl_session_button_color(slot);
    if (index < 0) continue;
    if (index == tb->hover_entry ||
        (tb->hover_entry < 0 && index == tb->selected_entry)) {
      bg = taskbar_menu_dl_lighten(bg, 44u);
    }
    if (taskbar_menu_dl_emit_rect(dl, bx, y + 8, button_w, 24u, bg) != 0) return -1;
    if (button_w > 4u) {
      if (taskbar_menu_dl_emit_rect(dl, bx + 2, y + 10, button_w - 4u, 1u,
                                    taskbar_menu_dl_lighten(bg, 95u)) != 0) return -1;
      if (taskbar_menu_dl_emit_rect(dl, bx + 2, y + 29, button_w - 4u, 1u,
                                    taskbar_menu_dl_darken(bg, 95u)) != 0) return -1;
    }
    if (taskbar_menu_dl_emit_rect(dl, bx, y + 8, button_w, 1u, theme->window_border) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, bx, y + 31, button_w, 1u, theme->window_border) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(dl, bx, y + 8, 1u, 24u, theme->window_border) != 0) return -1;
    if (button_w > 0u &&
        taskbar_menu_dl_emit_rect(dl, bx + (int32_t)button_w - 1, y + 8, 1u,
                                  24u, theme->window_border) != 0) return -1;
    if (f && button_w > 18u) {
      const char *label = tb->menu_entries[index].label;
      if (taskbar_menu_dl_emit_fit(dl, f, bx + 4, y + 15,
                                   button_w > 8u ? button_w - 8u : button_w,
                                   label, 0x00FFFFFFu) != 0) return -1;
    }
  }
  return 0;
}

static int taskbar_menu_emit_display_list(void *producer,
                                          struct capy_display_list *out) {
  struct gui_window *win = (struct gui_window *)producer;
  struct taskbar *tb;
  const struct gui_theme_palette *theme = compositor_theme();
  const struct font *f = font_default();
  struct gui_surface *s;
  char search_line[64];
  char recent_label[48];
  int32_t ey = TASKBAR_MENU_HEADER_HEIGHT;
  uint32_t recent_available;
  uint32_t footer_h;
  uint32_t scroll_max;
  const char *last = "";
  int32_t list_top = (int32_t)TASKBAR_MENU_HEADER_HEIGHT;
  int32_t list_bottom;
  int visible = 0;
  if (!win || !win->user_data || !out || !f) return -1;
  tb = (struct taskbar *)win->user_data;
  s = &win->surface;
  taskbar_clamp_menu_scroll(tb);
  recent_available = taskbar_recent_available_count(tb);
  footer_h = taskbar_menu_footer_height(tb);
  scroll_max = taskbar_menu_scroll_max(tb);
  list_bottom = (int32_t)(s->height > footer_h ? s->height - footer_h : s->height);
  ey -= tb->menu_scroll_offset;
  out->count = 0u;
  out->text_used = 0u;
  if (taskbar_menu_dl_emit_rect(out, 0, 0, s->width, s->height, theme->window_bg) != 0) return -1;
  if (taskbar_menu_dl_emit_rect(out, 0, 0, 4u, s->height, theme->accent_alt) != 0) return -1;
  if (taskbar_menu_dl_emit_fit(out, f, 16, 8, s->width > 32u ? s->width - 32u : 0u,
                               "Capy Launcher", theme->accent) != 0) return -1;
  if (s->width > 24u) {
    if (taskbar_menu_dl_emit_rect(out, 12, 28, s->width - 24u, 22u,
                                  theme->taskbar_bg) != 0) return -1;
    if (taskbar_menu_dl_emit_rect(out, 12, 28, s->width - 24u, 1u,
                                  theme->window_border) != 0) return -1;
  }
  search_line[0] = '\0';
  tb_strcpy(search_line, tb->menu_filter[0] ? "Search: " : "Type to search",
            sizeof(search_line));
  if (tb->menu_filter[0]) tb_append(search_line, sizeof(search_line), tb->menu_filter);
  if (taskbar_menu_dl_emit_fit(out, f, 18, 34, s->width > 36u ? s->width - 36u : 0u,
                               search_line,
                               tb->menu_filter[0] ? theme->text : theme->text_muted) != 0) return -1;
  if (list_bottom > list_top &&
      taskbar_menu_dl_emit_rect(out, 8, list_top, s->width > 16u ? s->width - 16u : 0u,
                                1u, theme->window_border) != 0) return -1;
  if (recent_available > 0u) {
    tb_strcpy(recent_label, APP_T("Recentes ", "Recent apps ", "Recientes "),
              sizeof(recent_label));
    tb_append(recent_label, sizeof(recent_label), ">");
    if (ey >= list_top && ey + (int32_t)TASKBAR_MENU_ENTRY_HEIGHT <= list_bottom) {
      if (taskbar_menu_dl_emit_row(out, tb, f, theme, s->width,
                                   TASKBAR_MENU_ROW_RECENT_TOGGLE, ey,
                                   recent_label) != 0) return -1;
    }
    ey += TASKBAR_MENU_ENTRY_HEIGHT;
    visible = 1;
  }
  for (uint32_t i = 0u; i < tb->menu_entry_count; i++) {
    const char *group = NULL;
    if (!taskbar_menu_entry_matches(tb, i)) continue;
    group = taskbar_menu_group_label(tb, i);
    if (!tb_streq(last, group)) {
      if (ey >= list_top && ey + (int32_t)TASKBAR_MENU_CATEGORY_HEIGHT <= list_bottom) {
        if (taskbar_menu_dl_emit_fit(out, f, 16, ey + 2,
                                     s->width > 32u ? s->width - 32u : 0u,
                                     group, theme->text_muted) != 0) return -1;
      }
      ey += TASKBAR_MENU_CATEGORY_HEIGHT;
      last = group;
    }
    if (ey >= list_top && ey + (int32_t)TASKBAR_MENU_ENTRY_HEIGHT <= list_bottom) {
      if (taskbar_menu_dl_emit_row(out, tb, f, theme, s->width, (int)i, ey,
                                   tb->menu_entries[i].label) != 0) return -1;
    }
    ey += TASKBAR_MENU_ENTRY_HEIGHT;
    visible = 1;
  }
  if (!visible) {
    int32_t empty_y = list_top + 4;
    if (empty_y + (int32_t)TASKBAR_MENU_EMPTY_HEIGHT <= list_bottom) {
      if (taskbar_menu_dl_emit_fit(out, f, 16, empty_y + 6,
                                   s->width > 32u ? s->width - 32u : 0u,
                                   "No apps found", theme->text_muted) != 0) return -1;
    }
  }
  if (scroll_max > 0u && list_bottom > list_top + 10 && s->width > 12u) {
    uint32_t track_h = (uint32_t)(list_bottom - list_top - 6);
    uint32_t thumb_h = track_h > 18u ? track_h / 3u : track_h;
    int32_t track_x = (int32_t)s->width - 8;
    int32_t track_y = list_top + 3;
    int32_t thumb_y = track_y;
    if (thumb_h < 12u && track_h >= 12u) thumb_h = 12u;
    if (thumb_h > track_h) thumb_h = track_h;
    if (taskbar_menu_dl_emit_rect(out, track_x, track_y, 3u, track_h,
                                  theme->window_border) != 0) return -1;
    if (track_h > thumb_h) {
      thumb_y += (int32_t)(((uint32_t)tb->menu_scroll_offset * (track_h - thumb_h)) / scroll_max);
    }
    if (taskbar_menu_dl_emit_rect(out, track_x - 1, thumb_y, 5u, thumb_h,
                                  theme->accent) != 0) return -1;
  }
  return taskbar_menu_dl_emit_session_footer(out, tb, f, theme, s->width, s->height);
}

static int taskbar_recent_emit_display_list(void *producer,
                                            struct capy_display_list *out) {
  struct gui_window *win = (struct gui_window *)producer;
  struct taskbar *tb;
  const struct gui_theme_palette *theme = compositor_theme();
  const struct font *f = font_default();
  struct gui_surface *s;
  int32_t ey = 2;
  if (!win || !win->user_data || !out || !f) return -1;
  tb = (struct taskbar *)win->user_data;
  s = &win->surface;
  out->count = 0u;
  out->text_used = 0u;
  if (taskbar_menu_dl_emit_rect(out, 0, 0, s->width, s->height, theme->window_bg) != 0) return -1;
  for (uint32_t i = 0u; i < tb->recent_count; i++) {
    if (!taskbar_recent_entry_matches(tb, i)) continue;
    if (taskbar_menu_dl_emit_row(out, tb, f, theme, s->width,
                                 taskbar_recent_row(i), ey,
                                 tb->recent_entries[i].label) != 0) return -1;
    ey += TASKBAR_MENU_ENTRY_HEIGHT;
  }
  return 0;
}

static int taskbar_menu_dl_render_window(struct taskbar_menu_dl_bridge *bridge,
                                         struct gui_window *window,
                                         capyui_display_adapter_emit_fn emit) {
  const struct capy_display_list *prev;
  uint8_t next;
  int rc;
  if (!bridge || !window || !emit) return -1;
  taskbar_menu_dl_init_once(bridge);
  if (bridge->last_w != window->surface.width ||
      bridge->last_h != window->surface.height) {
    bridge->have_prev = 0;
    bridge->last_w = window->surface.width;
    bridge->last_h = window->surface.height;
  }
  next = bridge->have_prev ? (uint8_t)(bridge->prev_index ^ 1u) : 0u;
  taskbar_menu_dl_prepare(bridge, next);
  prev = bridge->have_prev ? &bridge->lists[bridge->prev_index] : 0;
  rc = capyui_display_adapter_render_producer_window(
      window, prev, &bridge->lists[next], emit, window, 0);
  if (rc != CAPYUI_DISPLAY_ADAPTER_OK) return rc;
  bridge->prev_index = next;
  bridge->have_prev = 1;
  return CAPYUI_DISPLAY_ADAPTER_OK;
}

int taskbar_menu_render_display_list(struct gui_window *win) {
  return taskbar_menu_dl_render_window(&g_menu_dl, win, taskbar_menu_emit_display_list);
}

int taskbar_recent_render_display_list(struct gui_window *win) {
  return taskbar_menu_dl_render_window(&g_recent_dl, win, taskbar_recent_emit_display_list);
}
#endif
