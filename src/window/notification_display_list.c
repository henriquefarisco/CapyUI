#include "gui/notification.h"
#include "gui/font.h"

#include <stddef.h>
#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#include "gui/capyui_display_adapter.h"
#include "capy_display_list.h"

#define NOTIFY_DL_CMD_CAP 48u
#define NOTIFY_DL_TEXT_CAP (NOTIFY_MAX * NOTIFY_TEXT_MAX)

static struct capy_dl_cmd g_notify_dl_cmds[NOTIFY_DL_CMD_CAP];
static char g_notify_dl_text[NOTIFY_DL_TEXT_CAP];

static void notify_dl_prepare(struct capy_display_list *dl) {
  if (!dl) return;
  dl->cmds = g_notify_dl_cmds;
  dl->count = 0u;
  dl->capacity = NOTIFY_DL_CMD_CAP;
  dl->text_pool = g_notify_dl_text;
  dl->text_used = 0u;
  dl->text_capacity = NOTIFY_DL_TEXT_CAP;
  dl->version = CAPY_DISPLAY_LIST_SCHEMA_VERSION;
  dl->theme = 0;
  dl->dpi_scale_x256 = 256u;
  dl->reserved_dpi = 0u;
}

static struct capy_dl_cmd *notify_dl_push(struct capy_display_list *dl) {
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

static int notify_dl_emit_rect(struct capy_display_list *dl,
                               int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height,
                               uint32_t color) {
  struct capy_dl_cmd *cmd;
  if (width == 0u || height == 0u) return 0;
  cmd = notify_dl_push(dl);
  if (!cmd) return -1;
  cmd->op = CAPY_DL_RECT;
  cmd->rect.x = x;
  cmd->rect.y = y;
  cmd->rect.width = width;
  cmd->rect.height = height;
  cmd->color = color;
  return 0;
}

static int notify_dl_copy_text(struct capy_display_list *dl,
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

static int notify_dl_emit_text(struct capy_display_list *dl,
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
  if (notify_dl_copy_text(dl, text, &text_offset, &text_len) != 0) return -1;
  cmd = notify_dl_push(dl);
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

static uint32_t notify_dl_accent_for(enum notify_type type,
                                     const struct gui_theme_palette *theme) {
  switch (type) {
  case NOTIFY_SUCCESS:
    return 0x00A6E3A1u;
  case NOTIFY_WARNING:
    return 0x00F9E2AFu;
  case NOTIFY_ERROR:
    return 0x00F38BA8u;
  case NOTIFY_INFO:
  default:
    return theme ? theme->accent : 0u;
  }
}

int notify_render_display_list(struct notification_manager *nm,
                               struct gui_surface *surface) {
  struct capy_display_list dl;
  const struct font *f = font_default();
  const struct gui_theme_palette *theme = compositor_theme();
  int32_t base_x;
  int32_t base_y = 12;
  int slot = 0;
  if (!nm || !surface || !f || !theme) return -1;
  notify_dl_prepare(&dl);
  base_x = (nm->screen_w > NOTIFY_WIDTH + 12u)
               ? (int32_t)(nm->screen_w - NOTIFY_WIDTH - 12u)
               : 0;
  for (int i = 0; i < NOTIFY_MAX; i++) {
    int32_t y;
    uint32_t accent;
    if (!nm->items[i].active) continue;
    y = base_y + slot * (int32_t)(NOTIFY_HEIGHT + 8);
    accent = notify_dl_accent_for(nm->items[i].type, theme);
    if (notify_dl_emit_rect(&dl, base_x, y, NOTIFY_WIDTH, NOTIFY_HEIGHT,
                            theme->window_border) != 0) return -1;
    if (notify_dl_emit_rect(&dl, base_x + 1, y + 1, NOTIFY_WIDTH - 2u,
                            NOTIFY_HEIGHT - 2u, theme->window_bg) != 0) return -1;
    if (notify_dl_emit_rect(&dl, base_x + 1, y + 1, 5u,
                            NOTIFY_HEIGHT - 2u, accent) != 0) return -1;
    if (notify_dl_emit_text(&dl, base_x + 14, y + 8,
                            NOTIFY_WIDTH > 22u ? NOTIFY_WIDTH - 22u : 0u,
                            f->glyph_height, nm->items[i].text,
                            theme->text) != 0) return -1;
    slot++;
  }
  return capyui_display_adapter_render(&dl, surface, 0, 0);
}
#endif
