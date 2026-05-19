#ifndef CAPY_UI_DISPLAY_LIST_H
#define CAPY_UI_DISPLAY_LIST_H

#include <stddef.h>
#include <stdint.h>

#include "capy_widget.h"

#define CAPY_DISPLAY_LIST_SCHEMA_VERSION 2u

enum capy_dl_op {
  CAPY_DL_NONE = 0,
  CAPY_DL_RECT,
  CAPY_DL_BORDER,
  CAPY_DL_TEXT,
  CAPY_DL_CLIP_PUSH,
  CAPY_DL_CLIP_POP,
  CAPY_DL_IMAGE_REF,
  CAPY_DL_FOCUS_RING
};

struct capy_dl_cmd {
  enum capy_dl_op op;
  struct capy_ui_rect rect;
  uint32_t color;
  uint16_t text_offset;
  uint16_t text_len;
  uint8_t border_width;
  uint8_t font_size;
  uint16_t reserved;
  uint32_t image_id;
};

struct capy_display_list {
  struct capy_dl_cmd *cmds;
  uint32_t count;
  uint32_t capacity;
  char *text_pool;
  uint32_t text_used;
  uint32_t text_capacity;
  uint32_t version;
  /* Optional theme pointer (since 0.6). NULL means tokens resolve to literal
   * style.*_color fallbacks (backward-compat behaviour for pre-0.6 callers). */
  const struct capy_theme *theme;
};

void capy_display_list_init(struct capy_display_list *dl, void *cmd_buf,
                            uint32_t cmd_cap, char *text_buf,
                            uint32_t text_cap);
void capy_display_list_reset(struct capy_display_list *dl);
int capy_widget_emit(struct capy_widget *root, struct capy_display_list *dl);

#endif
