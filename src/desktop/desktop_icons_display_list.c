#include "gui/desktop_icons.h"
#include "gui/font.h"
#include "gui/compositor.h"
#include "internal/desktop_icons_internal.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#include "gui/capyui_display_adapter.h"
#include "capy_display_list.h"

#define DESKTOP_ICONS_DL_CMD_CAP 2304u
#define DESKTOP_ICONS_DL_TEXT_CAP 4096u

static struct capy_dl_cmd g_desktop_icons_dl_cmds[DESKTOP_ICONS_DL_CMD_CAP];
static char g_desktop_icons_dl_text[DESKTOP_ICONS_DL_TEXT_CAP];

static uint32_t di_dl_strlen(const char *s) {
  uint32_t n = 0u;
  while (s && s[n]) n++;
  return n;
}

static uint32_t di_dl_mix_color(uint32_t color, uint32_t target,
                                uint8_t amount) {
  uint32_t r = (color >> 16) & 0xFFu;
  uint32_t g = (color >> 8) & 0xFFu;
  uint32_t b = color & 0xFFu;
  uint32_t tr = (target >> 16) & 0xFFu;
  uint32_t tg = (target >> 8) & 0xFFu;
  uint32_t tb = target & 0xFFu;
  int32_t nr = (int32_t)r + (((int32_t)tr - (int32_t)r) * amount) / 255;
  int32_t ng = (int32_t)g + (((int32_t)tg - (int32_t)g) * amount) / 255;
  int32_t nb = (int32_t)b + (((int32_t)tb - (int32_t)b) * amount) / 255;
  if (nr < 0) nr = 0;
  if (ng < 0) ng = 0;
  if (nb < 0) nb = 0;
  if (nr > 255) nr = 255;
  if (ng > 255) ng = 255;
  if (nb > 255) nb = 255;
  return ((uint32_t)nr << 16) | ((uint32_t)ng << 8) | (uint32_t)nb;
}

static uint32_t di_dl_usable_height(void) {
  uint32_t reserved = g_di.taskbar_h + DESKTOP_ICON_PAD_BOTTOM;
  if (g_di.screen_h <= reserved) return 0u;
  return g_di.screen_h - reserved;
}

static uint32_t di_dl_usable_width(void) {
  if (g_di.screen_w <= DESKTOP_ICON_PAD_RIGHT) return g_di.screen_w;
  return g_di.screen_w - DESKTOP_ICON_PAD_RIGHT;
}

static void desktop_icons_dl_prepare(struct capy_display_list *dl) {
  if (!dl) return;
  dl->cmds = g_desktop_icons_dl_cmds;
  dl->count = 0u;
  dl->capacity = DESKTOP_ICONS_DL_CMD_CAP;
  dl->text_pool = g_desktop_icons_dl_text;
  dl->text_used = 0u;
  dl->text_capacity = DESKTOP_ICONS_DL_TEXT_CAP;
  dl->version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl->theme = 0;
  dl->dpi_scale_x256 = 256u;
  dl->reserved_dpi = 0u;
}

static struct capy_dl_cmd *desktop_icons_dl_push(struct capy_display_list *dl) {
  struct capy_dl_cmd *cmd;
  if (!dl || !dl->cmds || dl->count >= dl->capacity) return 0;
  cmd = &dl->cmds[dl->count++];
  cmd->op = 0;
  cmd->rect.x = 0;
  cmd->rect.y = 0;
  cmd->rect.width = 0u;
  cmd->rect.height = 0u;
  cmd->color = 0u;
  cmd->border_width = 0u;
  cmd->text_offset = 0u;
  cmd->text_len = 0u;
  cmd->font_size = 0u;
  return cmd;
}

static int desktop_icons_dl_emit_rect(struct capy_display_list *dl,
                                      int32_t x,
                                      int32_t y,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t color) {
  struct capy_dl_cmd *cmd;
  if (width == 0u || height == 0u) return 0;
  cmd = desktop_icons_dl_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_RECT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  return 0;
}

static int desktop_icons_dl_copy_text(struct capy_display_list *dl,
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

static int desktop_icons_dl_emit_text(struct capy_display_list *dl,
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
  if (desktop_icons_dl_copy_text(dl, text, &text_offset, &text_len) != 0) return -1;
  cmd = desktop_icons_dl_push(dl);
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

static int desktop_icons_dl_draw_hline(struct capy_display_list *dl,
                                       int32_t x,
                                       int32_t y,
                                       uint32_t width,
                                       uint32_t color) {
  return desktop_icons_dl_emit_rect(dl, x, y, width, 1u, color);
}

static int desktop_icons_dl_draw_vline(struct capy_display_list *dl,
                                       int32_t x,
                                       int32_t y,
                                       uint32_t height,
                                       uint32_t color) {
  return desktop_icons_dl_emit_rect(dl, x, y, 1u, height, color);
}

static int desktop_icons_dl_draw_folder(struct capy_display_list *dl,
                                        int32_t x,
                                        int32_t y,
                                        const struct gui_theme_palette *theme) {
  uint32_t fill = di_dl_mix_color(theme->accent, theme->wallpaper, 18u);
  uint32_t lip = di_dl_mix_color(theme->accent, 0x00FFFFFFu, 38u);
  uint32_t outline = theme->window_border;
  uint32_t shade = di_dl_mix_color(theme->accent, 0x00000000u, 34u);
  if (desktop_icons_dl_emit_rect(dl, x + 3, y + 5, 10u, 4u, lip) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 13, y + 7, 16u, 2u, lip) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 2, y + 9, 28u, 18u, fill) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 4, y + 24, 24u, 2u, shade) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 5, y + 11, 22u, 2u,
                                 di_dl_mix_color(fill, 0x00FFFFFFu, 62u)) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 3, y + 4, 10u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 13, y + 6, 16u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 2, y + 8, 28u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 2, y + 27, 28u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_vline(dl, x + 2, y + 9, 19u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_vline(dl, x + 30, y + 9, 18u, outline) != 0) return -1;
  return desktop_icons_dl_draw_vline(dl, x + 13, y + 5, 3u, outline);
}

