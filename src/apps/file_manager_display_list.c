#include "apps/file_manager.h"
#include "gui/compositor.h"
#include "gui/font.h"
#include "lang/app_language.h"
#include "util/kstring.h"

#include "internal/file_manager_internal.h"
#include "internal/app_display_list_bridge.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#define FM_DL_CMD_CAP 384u
#define FM_DL_TEXT_CAP 8192u

static struct capy_dl_cmd g_fm_dl_cmds[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT][FM_DL_CMD_CAP];
static char g_fm_dl_text[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT][FM_DL_TEXT_CAP];
static struct app_display_list_bridge g_fm_dl_bridge;

static void fm_dl_init_once(void) {
  if (g_fm_dl_bridge.initialized) return;
  app_display_list_bridge_init(&g_fm_dl_bridge,
                               g_fm_dl_cmds[0],
                               g_fm_dl_cmds[1],
                               FM_DL_CMD_CAP,
                               g_fm_dl_text[0],
                               g_fm_dl_text[1],
                               FM_DL_TEXT_CAP);
}

void file_manager_display_list_reset(void) {
  app_display_list_bridge_reset(&g_fm_dl_bridge);
}

static int fm_dl_emit_fit(struct capy_display_list *out,
                          const struct font *f,
                          int32_t x,
                          int32_t y,
                          uint32_t max_width,
                          const char *text,
                          uint32_t color) {
  char fitted[96];
  fm_fit_text(f, text, max_width, fitted, sizeof(fitted));
  if (!fitted[0]) return 0;
  return app_display_list_emit_text(out, x, y, max_width, f->glyph_height,
                                    fitted, color, 16u);
}

