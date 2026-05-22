#ifndef CAPY_UI_LAYOUT_H
#define CAPY_UI_LAYOUT_H

#include <stddef.h>
#include <stdint.h>

enum capy_layout_kind {
  CAPY_LAYOUT_NONE = 0,
  CAPY_LAYOUT_STACK,
  CAPY_LAYOUT_GRID,
  CAPY_LAYOUT_FLOW
};

struct capy_layout_constraints {
  uint32_t min_w;
  uint32_t min_h;
  uint32_t max_w;
  uint32_t max_h;
  uint8_t axis;
  uint8_t cols;
  uint8_t gap;
  uint8_t pad_l;
  uint8_t pad_t;
  uint8_t pad_r;
  uint8_t pad_b;
  /* Since 0.13: when non-zero, capy_widget_arrange mirrors the x-coordinate
   * of every direct child within the inner content rect after the layout
   * kind-specific pass. Vertical-only layouts are unaffected because the
   * mirror is a no-op when iw covers the child width exactly. */
  uint8_t rtl;
};

struct capy_layout_node {
  enum capy_layout_kind kind;
  struct capy_layout_constraints c;
  uint8_t grow;
  uint8_t shrink;
};

struct capy_widget;

void capy_widget_set_layout(struct capy_widget *widget,
                            const struct capy_layout_node *node);
int capy_widget_measure(struct capy_widget *root, uint32_t avail_w,
                        uint32_t avail_h);
int capy_widget_arrange(struct capy_widget *root);

#endif