static int desktop_icons_dl_draw_file(struct capy_display_list *dl,
                                      int32_t x,
                                      int32_t y,
                                      const struct gui_theme_palette *theme) {
  uint32_t fill = di_dl_mix_color(theme->window_bg, theme->accent_alt, 14u);
  uint32_t fold = di_dl_mix_color(theme->accent_alt, theme->window_bg, 34u);
  uint32_t outline = theme->window_border;
  uint32_t ink = di_dl_mix_color(theme->accent_alt, theme->text, 64u);
  if (desktop_icons_dl_emit_rect(dl, x + 6, y + 2, 16u, 27u, fill) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 22, y + 8, 4u, 21u, fill) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 22, y + 3, 1u, 5u, fold) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 23, y + 4, 1u, 4u, fold) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 24, y + 5, 1u, 3u, fold) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 25, y + 6, 1u, 2u, fold) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 9, y + 14, 14u, 2u, ink) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 9, y + 19, 12u, 2u, ink) != 0) return -1;
  if (desktop_icons_dl_emit_rect(dl, x + 9, y + 24, 9u, 2u, ink) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 7, y + 2, 15u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 6, y + 29, 20u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_vline(dl, x + 6, y + 3, 26u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_vline(dl, x + 26, y + 8, 21u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 22, y + 8, 5u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_vline(dl, x + 22, y + 3, 5u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 23, y + 4, 1u, outline) != 0) return -1;
  if (desktop_icons_dl_draw_hline(dl, x + 24, y + 5, 1u, outline) != 0) return -1;
  return desktop_icons_dl_draw_hline(dl, x + 25, y + 6, 1u, outline);
}

static int desktop_icons_dl_emit_wallpaper(struct capy_display_list *dl,
                                           struct gui_surface *s,
                                           const struct gui_theme_palette *theme) {
  uint32_t usable_h = di_dl_usable_height();
  uint32_t bands = 8u;
  uint32_t top;
  uint32_t bottom;
  if (!s || !theme || usable_h == 0u) return 0;
  top = di_dl_mix_color(theme->wallpaper, theme->accent_alt, 18u);
  bottom = di_dl_mix_color(theme->wallpaper, 0x00000000u, 32u);
  for (uint32_t band = 0u; band < bands; band++) {
    uint32_t y0 = (usable_h * band) / bands;
    uint32_t y1 = (usable_h * (band + 1u)) / bands;
    uint8_t amount = (bands > 1u) ? (uint8_t)((band * 255u) / (bands - 1u)) : 0u;
    if (desktop_icons_dl_emit_rect(dl, 0, (int32_t)y0, s->width, y1 - y0,
                                   di_dl_mix_color(top, bottom, amount)) != 0) return -1;
  }
  return 0;
}

static void desktop_icons_dl_split_name(const char *name,
                                        uint32_t chars_per_line,
                                        char *line1,
                                        char *line2) {
  uint32_t nlen = di_dl_strlen(name);
  uint32_t split = nlen <= chars_per_line ? nlen : chars_per_line;
  line1[0] = '\0';
  line2[0] = '\0';
  for (uint32_t k = 0u; k < split && k < 15u; k++) line1[k] = name[k];
  line1[split < 15u ? split : 15u] = '\0';
  if (nlen > chars_per_line) {
    uint32_t rem = nlen - chars_per_line;
    uint32_t rs = rem <= chars_per_line ? rem : chars_per_line;
    for (uint32_t k = 0u; k < rs && k < 15u; k++) line2[k] = name[chars_per_line + k];
    if (rem > chars_per_line && rs >= 3u) {
      line2[rs - 3u] = '.';
      line2[rs - 2u] = '.';
      line2[rs - 1u] = '.';
    }
    line2[rs < 15u ? rs : 15u] = '\0';
  }
}

static int desktop_icons_dl_emit_icon(struct capy_display_list *dl,
                                      const struct font *f,
                                      const struct gui_theme_palette *theme,
                                      uint32_t index,
                                      uint32_t usable_w,
                                      uint32_t usable_h) {
  int32_t x = 0;
  int32_t y = 0;
  int32_t ix;
  int32_t iy;
  int is_dir;
  int sel;
  int drop;
  uint32_t shadow;
  char line1[16];
  char line2[16];
  uint32_t gw;
  uint32_t text_area;
  uint32_t chars_per_line;
  int32_t name_y;
  uint32_t chip_h;
  int32_t lw1;
  int32_t name_x1;
  di_icon_position(index, &x, &y);
  if (x + (int32_t)DESKTOP_ICON_CELL_W > (int32_t)usable_w) return 1;
  if (y + (int32_t)DESKTOP_ICON_CELL_H > (int32_t)usable_h) return 1;
  sel = ((int)index == g_di.selected);
  drop = ((int)index == g_di.drag_over);
  if (sel || drop) {
    uint32_t sel_bg = drop ? di_dl_mix_color(theme->accent, theme->wallpaper, 42u)
                           : di_dl_mix_color(theme->accent_alt, theme->wallpaper, 58u);
    if (desktop_icons_dl_emit_rect(dl, x - 3, y - 3, DESKTOP_ICON_CELL_W,
                                   DESKTOP_ICON_CELL_H - 2u, sel_bg) != 0) return -1;
    if (desktop_icons_dl_emit_rect(dl, x - 3, y - 3, DESKTOP_ICON_CELL_W,
                                   1u, theme->accent) != 0) return -1;
    if (desktop_icons_dl_emit_rect(dl, x - 3,
                                   y + (int32_t)DESKTOP_ICON_CELL_H - 5,
                                   DESKTOP_ICON_CELL_W, 1u,
                                   theme->accent) != 0) return -1;
  }
  ix = x + (int32_t)((DESKTOP_ICON_CELL_W - 32u) / 2u);
  iy = y + 5;
  is_dir = (g_di.entries[index].mode & VFS_MODE_DIR) != 0;
  shadow = di_dl_mix_color(theme->wallpaper, 0x00000000u, 55u);
  if (desktop_icons_dl_emit_rect(dl, ix + 2, iy + 3, 32u, 32u, shadow) != 0) return -1;
  if (is_dir) {
    if (desktop_icons_dl_draw_folder(dl, ix, iy, theme) != 0) return -1;
  } else {
    if (desktop_icons_dl_draw_file(dl, ix, iy, theme) != 0) return -1;
  }
  gw = f->glyph_width ? f->glyph_width : 8u;
  text_area = DESKTOP_ICON_CELL_W - 10u;
  chars_per_line = text_area / gw;
  if (chars_per_line < 3u) chars_per_line = 3u;
  if (chars_per_line > 15u) chars_per_line = 15u;
  desktop_icons_dl_split_name(g_di.entries[index].name, chars_per_line, line1, line2);
  name_y = y + 43;
  chip_h = line2[0] ? 27u : 15u;
  if (sel && desktop_icons_dl_emit_rect(dl, x + 4, name_y - 2,
                                        text_area, chip_h,
                                        di_dl_mix_color(theme->accent_alt,
                                                        theme->wallpaper, 38u)) != 0) return -1;
  lw1 = (int32_t)(di_dl_strlen(line1) * gw);
  name_x1 = x + 5;
  if ((uint32_t)lw1 < text_area) name_x1 += (int32_t)((text_area - (uint32_t)lw1) / 2u);
  if (desktop_icons_dl_emit_text(dl, name_x1, name_y, text_area,
                                 f->glyph_height, line1,
                                 sel ? theme->accent_text : theme->text) != 0) return -1;
  if (line2[0]) {
    int32_t lw2 = (int32_t)(di_dl_strlen(line2) * gw);
    int32_t name_x2 = x + 5;
    if ((uint32_t)lw2 < text_area) name_x2 += (int32_t)((text_area - (uint32_t)lw2) / 2u);
    if (desktop_icons_dl_emit_text(dl, name_x2, name_y + 12, text_area,
                                   f->glyph_height, line2,
                                   sel ? theme->accent_text : theme->text) != 0) return -1;
  }
  return 0;
}

int desktop_icons_render_display_list(struct gui_surface *s) {
  struct capy_display_list dl;
  const struct gui_theme_palette *theme = compositor_theme();
  const struct font *f = font_default();
  uint32_t usable_h;
  uint32_t usable_w;
  if (!s || !theme) return -1;
  g_di.screen_w = s->width;
  g_di.screen_h = s->height;
  usable_h = di_dl_usable_height();
  usable_w = di_dl_usable_width();
  desktop_icons_dl_prepare(&dl);
  if (desktop_icons_dl_emit_wallpaper(&dl, s, theme) != 0) return -1;
  if (!f || usable_h == 0u) return capyui_display_adapter_render(&dl, s, 0, 0);
  for (uint32_t i = 0u; i < g_di.count; i++) {
    int rc = desktop_icons_dl_emit_icon(&dl, f, theme, i, usable_w, usable_h);
    if (rc < 0) return -1;
    if (rc > 0) break;
  }
  return capyui_display_adapter_render(&dl, s, 0, 0);
}
#endif
