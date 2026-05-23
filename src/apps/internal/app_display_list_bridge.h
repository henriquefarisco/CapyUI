#ifndef APPS_INTERNAL_APP_DISPLAY_LIST_BRIDGE_H
#define APPS_INTERNAL_APP_DISPLAY_LIST_BRIDGE_H

#include <stdint.h>

#if defined(CAPYOS_HAVE_CAPYUI_WIDGET)
#include "capy_display_list.h"
#include "gui/capyui_display_adapter.h"
#include "gui/compositor.h"
#include "gui/widget.h"

#define APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT 2u

struct app_display_list_bridge {
  struct capy_dl_cmd *cmds[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT];
  char *text[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT];
  struct capy_display_list lists[APP_DISPLAY_LIST_BRIDGE_FRAME_COUNT];
  uint32_t cmd_cap;
  uint32_t text_cap;
  int initialized;
  int have_prev;
  uint8_t prev_index;
};

void app_display_list_bridge_init(struct app_display_list_bridge *bridge,
                                  struct capy_dl_cmd *cmds0,
                                  struct capy_dl_cmd *cmds1,
                                  uint32_t cmd_cap,
                                  char *text0,
                                  char *text1,
                                  uint32_t text_cap);
void app_display_list_bridge_reset(struct app_display_list_bridge *bridge);
int app_display_list_bridge_render_window(struct app_display_list_bridge *bridge,
                                          struct gui_window *window,
                                          capyui_display_adapter_emit_fn emit,
                                          void *producer);
struct capy_dl_cmd *app_display_list_push(struct capy_display_list *dl);
int app_display_list_emit_rect(struct capy_display_list *dl,
                               int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height,
                               uint32_t color);
int app_display_list_emit_border_rect(struct capy_display_list *dl,
                                      int32_t x,
                                      int32_t y,
                                      uint32_t width,
                                      uint32_t height,
                                      uint32_t color,
                                      uint8_t border_width);
int app_display_list_emit_text(struct capy_display_list *dl,
                               int32_t x,
                               int32_t y,
                               uint32_t width,
                               uint32_t height,
                               const char *text,
                               uint32_t color,
                               uint8_t font_size);
int app_display_list_emit_widget(struct capy_display_list *dl,
                                 const struct widget *w);
#endif

#endif