static uint32_t fm_dl_mix_color(uint32_t color, uint32_t target,
                                uint8_t amount) {
  int r = (int)((color >> 16) & 0xFFu);
  int g = (int)((color >> 8) & 0xFFu);
  int b = (int)(color & 0xFFu);
  int tr = (int)((target >> 16) & 0xFFu);
  int tg = (int)((target >> 8) & 0xFFu);
  int tb = (int)(target & 0xFFu);
  r += ((tr - r) * (int)amount) / 255;
  g += ((tg - g) * (int)amount) / 255;
  b += ((tb - b) * (int)amount) / 255;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static const char *fm_dl_button_label(const char *label) {
  if (!label) return "";
  if (label[0] == '0') return "<";
  if (label[0] == '1') return "^";
  if (label[0] == '2') return "+F";
  if (label[0] == '3') return "+D";
  if (label[0] == '4') return "R";
  if (label[0] == '5') return "X";
  return label;
}

static int fm_dl_emit_button(struct capy_display_list *out,
                             const struct font *f,
                             int32_t x,
                             int32_t y,
                             int32_t w,
                             const char *label,
                             uint32_t bg,
                             uint32_t fg,
                             uint32_t border) {
  const char *text = fm_dl_button_label(label);
  uint32_t text_w = 0u;
  int32_t tx;
  if (w <= 0) return 0;
  if (app_display_list_emit_rect(out, x, y, (uint32_t)w, FM_BUTTON_H, bg) != 0) return -1;
  if (app_display_list_emit_border_rect(out, x, y, (uint32_t)w, FM_BUTTON_H,
                                        border, 1u) != 0) return -1;
  if (w > 4) {
    if (app_display_list_emit_rect(out, x + 2, y + 2, (uint32_t)w - 4u, 1u,
                                   fm_dl_mix_color(bg, 0x00FFFFFF, 72u)) != 0) return -1;
  }
  if (!f || !text[0]) return 0;
  text_w = font_string_width(f, text);
  tx = (text_w < (uint32_t)w) ? x + (int32_t)(((uint32_t)w - text_w) / 2u) : x + 2;
  return app_display_list_emit_text(out, tx, y + 5,
                                    text_w < (uint32_t)w ? text_w : (uint32_t)w,
                                    f->glyph_height, text, fg, 16u);
}

static int fm_dl_emit_toolbar(struct capy_display_list *out,
                              struct file_manager_app *app,
                              const struct font *f,
                              const struct gui_theme_palette *theme,
                              int32_t toolbar_x,
                              int32_t button_w,
                              int32_t button_gap) {
  uint32_t btn_bg = fm_dl_mix_color(theme->accent_alt, theme->window_bg, 30u);
  uint32_t disabled_bg = fm_dl_mix_color(theme->window_border, theme->window_bg, 70u);
  if (fm_dl_emit_button(out, f, toolbar_x, FM_BUTTON_Y, button_w, "0",
                        app->previous_path[0] ? btn_bg : disabled_bg,
                        app->previous_path[0] ? theme->accent_text : theme->text_muted,
                        theme->window_border) != 0) return -1;
  if (fm_dl_emit_button(out, f, toolbar_x + button_w + button_gap, FM_BUTTON_Y,
                        button_w, "1", btn_bg, theme->accent_text,
                        theme->window_border) != 0) return -1;
  if (fm_dl_emit_button(out, f, toolbar_x + (button_w + button_gap) * 2,
                        FM_BUTTON_Y, button_w, "2", btn_bg, theme->accent_text,
                        theme->window_border) != 0) return -1;
  if (fm_dl_emit_button(out, f, toolbar_x + (button_w + button_gap) * 3,
                        FM_BUTTON_Y, button_w, "3", btn_bg, theme->accent_text,
                        theme->window_border) != 0) return -1;
  if (fm_dl_emit_button(out, f, toolbar_x + (button_w + button_gap) * 4,
                        FM_BUTTON_Y, button_w, "4", btn_bg, theme->accent_text,
                        theme->window_border) != 0) return -1;
  return fm_dl_emit_button(out, f, toolbar_x + (button_w + button_gap) * 5,
                           FM_BUTTON_Y, button_w, "5",
                           (app->selected >= 0) ? 0x00CC3333 : theme->window_border,
                           (app->selected >= 0) ? 0x00FFFFFF : theme->text_muted,
                           theme->window_border);
}

static int fm_dl_emit_list(struct capy_display_list *out,
                           struct file_manager_app *app,
                           const struct font *f,
                           const struct gui_theme_palette *theme,
                           int32_t row_h,
                           int32_t status_y) {
  int32_t y = FM_TOOLBAR_H + 4;
  for (int i = app->scroll_offset;
       i < app->entry_count && y + row_h <= status_y; i++) {
    struct fm_entry *e = &app->entries[i];
    uint32_t color = (e->mode & 0x2) ? theme->accent : theme->text;
    uint32_t bg = (i == app->selected) ? theme->accent_alt :
                  ((i == app->drag_over || i == app->external_drag_over)
                       ? theme->taskbar_highlight : theme->window_bg);
    const char *icon = (e->mode & 0x2) ? "[D] " : "[F] ";
    if (app_display_list_emit_rect(out, 0, y, app->window->surface.width,
                                   (uint32_t)row_h, bg) != 0) return -1;
    if (app_display_list_emit_text(out, 8, y + 2, 32u, f->glyph_height,
                                   icon, theme->text_muted, 16u) != 0) return -1;
    if (fm_dl_emit_fit(out, f, 40, y + 2,
                       (app->window->surface.width > 48u)
                           ? app->window->surface.width - 48u : 0u,
                       e->name, color) != 0) return -1;
    y += row_h;
  }
  return 0;
}

static int fm_emit_display_list(void *producer, struct capy_display_list *out) {
  struct file_manager_app *app = (struct file_manager_app *)producer;
  struct gui_surface *s;
  const struct font *f = font_default();
  const struct gui_theme_palette *theme = compositor_theme();
  int32_t toolbar_x = 0;
  int32_t button_w = 0;
  int32_t button_gap = 0;
  int32_t row_h = 0;
  int32_t status_y = 0;
  if (!app || !app->window || !out || !f) return -1;
  s = &app->window->surface;
  fm_toolbar_layout(s->width, &toolbar_x, &button_w, &button_gap);
  row_h = fm_row_height(f);
  out->count = 0u;
  out->text_used = 0u;
  if (app_display_list_emit_rect(out, 0, 0, s->width, s->height,
                                 theme->window_bg) != 0) return -1;
  if (app_display_list_emit_rect(out, 0, 0, s->width, FM_PATHBAR_H,
                                 theme->taskbar_bg) != 0) return -1;
  if (app_display_list_emit_rect(out, 8, 5,
                                 s->width > 16u ? s->width - 16u : 0u, 15u,
                                 fm_dl_mix_color(theme->window_bg, theme->taskbar_bg, 45u)) != 0) return -1;
  if (fm_dl_emit_fit(out, f, 14, 8, s->width > 28u ? s->width - 28u : 0u,
                     app->current_path, theme->accent) != 0) return -1;
  if (app_display_list_emit_rect(out, 0, FM_PATHBAR_H, s->width,
                                 FM_TOOLBAR_H - FM_PATHBAR_H - 1u,
                                 theme->window_bg) != 0) return -1;
  if (fm_dl_emit_toolbar(out, app, f, theme, toolbar_x, button_w, button_gap) != 0) return -1;
  if (s->height > FM_TOOLBAR_H) {
    if (app_display_list_emit_rect(out, 0, FM_TOOLBAR_H - 1, s->width, 1u,
                                   theme->window_border) != 0) return -1;
  }
  status_y = (s->height > f->glyph_height + 2u)
      ? (int32_t)s->height - (int32_t)f->glyph_height - 2
      : (int32_t)s->height;
  if (fm_dl_emit_list(out, app, f, theme, row_h, status_y) != 0) return -1;
  if (s->height > 18) {
    if (fm_dl_emit_fit(out, f, 8, status_y,
                       (s->width > 16u) ? s->width - 16u : 0u,
                       app->status_text,
                       app->status_text[0] ? app->status_color : theme->text_muted) != 0) return -1;
  }
  return 0;
}

int file_manager_render_display_list(struct file_manager_app *app) {
  if (!app || !app->window) return -1;
  fm_dl_init_once();
  return app_display_list_bridge_render_window(&g_fm_dl_bridge, app->window,
                                               fm_emit_display_list, app);
}
#endif
